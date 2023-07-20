#pragma once

#include <cassert>
#include <cstring>

#include <context.hpp>

#include <lsAdvect.hpp>
#include <lsDomain.hpp>
#include <lsMesh.hpp>
#include <lsToDiskMesh.hpp>

#include <pscuProcessModel.hpp>
#include <pscuSurfaceModel.hpp>
#include <pscuTranslationField.hpp>

#include <psAdvectionCallback.hpp>
#include <psDomain.hpp>
#include <psLogger.hpp>
#include <psSmartPointer.hpp>
#include <psVelocityField.hpp>

#include <curtIndexMap.hpp>
#include <curtParticle.hpp>
#include <curtSmoothing.hpp>
#include <curtTracer.hpp>

#include <culsToSurfaceMesh.hpp>

template <typename NumericType, int D> class pscuProcess {
  using psDomainType = psSmartPointer<psDomain<NumericType, D>>;

public:
  pscuProcess(pscuContext passedContext) : context(passedContext) {}

  template <typename ProcessModelType>
  void setProcessModel(psSmartPointer<ProcessModelType> passedProcessModel) {
    model = std::dynamic_pointer_cast<pscuProcessModel<NumericType>>(
        passedProcessModel);
  }

  void setDomain(psDomainType passedDomain) { domain = passedDomain; }

  void setProcessDuration(double duration) { processDuration = duration; }

  void setNumberOfRaysPerPoint(long numRays) { raysPerPoint = numRays; }

  void setMaxCoverageInitIterations(long maxIt) { maxIterations = maxIt; }

  void setPeriodicBoundary(const int passedPeriodic) {
    periodicBoundary = static_cast<bool>(passedPeriodic);
  }

  void setSmoothFlux(bool pSmoothFlux) { smoothFlux = pSmoothFlux; }

  void apply() {
    /* ---------- Process Setup --------- */
    if (!model) {
      psLogger::getInstance()
          .addWarning("No process model passed to psProcess.")
          .print();
      return;
    }
    const auto name = model->getProcessName();

    if (!domain) {
      psLogger::getInstance()
          .addWarning("No domain passed to psProcess.")
          .print();
      return;
    }

    if (model->getGeometricModel()) {
      model->getGeometricModel()->setDomain(domain);
      psLogger::getInstance().addInfo("Applying geometric model...").print();
      model->getGeometricModel()->apply();
      return;
    }

    if (processDuration == 0.) {
      // apply only advection callback
      if (model->getAdvectionCallback()) {
        model->getAdvectionCallback()->setDomain(domain);
        model->getAdvectionCallback()->applyPreAdvect(0);
      } else {
        psLogger::getInstance()
            .addWarning("No advection callback passed to psProcess.")
            .print();
      }
      return;
    }

    if (!model->getSurfaceModel()) {
      psLogger::getInstance()
          .addWarning("No surface model passed to psProcess.")
          .print();
      return;
    }

    psUtils::Timer processTimer;
    processTimer.start();

    double remainingTime = processDuration;
    assert(domain->getLevelSets()->size() != 0 && "No level sets in domain.");
    const NumericType gridDelta =
        domain->getLevelSets()->back()->getGrid().getGridDelta();

    auto diskMesh = psSmartPointer<lsMesh<NumericType>>::New();
    lsToDiskMesh<NumericType, D> diskMeshConv(diskMesh);

    /* --------- Setup advection kernel ----------- */
    lsAdvect<NumericType, D> advectionKernel;
    advectionKernel.setIntegrationScheme(integrationScheme);
    advectionKernel.setIgnoreVoids(true);

    auto pointKdTree = psSmartPointer<
        psKDTree<NumericType, std::array<NumericType, 3>>>::New();
    if (model->getVelocityField()->getTranslationFieldOptions() > 0) {
      auto transField =
          psSmartPointer<pscuTranslationField<NumericType, true>>::New(
              model->getVelocityField(), pointKdTree, domain->getMaterialMap());
      advectionKernel.setVelocityField(transField);
    } else {
      auto transField =
          psSmartPointer<pscuTranslationField<NumericType, false>>::New(
              model->getVelocityField(), pointKdTree, domain->getMaterialMap());
      advectionKernel.setVelocityField(transField);
    }

    for (auto dom : *domain->getLevelSets()) {
      advectionKernel.insertNextLevelSet(dom);
      diskMeshConv.insertNextLevelSet(dom);
    }

    /* --------- Setup element-point translation ----------- */

    auto mesh = rayTrace.getSurfaceMesh();
    auto elementKdTree = psSmartPointer<
        psKDTree<NumericType, std::array<NumericType, 3>>>::New();
    rayTrace.setKdTree(elementKdTree);

    diskMeshConv.apply();

    pointKdTree->setPoints(diskMesh->nodes);
    pointKdTree->build();

    /* --------- Setup for ray tracing ----------- */
    const bool useRayTracing = model->getParticleTypes() != nullptr;
    unsigned int numRates = 0;
    unsigned int numCov = 0;
    typename curtIndexMap<NumericType>::indexMap fluxesIndexMap;

    if (useRayTracing && !rayTracerInitialized) {
      if (!model->getPtxCode()) {
        psLogger::getInstance()
            .addWarning("No pipeline in process model. Aborting.")
            .print();
        return;
      }
      rayTrace.setPipeline(model->getPtxCode());
      rayTrace.setLevelSet(domain);
      rayTrace.setNumberOfRaysPerPoint(raysPerPoint);
      rayTrace.setUseRandomSeed(useRandomSeeds);
      rayTrace.setPeriodicBoundary(periodicBoundary);
      for (auto &particle : *model->getParticleTypes()) {
        rayTrace.insertNextParticle(particle);
      }
      numRates = rayTrace.prepareParticlePrograms();
      fluxesIndexMap = curtIndexMap<NumericType>::getPointDataIndexMap(
          rayTrace.getParticles());
    }

    // Determine whether advection callback is used
    const bool useAdvectionCallback = model->getAdvectionCallback() != nullptr;
    if (useAdvectionCallback) {
      model->getAdvectionCallback()->setDomain(domain);
    }

    // Determine whether there are process parameters used in ray tracing
    model->getSurfaceModel()->initializeProcessParameters();
    const bool useProcessParams =
        model->getSurfaceModel()->getProcessParameters() != nullptr;

    if (useProcessParams)
      psLogger::getInstance().addInfo("Using process parameters.").print();
    if (useAdvectionCallback)
      psLogger::getInstance().addInfo("Using advection callback.").print();

    unsigned int numElements = 0;
    if (useRayTracing) {
      rayTrace.updateSurface(); // also creates mesh
      numElements = rayTrace.getNumberOfElements();
    }

    // Initialize coverages
    if (!coveragesInitialized)
      model->getSurfaceModel()->initializeCoverages(diskMesh->nodes.size());
    auto coverages = model->getSurfaceModel()->getCoverages(); // might be null
    const bool useCoverages = coverages != nullptr;

    if (useCoverages) {
      numCov = coverages->getScalarDataSize();
      rayTrace.setUseCellData(numCov + 1); // + material IDs

      psLogger::getInstance().addInfo("Using coverages.").print();

      if (!coveragesInitialized) {
        psUtils::Timer timer;
        psLogger::getInstance().addInfo("Initializing coverages ... ").print();

        timer.start();
        for (size_t iterations = 1; iterations <= maxIterations; iterations++) {

          // get coverages and material ids in ray tracer
          const auto &materialIds =
              *diskMesh->getCellData().getScalarData("Material");
          utCudaBuffer d_coverages; // device buffer
          translatePointToElementData(materialIds, coverages, d_coverages,
                                      pointKdTree, mesh);
          rayTrace.setCellData(d_coverages, numCov);

          // run the ray tracer
          rayTrace.apply();

          // extract fluxes on points
          auto fluxes = psSmartPointer<psPointData<NumericType>>::New();
          translateElementToPointData(rayTrace.getResults(), fluxes,
                                      fluxesIndexMap, elementKdTree, diskMesh,
                                      gridDelta);

          // calculate coverages
          model->getSurfaceModel()->updateCoverages(fluxes, materialIds);

          if (psLogger::getLogLevel() >= 3) {
            assert(numElements == mesh->triangles.size());

            downloadCoverages(d_coverages, mesh->getCellData(), coverages,
                              numElements);

            rayTrace.downloadResultsToPointData(mesh->getCellData());
            lsVTKWriter<NumericType>(
                mesh, name + "_covIinit_" + std::to_string(iterations) + ".vtp")
                .apply();
          }
          psLogger::getInstance()
              .addInfo("Iteration: " + std::to_string(iterations))
              .print();
        }
        coveragesInitialized = true;

        timer.finish();
        psLogger::getInstance()
            .addTiming("Coverage initialization", timer)
            .print();
      }
    }

    // // now build kd tree with mesh
    // rayTrace.setKdTree(kdTree);
    // rayTrace.updateSurface();

    // double previousTimeStep = 0.;
    // size_t counter = 0;
    // psUtils::Timer rtTimer;
    // psUtils::Timer callbackTimer;
    // psUtils::Timer advTimer;
    // while (remainingTime > 0.) {
    //   psLogger::getInstance()
    //       .addInfo("Remaining time: " + std::to_string(remainingTime))
    //       .print();

    //   auto &materialIds = *mesh->getCellData().getScalarData("Material");

    //   if (useRayTracing) {
    //     rtTimer.start();

    //     rayTrace.apply();

    //     rtTimer.finish();
    //     psLogger::getInstance()
    //         .addTiming("Top-down flux calculation", rtTimer)
    //         .print();
    //   }

    //   auto velocities = model->getSurfaceModel()->calculateVelocities(
    //       rayTrace.getResults(), materialIds);
    //   if (smoothFlux) {
    //     smoothVelocities(velocities, kdTree, gridDelta);
    //   }
    //   model->getVelocityField()->setVelocities(velocities);

    //   if (psLogger::getLogLevel() >= 4) {

    //     if (useCoverages)
    //       downloadCoverages(mesh->getCellData(), numElements);

    //     if (useRayTracing)
    //       rayTrace.downloadResultsToPointData(
    //           mesh->getCellData(), rayTrace.getResults(), numElements);

    //     if (velocities)
    //       mesh->getCellData().insertNextScalarData(*velocities,
    //       "velocities");

    //     lsVTKWriter<NumericType>(mesh, name + "_step_" +
    //                                        std::to_string(counter) + ".vtp")
    //         .apply();

    //     if (domain->getUseCellSet()) {
    //       domain->getCellSet()->writeVTU(name + "_cellSet_" +
    //                                      std::to_string(counter) + ".vtu");
    //     }

    //     counter++;
    //   }

    //   // apply advection callback
    //   if (useAdvectionCallback) {
    //     callbackTimer.start();
    //     bool continueProcess = model->getAdvectionCallback()->applyPreAdvect(
    //         processDuration - remainingTime);
    //     callbackTimer.finish();
    //     psLogger::getInstance()
    //         .addTiming("Advection callback pre-advect", callbackTimer)
    //         .print();

    //     if (!continueProcess) {
    //       psLogger::getInstance()
    //           .addInfo("Process stopped early by AdvectionCallback during"
    //                    "`preAdvect`.")
    //           .print();
    //       break;
    //     }
    //   }

    //   // adjust time step near end
    //   if (remainingTime - previousTimeStep < 0.) {
    //     advectionKernel.setAdvectionTime(remainingTime);
    //   }

    //   advTimer.start();
    //   advectionKernel.apply();
    //   advTimer.finish();

    //   psLogger::getInstance().addTiming("Surface advection",
    //   advTimer).print();

    //   // update surface and move coverages to new surface
    //   auto newKdTree =
    //       psSmartPointer<psKDTree<NumericType, std::array<float, 3>>>::New();
    //   rayTrace.setKdTree(newKdTree);
    //   rayTrace.updateSurface();
    //   if (useCoverages)
    //     moveCoverages(kdTree, newKdTree, numCov);
    //   kdTree = newKdTree;

    //   // apply advection callback
    //   if (useAdvectionCallback) {
    //     callbackTimer.start();
    //     bool continueProcess =
    //     model->getAdvectionCallback()->applyPostAdvect(
    //         advectionKernel.getAdvectedTime());
    //     callbackTimer.finish();
    //     psLogger::getInstance()
    //         .addTiming("Advection callback post-advect", callbackTimer)
    //         .print();
    //     if (!continueProcess) {
    //       psLogger::getInstance()
    //           .addInfo("Process stopped early by AdvectionCallback during "
    //                    "`postAdvect`.")
    //           .print();
    //       break;
    //     }
    //   }

    //   previousTimeStep = advectionKernel.getAdvectedTime();
    //   remainingTime -= previousTimeStep;
    // }

    // processTime = processDuration - remainingTime;
    // processTimer.finish();

    // psLogger::getInstance()
    //     .addTiming("\nProcess " + name, processTimer)
    //     .addTiming("Surface advection total time",
    //                advTimer.totalDuration * 1e-9,
    //                processTimer.totalDuration * 1e-9)
    //     .print();
    // if (useRayTracing) {
    //   psLogger::getInstance()
    //       .addTiming("Top-down flux calculation total time
    // }
    // if (useAdvectionCallback) {
    //   psLogger::getInstance()
    //       .addTiming("Advection callback total time",
    //                  callbackTimer.totalDuration * 1e-9,
    //                  processTimer.totalDuration * 1e-9)
    //       .print();
    // }
  }

private:
  //   void moveCoverages(
  //       psSmartPointer<psKDTree<NumericType, std::array<float, 3>>> kdTree,
  //       psSmartPointer<psKDTree<NumericType, std::array<float, 3>>>
  //       newkdTree, const unsigned numCov) {
  //     psPointData<NumericType> oldCoverages, newCoverages;
  //     downloadCoverages(oldCoverages, kdTree->getNumberOfPoints());

  //     // prepare new coverages
  //     std::vector<std::vector<NumericType>> newCovData(
  //         oldCoverages.getScalarDataSize());
  //     for (unsigned i = 0; i < oldCoverages.getScalarDataSize(); i++) {
  //       newCovData[i].resize(newkdTree->getNumberOfPoints());
  //     }

  //     // move coverages
  // #pragma omp parallel for
  //     for (std::size_t i = 0; i < newkdTree->getNumberOfPoints(); i++) {
  //       auto point = newkdTree->getPoint(i);
  //       auto nearest = kdTree->findNearest(point);

  //       for (unsigned j = 0;
  //            j <
  //            oldCoverages.getScalarDataSize
  //            // scalar data with offset
  //            for (unsigned j = 0; j < numData; j++) {
  //              elementData[i + (j + 1) * numElements] =
  //                  pointData->getScalarData(j)[closestPoint->first];
  //            }();
  //            j++) {
  //         newCovData[j][i] =
  //         oldCoverages.getScalarData(j)->at(nearest->first);
  //       }
  //     }

  //     for (unsigned i = 0; i < oldCoverages.getScalarDataSize(); i++) {
  //       auto label = oldCoverages.getScalarDataLabel(i);
  //       newCoverages.insertNextScalarData(std::move(newCovData[i]), label);
  //     }

  //     uploadCoverages(newCoverages);
  //   }

  void smoothVelocities(
      psSmartPointer<std::vector<NumericType>> velocities,
      psSmartPointer<psKDTree<NumericType, std::array<float, 3>>> kdTree,
      const NumericType gridDelta) {
    const auto numPoints = velocities->size();
    std::cout << numPoints << " " << kdTree->getNumberOfPoints() << std::endl;
    assert(numPoints == kdTree->getNumberOfPoints());

    std::vector<NumericType> smoothed(numPoints, 0.);

#pragma omp parallel for
    for (std::size_t i = 0; i < numPoints; ++i) {
      auto point = kdTree->getPoint(i);

      auto closePoints =
          kdTree->findNearestWithinRadius(point, smoothFlux * gridDelta);

      unsigned n = 0;
      for (auto p : closePoints.value()) {
        smoothed[i] += velocities->at(p.first);
        n++;
      }

      if (n > 1) {
        smoothed[i] /= static_cast<NumericType>(n);
      }
    }

    *velocities = std::move(smoothed);
  }

  void downloadCoverages(utCudaBuffer &d_coverages,
                         psPointData<NumericType> &elementData,
                         psSmartPointer<psPointData<NumericType>> &coverages,
                         unsigned int numElements) {

    auto numCov = coverages->getScalarDataSize();
    NumericType *temp = new NumericType[numElements * numCov];
    d_coverages.download(temp, numElements * numCov);

    for (unsigned i = 0; i < numCov; i++) {
      auto covName = coverages->getScalarDataLabel(i);
      auto cov = elementData.getScalarData(covName);
      if (cov == nullptr) {
        std::vector<NumericType> covInit(numElements);
        elementData.insertNextScalarData(std::move(covInit), covName);
        cov = elementData.getScalarData(covName);
      }
      if (cov->size() != numElements)
        cov->resize(numElements);
      std::memcpy(cov->data(), temp + i * numElements,
                  numElements * sizeof(NumericType));
    }

    delete temp;
  }

  // void uploadCoverages(psPointData<NumericType> &coverages) {
  //   std::vector<NumericType> flattenedCoverages;
  //   assert(coverages.getScalarData(0) != nullptr);
  //   const auto covSize = coverages.getScalarData(0)->size();
  //   const auto numCoverages = coverages.getScalarDataSize();
  //   flattenedCoverages.resize(covSize * numCoverages);

  //   auto covIndexMap = model->getSurfaceModel()->getCoverageIndexMap();
  //   for (auto &i : covIndexMap) {
  //     auto covData = coverages.getScalarData(i.first);
  //     std::memcpy(flattenedCoverages.data() + i.second * covSize,
  //                 covData->data(), covSize * sizeof(NumericType));
  //   }
  //   model->getSurfaceModel()->getCoverages().alloc_and_upload(
  //       flattenedCoverages);
  // }

  void translateElementToPointData(
      utCudaBuffer &d_elementData,
      psSmartPointer<psPointData<NumericType>> pointData,
      const typename curtIndexMap<NumericType>::indexMap &indexMap,
      psSmartPointer<psKDTree<float, std::array<float, 3>>> elementKdTree,
      psSmartPointer<lsMesh<float>> pointMesh, const NumericType gridDelta) {

    auto numData = indexMap.size();
    const auto &points = pointMesh->nodes;
    auto numPoints = points.size();
    auto numElements = elementKdTree->getNumberOfPoints();

    // retrieve data from device
    std::vector<NumericType> elementData(numData * numElements);
    d_elementData.download(elementData.data(), numData * numElements);

    // prepare point data
    pointData->clear();
    for (const auto &r : indexMap) {
      std::vector<NumericType> data(numPoints, 0.);
      pointData->insertNextScalarData(std::move(data), r.first);
      assert(pointData->getScalarDataSize() - 1 ==
             r.second); // assert correct order of data
    }
    assert(pointData->getScalarDataSize() == numData); // assert number of data

#pragma omp parallel for
    for (unsigned i = 0; i < numPoints; i++) {

      auto closePoints = elementKdTree->findNearestWithinRadius(
          points[i], smoothFlux * gridDelta);

      for (unsigned j = 0; j < numData; j++) {

        NumericType value = 0;
        unsigned n = 0;
        for (auto p : closePoints.value()) {
          value += elementData[p.first + j * numElements];
          n++;
        }

        if (n > 1)
          value /= static_cast<NumericType>(n);

        pointData->getScalarData(j)->at(i) = value;
      }
    }
  }

  static void translatePointToElementData(
      const std::vector<NumericType> &materialIds,
      psSmartPointer<psPointData<NumericType>> pointData,
      utCudaBuffer &d_elementData,
      psSmartPointer<psKDTree<NumericType, std::array<NumericType, 3>>>
          pointKdTree,
      psSmartPointer<lsMesh<float>> elementMesh) {

    auto numData = pointData ? pointData->getScalarDataSize() : 0;
    const auto &elements = elementMesh->template getElements<D>();
    auto numElements = elements.size();
    std::vector<NumericType> elementData((numData + 1) * numElements);

#pragma omp parallel for
    for (unsigned i = 0; i < numElements; i++) {
      auto &elIdx = elements[i];
      std::array<NumericType, 3> elementCenter{
          (elementMesh->nodes[elIdx[0]][0] + elementMesh->nodes[elIdx[1]][0] +
           elementMesh->nodes[elIdx[2]][0]) /
              3.f,
          (elementMesh->nodes[elIdx[0]][1] + elementMesh->nodes[elIdx[1]][1] +
           elementMesh->nodes[elIdx[2]][1]) /
              3.f,
          (elementMesh->nodes[elIdx[0]][2] + elementMesh->nodes[elIdx[1]][2] +
           elementMesh->nodes[elIdx[2]][2]) /
              3.f};

      auto closestPoint = pointKdTree->findNearest(elementCenter);

      // fill in material ids at front
      elementData[i] = materialIds[closestPoint->first];

      // scalar data with offset
      for (unsigned j = 0; j < numData; j++) {
        elementData[i + (j + 1) * numElements] =
            pointData->getScalarData(j)->at(closestPoint->first);
      }
    }

    d_elementData.alloc_and_upload(elementData);
  }

  pscuContext_t *context;
  curtTracer<NumericType, D> rayTrace = curtTracer<NumericType, D>(context);

  psDomainType domain = nullptr;
  psSmartPointer<pscuProcessModel<NumericType>> model = nullptr;
  lsIntegrationSchemeEnum integrationScheme =
      lsIntegrationSchemeEnum::ENGQUIST_OSHER_1ST_ORDER;
  NumericType processDuration;
  long raysPerPoint = 1000;
  bool useRandomSeeds = true;
  size_t maxIterations = 20;
  bool periodicBoundary = false;
  bool coveragesInitialized = false;
  bool rayTracerInitialized = false;
  NumericType smoothFlux = 1.;
  NumericType printTime = 0.;
  NumericType processTime = 0;
};
