/*
  This file is used to generate the python module of viennaps.
  It uses pybind11 to create the modules.

  All necessary headers are included here and the interface
  of the classes which should be exposed defined
*/

#define PYBIND11_DETAILED_ERROR_MESSAGES

// correct module name macro
#define TOKENPASTE_INTERNAL(x, y, z) x##y##z
#define TOKENPASTE(x, y, z) TOKENPASTE_INTERNAL(x, y, z)
#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)
#define VIENNAPS_MODULE_VERSION STRINGIZE(VIENNAPS_VERSION)

#include <optional>
#include <vector>

#include <pybind11/iostream.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

// all header files which define API functions
#include <psAdvectionCallback.hpp>
#include <psDomain.hpp>
#include <psGDSGeometry.hpp>
#include <psGDSReader.hpp>
#include <psPlanarize.hpp>
#include <psProcess.hpp>
#include <psProcessModel.hpp>
#include <psSurfaceModel.hpp>

// geometries
#include <psMakeFin.hpp>
#include <psMakeHole.hpp>
#include <psMakePlane.hpp>
#include <psMakeStack.hpp>
#include <psMakeTrench.hpp>

// visualization
#include <psToDiskMesh.hpp>
#include <psToSurfaceMesh.hpp>
#include <psWriteVisualizationMesh.hpp>

// models
#include <DirectionalEtching.hpp>
#include <FluorocarbonEtching.hpp>
#include <GeometricDistributionModels.hpp>
#include <IsotropicProcess.hpp>
#include <PlasmaDamage.hpp>
#include <SF6O2Etching.hpp>
#include <SimpleDeposition.hpp>
#include <WetEtching.hpp>

// CellSet
#include <csDenseCellSet.hpp>

// Compact
#include <psKDTree.hpp>

// other
#include <lsDomain.hpp>
#include <rayParticle.hpp>
#include <rayReflection.hpp>
#include <rayTraceDirection.hpp>
#include <rayUtil.hpp>

// always use double for python export
typedef double T;
typedef std::vector<hrleCoordType> VectorHRLEcoord;
// get dimension from cmake define
constexpr int D = VIENNAPS_PYTHON_DIMENSION;
typedef psSmartPointer<psDomain<T, D>> DomainType;

PYBIND11_DECLARE_HOLDER_TYPE(Types, psSmartPointer<Types>)
PYBIND11_MAKE_OPAQUE(std::vector<T, std::allocator<T>>)

PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>)

// define trampoline classes for interface functions
// ALSO NEED TO ADD TRAMPOLINE CLASSES FOR CLASSES
// WHICH HOLD REFERENCES TO INTERFACE(ABSTRACT) CLASSES

class PypsSurfaceModel : public psSurfaceModel<T> {
  using psSurfaceModel<T>::Coverages;
  using psSurfaceModel<T>::processParams;
  using psSurfaceModel<T>::getCoverages;
  using psSurfaceModel<T>::getProcessParameters;
  typedef std::vector<T> vect_type;

public:
  void initializeCoverages(unsigned numGeometryPoints) override {
    PYBIND11_OVERRIDE(void, psSurfaceModel<T>, initializeCoverages,
                      numGeometryPoints);
  }

  void initializeProcessParameters() override {
    PYBIND11_OVERRIDE(void, psSurfaceModel<T>, initializeProcessParameters, );
  }

  psSmartPointer<std::vector<T>>
  calculateVelocities(psSmartPointer<psPointData<T>> Rates,
                      const std::vector<std::array<T, 3>> &coordinates,
                      const std::vector<T> &materialIds) override {
    PYBIND11_OVERRIDE(psSmartPointer<std::vector<T>>, psSurfaceModel<T>,
                      calculateVelocities, Rates, coordinates, materialIds);
  }

  void updateCoverages(psSmartPointer<psPointData<T>> Rates,
                       const std::vector<T> &materialIds) override {
    PYBIND11_OVERRIDE(void, psSurfaceModel<T>, updateCoverages, Rates,
                      materialIds);
  }
};

// psAdvectionCallback
class PyAdvectionCallback : public psAdvectionCallback<T, D> {
protected:
  using ClassName = psAdvectionCallback<T, D>;

public:
  using ClassName::domain;

  bool applyPreAdvect(const T processTime) override {
    PYBIND11_OVERRIDE(bool, ClassName, applyPreAdvect, processTime);
  }

  bool applyPostAdvect(const T advectionTime) override {
    PYBIND11_OVERRIDE(bool, ClassName, applyPostAdvect, advectionTime);
  }
};

// Particle Class
template <int D> class psParticle : public rayParticle<psParticle<D>, T> {
  using ClassName = rayParticle<psParticle<D>, T>;

public:
  psParticle(T pSticking, T pPower)
      : stickingProbability(pSticking), sourcePower(pPower) {}
  void surfaceCollision(T rayWeight, const rayTriple<T> &rayDir,
                        const rayTriple<T> &geomNormal,
                        const unsigned int primID, const int materialID,
                        rayTracingData<T> &localData,
                        const rayTracingData<T> *globalData,
                        rayRNG &Rng) override final {
    PYBIND11_OVERRIDE(void, ClassName, surfaceCollision, rayWeight, rayDir,
                      geomNormal, primID, materialID, localData, globalData,
                      Rng);
  }

  std::pair<T, rayTriple<T>>
  surfaceReflection(T rayWeight, const rayTriple<T> &rayDir,
                    const rayTriple<T> &geomNormal, const unsigned int primID,
                    const int materialID, const rayTracingData<T> *globalData,
                    rayRNG &Rng) override final {
    using Pair = std::pair<T, rayTriple<T>>;
    PYBIND11_OVERRIDE(Pair, ClassName, surfaceReflection, rayWeight, rayDir,
                      geomNormal, primID, materialID, globalData, Rng);
  }

  void initNew(rayRNG &RNG) override final {
    PYBIND11_OVERRIDE(void, ClassName, initNew, RNG);
  }

  int getRequiredLocalDataSize() const override final {
    PYBIND11_OVERRIDE(int, ClassName, getRequiredLocalDataSize);
  }

  T getSourceDistributionPower() const override final {
    PYBIND11_OVERRIDE(T, ClassName, getSourceDistributionPower);
  }

  std::vector<std::string> getLocalDataLabels() const override final {
    PYBIND11_OVERRIDE(std::vector<std::string>, ClassName, getLocalDataLabels);
  }

private:
  T stickingProbability = 0.2;
  T sourcePower = 1.0;
};

// psProcessModel
class PyProcessModel : public psProcessModel<T, D> {
public:
  using psProcessModel<T, D>::psProcessModel;
  using psProcessModel<T, D>::setProcessName;
  using psProcessModel<T, D>::getProcessName;
  using psProcessModel<T, D>::setSurfaceModel;
  using psProcessModel<T, D>::setAdvectionCallback;
  using psProcessModel<T, D>::setGeometricModel;
  using psProcessModel<T, D>::getParticleLogSize;
  using ParticleTypeList = std::vector<std::unique_ptr<rayAbstractParticle<T>>>;
  psSmartPointer<ParticleTypeList> particles = nullptr;
  std::vector<int> particleLogSize;
  psSmartPointer<psSurfaceModel<T>> surfaceModel = nullptr;
  psSmartPointer<psAdvectionCallback<T, D>> advectionCallback = nullptr;
  psSmartPointer<psGeometricModel<T, D>> geometricModel = nullptr;
  psSmartPointer<psVelocityField<T>> velocityField = nullptr;
  std::string processName = "default";
  using ClassName = psProcessModel<T, D>;

  psSmartPointer<psSurfaceModel<T>> getSurfaceModel() override {

    PYBIND11_OVERRIDE(psSmartPointer<psSurfaceModel<T>>, ClassName,
                      getSurfaceModel);
  }

  psSmartPointer<psAdvectionCallback<T, D>> getAdvectionCallback() override {
    using SmartPointerAdvectionCalBack_TD =
        psSmartPointer<psAdvectionCallback<T, D>>;
    PYBIND11_OVERRIDE(SmartPointerAdvectionCalBack_TD, ClassName,
                      getAdvectionCallback, );
  }
  psSmartPointer<psGeometricModel<T, D>> getGeometricModel() override {
    using SmartPointerGeometricModel_TD =
        psSmartPointer<psGeometricModel<T, D>>;
    PYBIND11_OVERRIDE(SmartPointerGeometricModel_TD, ClassName,
                      getGeometricModel, );
  }
  psSmartPointer<psVelocityField<T>> getVelocityField() {
    PYBIND11_OVERRIDE(psSmartPointer<psVelocityField<T>>, ClassName,
                      getVelocityField, );
  }
};

// psVelocityField
class PyVelocityField : public psVelocityField<T> {
  using psVelocityField<T>::psVelocityField;

public:
  T getScalarVelocity(const std::array<T, 3> &coordinate, int material,
                      const std::array<T, 3> &normalVector,
                      unsigned long pointId) override {
    PYBIND11_OVERRIDE(T, psVelocityField<T>, getScalarVelocity, coordinate,
                      material, normalVector, pointId);
  }
  // if we declare a typedef for std::array<T,3>, we will no longer get this
  // error: the compiler doesn't understand why std::array gets 2 template
  // arguments
  // add template argument as the preprocessor becomes confused with the comma
  // in std::array<T, 3>
  typedef std::array<T, 3> arrayType;
  std::array<T, 3> getVectorVelocity(const std::array<T, 3> &coordinate,
                                     int material,
                                     const std::array<T, 3> &normalVector,
                                     unsigned long pointId) override {
    PYBIND11_OVERRIDE(
        arrayType, // add template argument here, as the preprocessor becomes
                   // confused with the comma in std::array<T, 3>
        psVelocityField<T>, getVectorVelocity, coordinate, material,
        normalVector, pointId);
  }

  T getDissipationAlpha(int direction, int material,
                        const std::array<T, 3> &centralDifferences) override {
    PYBIND11_OVERRIDE(T, psVelocityField<T>, getDissipationAlpha, direction,
                      material, centralDifferences);
  }
  void setVelocities(psSmartPointer<std::vector<T>> passedVelocities) override {
    PYBIND11_OVERRIDE(void, psVelocityField<T>, setVelocities,
                      passedVelocities);
  }
  int getTranslationFieldOptions() const override {
    PYBIND11_OVERRIDE(int, psVelocityField<T>, getTranslationFieldOptions, );
  }
};

// a function to declare GeometricDistributionModel of type DistType
template <typename NumericType, int D, typename DistType>
void declare_GeometricDistributionModel(pybind11::module &m,
                                        const std::string &typestr) {
  using Class = GeometricDistributionModel<NumericType, D, DistType>;

  pybind11::class_<Class, psSmartPointer<Class>>(m, typestr.c_str())
      .def(pybind11::init<psSmartPointer<DistType>>(), pybind11::arg("dist"))
      .def(pybind11::init<psSmartPointer<DistType>,
                          psSmartPointer<lsDomain<NumericType, D>>>(),
           pybind11::arg("dist"), pybind11::arg("mask"))
      .def("apply", &Class::apply);
}

PYBIND11_MODULE(VIENNAPS_MODULE_NAME, module) {
  module.doc() =
      "ViennaPS is a header-only C++ process simulation library, which "
      "includes surface and volume representations, a ray tracer, and physical "
      "models for the simulation of microelectronic fabrication processes. The "
      "main design goals are simplicity and efficiency, tailored towards "
      "scientific simulations.";

  // set version string of python module
  module.attr("__version__") = VIENNAPS_MODULE_VERSION;

  // wrap omp_set_num_threads to control number of threads
  module.def("setNumThreads", &omp_set_num_threads);

  // it was giving an error that it couldnt convert this type to python
  // pybind11::bind_vector<std::vector<double, std::allocator<double>>>(
  //     module, "VectorDouble");

  // psProcessParams
  pybind11::class_<psProcessParams<T>, psSmartPointer<psProcessParams<T>>>(
      module, "psProcessParams")
      .def(pybind11::init<>())
      .def("insertNextScalar", &psProcessParams<T>::insertNextScalar)
      .def("getScalarData", (T & (psProcessParams<T>::*)(int)) &
                                psProcessParams<T>::getScalarData)
      .def("getScalarData", (const T &(psProcessParams<T>::*)(int) const) &
                                psProcessParams<T>::getScalarData)
      .def("getScalarData", (T & (psProcessParams<T>::*)(std::string)) &
                                psProcessParams<T>::getScalarData)
      .def("getScalarDataIndex", &psProcessParams<T>::getScalarDataIndex)
      .def("getScalarData", (std::vector<T> & (psProcessParams<T>::*)()) &
                                psProcessParams<T>::getScalarData)
      .def("getScalarData",
           (const std::vector<T> &(psProcessParams<T>::*)() const) &
               psProcessParams<T>::getScalarData)
      .def("getScalarDataLabel", &psProcessParams<T>::getScalarDataLabel);

  // psSurfaceModel
  pybind11::class_<psSurfaceModel<T>, psSmartPointer<psSurfaceModel<T>>,
                   PypsSurfaceModel>(module, "psSurfaceModel")
      .def(pybind11::init<>())
      .def("initializeCoverages", &psSurfaceModel<T>::initializeCoverages)
      .def("initializeProcessParameters",
           &psSurfaceModel<T>::initializeProcessParameters)
      .def("getCoverages", &psSurfaceModel<T>::getCoverages)
      .def("getProcessParameters", &psSurfaceModel<T>::getProcessParameters)
      .def("calculateVelocities", &psSurfaceModel<T>::calculateVelocities)
      .def("updateCoverages", &psSurfaceModel<T>::updateCoverages);

  pybind11::enum_<psLogLevel>(module, "psLogLevel")
      .value("ERROR", psLogLevel::ERROR)
      .value("WARNING", psLogLevel::WARNING)
      .value("INFO", psLogLevel::INFO)
      .value("TIMING", psLogLevel::TIMING)
      .value("INTERMEDIATE", psLogLevel::INTERMEDIATE)
      .value("DEBUG", psLogLevel::DEBUG)
      .export_values();

  // some unexpected behaviour can happen as it is working with multithreading
  pybind11::class_<psLogger, psSmartPointer<psLogger>>(module, "psLogger")
      .def_static("setLogLevel", &psLogger::setLogLevel)
      .def_static("getLogLevel", &psLogger::getLogLevel)
      .def_static("getInstance", &psLogger::getInstance,
                  pybind11::return_value_policy::reference)
      .def("addDebug", &psLogger::addDebug)
      .def("addTiming", (psLogger & (psLogger::*)(std::string, double)) &
                            psLogger::addTiming)
      .def("addTiming",
           (psLogger & (psLogger::*)(std::string, double, double)) &
               psLogger::addTiming)
      .def("addInfo", &psLogger::addInfo)
      .def("addWarning", &psLogger::addWarning)
      .def("addError", &psLogger::addError, pybind11::arg("s"),
           pybind11::arg("shouldAbort") = true)
      .def("print", [](psLogger &instance) { instance.print(std::cout); });

  // psVelocityField
  pybind11::class_<psVelocityField<T>, psSmartPointer<psVelocityField<T>>,
                   PyVelocityField>(module, "psVelocityField")
      // constructors
      .def(pybind11::init<>())
      // methods
      .def("getScalarVelocity", &psVelocityField<T>::getScalarVelocity)
      .def("getVectorVelocity", &psVelocityField<T>::getVectorVelocity)
      .def("getDissipationAlpha", &psVelocityField<T>::getDissipationAlpha)
      .def("getTranslationFieldOptions",
           &psVelocityField<T>::getTranslationFieldOptions)
      .def("setVelocities", &psVelocityField<T>::setVelocities);

  // psDomain
  pybind11::class_<psDomain<T, D>, DomainType>(module, "psDomain")
      // constructors
      .def(pybind11::init<bool>())
      .def(pybind11::init(&DomainType::New<>))
      // methods
      .def("insertNextLevelSet", &psDomain<T, D>::insertNextLevelSet,
           pybind11::arg("levelset"), pybind11::arg("wrapLowerLevelSet") = true,
           "Insert a level set to domain.")
      .def("insertNextLevelSetAsMaterial",
           &psDomain<T, D>::insertNextLevelSetAsMaterial)
      .def("duplicateTopLevelSet", &psDomain<T, D>::duplicateTopLevelSet)
      .def("setMaterialMap", &psDomain<T, D>::setMaterialMap)
      .def("getMaterialMap", &psDomain<T, D>::getMaterialMap)
      .def("generateCellSet", &psDomain<T, D>::generateCellSet,
           "Generate the cell set.")
      .def("getLevelSets",
           [](psDomain<T, D> &d)
               -> std::optional<std::vector<psSmartPointer<lsDomain<T, D>>>> {
             auto levelsets = d.getLevelSets();
             if (levelsets)
               return *levelsets;
             return std::nullopt;
           })
      .def("getCellSet", &psDomain<T, D>::getCellSet, "Get the cell set.")
      .def("getGrid", &psDomain<T, D>::getGrid, "Get the grid")
      .def("setUseCellSet", &psDomain<T, D>::setUseCellSet)
      .def("getUseCellSet", &psDomain<T, D>::getUseCellSet)
      .def("print", &psDomain<T, D>::print)
      .def("printSurface", &psDomain<T, D>::printSurface,
           pybind11::arg("filename"), pybind11::arg("addMaterialIds") = false,
           "Print the surface of the domain.")
      .def("writeLevelSets", &psDomain<T, D>::writeLevelSets)
      .def("clear", &psDomain<T, D>::clear);

  // Enum psMaterial
  pybind11::enum_<psMaterial>(module, "psMaterial")
      .value("Undefined", psMaterial::Undefined)
      .value("Mask", psMaterial::Mask)
      .value("Si", psMaterial::Si)
      .value("SiO2", psMaterial::SiO2)
      .value("Si3N4", psMaterial::Si3N4)
      .value("SiN", psMaterial::SiN)
      .value("SiON", psMaterial::SiON)
      .value("SiC", psMaterial::SiC)
      .value("PolySi", psMaterial::PolySi)
      .value("GaN", psMaterial::GaN)
      .value("W", psMaterial::W)
      .value("Al2O3", psMaterial::Al2O3)
      .value("TiN", psMaterial::TiN)
      .value("Cu", psMaterial::Cu)
      .value("Polymer", psMaterial::Polymer)
      .value("Dielectric", psMaterial::Dielectric)
      .value("Metal", psMaterial::Metal)
      .value("Air", psMaterial::Air)
      .value("GAS", psMaterial::GAS)
      .export_values();

  // psMaterialMap
  pybind11::class_<psMaterialMap, psSmartPointer<psMaterialMap>>(
      module, "psMaterialMap")
      .def(pybind11::init<>())
      .def("insertNextMaterial", &psMaterialMap::insertNextMaterial,
           pybind11::arg("material") = psMaterial::Undefined)
      .def("getMaterialAtIdx", &psMaterialMap::getMaterialAtIdx)
      .def("getMaterialMap", &psMaterialMap::getMaterialMap)
      .def("size", &psMaterialMap::size)
      .def_static("mapToMaterial", &psMaterialMap::mapToMaterial<T>,
                  "Map a float to a material.")
      .def_static("isMaterial", &psMaterialMap::isMaterial<T>);

  // Shim to instantiate the particle class
  // pybind11::class_<psParticle<D>, psSmartPointer<psParticle<D>>>(module,
  //                                                                "psParticle")
  //     // constructors
  //     .def(pybind11::init<T, T>())
  //     // methods
  //     .def("surfaceCollision",
  //          [](psParticle<D> &p, T rayWeight) {
  //            rayTracingData<T> rtData;
  //            rayTriple<T> triple;
  //            rayRNG rng;
  //            p.surfaceCollision(rayWeight, triple, triple, 0, 0, rtData,
  //                               nullptr, rng);
  //          })
  //     .def("surfaceReflection", [](psParticle<D> &p, T rayWeight) {
  //       rayTracingData<T> rtData;
  //       rayRNG rng;
  //       rayTriple<T> triple;
  //       p.surfaceReflection(rayWeight, triple, triple, 0, 0, nullptr, rng);
  //     });

  /****************************************************************************
   *                               VISUALIZATION                              *
   ****************************************************************************/

  // pybind11::class_<psToDiskMesh<T, D>, psSmartPointer<psToDiskMesh<T, D>>>(
  //     module, "psToDiskMesh")
  //     .def(pybind11::init(&psSmartPointer<psToDiskMesh<T, D>>::New<
  //                             DomainType, psSmartPointer<lsMesh<T>>>),
  //                         pybind11::arg("domain"), pybind11::arg("mesh"))
  //     .def(pybind11::init<>());

  pybind11::class_<psWriteVisualizationMesh<T, D>>(module,
                                                   "psWriteVisualizationMesh")
      .def(pybind11::init<DomainType, std::string>(), pybind11::arg("domain"),
           pybind11::arg("fileName"))
      .def("apply", &psWriteVisualizationMesh<T, D>::apply);

  /****************************************************************************
   *                               MODELS                                     *
   ****************************************************************************/

  // psProcessModel
  pybind11::class_<psProcessModel<T, D>, psSmartPointer<psProcessModel<T, D>>,
                   PyProcessModel>
      processModel(module, "psProcessModel");

  // constructors
  processModel
      .def(pybind11::init<>())
      // methods
      .def("setProcessName", &psProcessModel<T, D>::setProcessName)
      .def("getProcessName", &psProcessModel<T, D>::getProcessName)
      .def("getSurfaceModel", &psProcessModel<T, D>::getSurfaceModel)
      .def("getAdvectionCallback", &psProcessModel<T, D>::getAdvectionCallback)
      .def("getGeometricModel", &psProcessModel<T, D>::getGeometricModel)
      .def("getVelocityField", &psProcessModel<T, D>::getVelocityField)
      .def("getParticleLogSize", &psProcessModel<T, D>::getParticleLogSize)
      .def("getParticleTypes",
           [](psProcessModel<T, D> &pm) {
             // Get smart pointer to vector of unique_ptr from the process
             // model
             auto unique_ptrs_sp = pm.getParticleTypes();

             // Dereference the smart pointer to access the vector
             auto &unique_ptrs = *unique_ptrs_sp;

             // Create vector to hold shared_ptr
             std::vector<std::shared_ptr<rayAbstractParticle<T>>> shared_ptrs;

             // Loop over unique_ptrs and create shared_ptrs from them
             for (auto &uptr : unique_ptrs) {
               shared_ptrs.push_back(
                   std::shared_ptr<rayAbstractParticle<T>>(uptr.release()));
             }

             // Return the new vector of shared_ptr
             return shared_ptrs;
           })
      .def("setSurfaceModel",
           [](psProcessModel<T, D> &pm, psSmartPointer<psSurfaceModel<T>> &sm) {
             pm.setSurfaceModel(sm);
           })
      .def("setAdvectionCallback",
           [](psProcessModel<T, D> &pm,
              psSmartPointer<psAdvectionCallback<T, D>> &ac) {
             pm.setAdvectionCallback(ac);
           })
      .def("insertNextParticleType",
           [](psProcessModel<T, D> &pm,
              psSmartPointer<psParticle<D>> &passedParticle) {
             if (passedParticle) {
               auto particle =
                   std::make_unique<psParticle<D>>(*passedParticle.get());
               pm.insertNextParticleType(particle);
             }
           })
      // IMPORTANT: here it may be needed to write this function for any
      // type of passed Particle
      .def("setGeometricModel",
           [](psProcessModel<T, D> &pm,
              psSmartPointer<psGeometricModel<T, D>> &gm) {
             pm.setGeometricModel(gm);
           })
      .def("setVelocityField", [](psProcessModel<T, D> &pm,
                                  psSmartPointer<psVelocityField<T>> &vf) {
        pm.setVelocityField<psVelocityField<T>>(vf);
      });

  // psProcess
  pybind11::class_<psProcess<T, D>, psSmartPointer<psProcess<T, D>>>(
      module, "psProcess")
      // constructors
      .def(pybind11::init(&psSmartPointer<psProcess<T, D>>::New<>))

      // methods
      .def("setDomain", &psProcess<T, D>::setDomain, "Set the process domain.")
      .def("setProcessDuration", &psProcess<T, D>::setProcessDuration,
           "Set the process duraction.")
      .def("setSourceDirection", &psProcess<T, D>::setSourceDirection,
           "Set source direction of the process.")
      .def("setNumberOfRaysPerPoint", &psProcess<T, D>::setNumberOfRaysPerPoint,
           "Set the number of rays to traced for each particle in the process. "
           "The number is per point in the process geometry")
      .def("setMaxCoverageInitIterations",
           &psProcess<T, D>::setMaxCoverageInitIterations,
           "Set the number of iterations to initialize the coverages.")
      .def("setPrintTimeInterval", &psProcess<T, D>::setPrintTimeInterval,
           "Sets the minimum time between printing intermediate results during "
           "the process. If this is set to a non-positive value, no "
           "intermediate results are printed.")
      .def("setProcessModel",
           &psProcess<T, D>::setProcessModel<psProcessModel<T, D>>,
           "Set the process model.")
      .def("apply", &psProcess<T, D>::apply, "Run the process.")
      .def("setIntegrationScheme", &psProcess<T, D>::setIntegrationScheme,
           "Set the integration scheme for solving the level-set equation. "
           "Should be used out of the ones specified in "
           "lsIntegrationSchemeEnum.")
      .def("setTimeStepRatio", &psProcess<T, D>::setTimeStepRatio,
           "Set the CFL condition to use during advection. The CFL condition "
           "sets the maximum distance a surface can be moved during one "
           "advection step. It MUST be below 0.5 to guarantee numerical "
           "stability. Defaults to 0.4999.");

  // psAdvectionCallback
  pybind11::class_<psAdvectionCallback<T, D>,
                   psSmartPointer<psAdvectionCallback<T, D>>,
                   PyAdvectionCallback>(module, "psAdvectionCallback")
      // constructors
      .def(pybind11::init<>())
      // methods
      .def("applyPreAdvect", &psAdvectionCallback<T, D>::applyPreAdvect)
      .def("applyPostAdvect", &psAdvectionCallback<T, D>::applyPostAdvect)
      .def_readwrite("domain", &PyAdvectionCallback::domain);

  // enums
  pybind11::enum_<rayTraceDirection>(module, "rayTraceDirection")
      .value("POS_X", rayTraceDirection::POS_X)
      .value("POS_Y", rayTraceDirection::POS_Y)
      .value("POS_Z", rayTraceDirection::POS_Z)
      .value("NEG_X", rayTraceDirection::NEG_X)
      .value("NEG_Y", rayTraceDirection::NEG_Y)
      .value("NEG_Z", rayTraceDirection::NEG_Z);

  /****************************************************************************
   *                               GEOMETRIES                                 *
   ****************************************************************************/

  // constructors with custom enum need lambda to work: seems to be an issue
  // with implicit move constructor

  // psMakePlane
  pybind11::class_<psMakePlane<T, D>, psSmartPointer<psMakePlane<T, D>>>(
      module, "psMakePlane")
      .def(pybind11::init([](DomainType Domain, const T GridDelta,
                             const T XExtent, const T YExtent, const T Height,
                             const bool Periodic, const psMaterial Material) {
             return psSmartPointer<psMakePlane<T, D>>::New(
                 Domain, GridDelta, XExtent, YExtent, Height, Periodic,
                 Material);
           }),
           pybind11::arg("psDomain"), pybind11::arg("gridDelta"),
           pybind11::arg("xExtent"), pybind11::arg("yExtent"),
           pybind11::arg("height") = 0.,
           pybind11::arg("periodicBoundary") = false,
           pybind11::arg("material") = psMaterial::Undefined)
      .def(pybind11::init(
               [](DomainType Domain, T Height, const psMaterial Material) {
                 return psSmartPointer<psMakePlane<T, D>>::New(Domain, Height,
                                                               Material);
               }),
           pybind11::arg("psDomain"), pybind11::arg("height") = 0.,
           pybind11::arg("material") = psMaterial::Undefined)
      .def("apply", &psMakePlane<T, D>::apply,
           "Create a plane geometry or add plane to existing geometry.");

  // psMakeTrench
  pybind11::class_<psMakeTrench<T, D>, psSmartPointer<psMakeTrench<T, D>>>(
      module, "psMakeTrench")
      .def(pybind11::init([](DomainType Domain, const T GridDelta,
                             const T XExtent, const T YExtent,
                             const T TrenchWidth, const T TrenchDepth,
                             const T TaperingAngle, const T BaseHeight,
                             const bool PeriodicBoundary, const bool MakeMask,
                             const psMaterial Material) {
             return psSmartPointer<psMakeTrench<T, D>>::New(
                 Domain, GridDelta, XExtent, YExtent, TrenchWidth, TrenchDepth,
                 TaperingAngle, BaseHeight, PeriodicBoundary, MakeMask,
                 Material);
           }),
           pybind11::arg("psDomain"), pybind11::arg("gridDelta"),
           pybind11::arg("xExtent"), pybind11::arg("yExtent"),
           pybind11::arg("trenchWidth"), pybind11::arg("trenchDepth"),
           pybind11::arg("taperingAngle") = 0.,
           pybind11::arg("baseHeight") = 0.,
           pybind11::arg("periodicBoundary") = false,
           pybind11::arg("makeMask") = false,
           pybind11::arg("material") = psMaterial::Undefined)
      .def("apply", &psMakeTrench<T, D>::apply, "Create a trench geometry.");

  // psMakeHole
  pybind11::class_<psMakeHole<T, D>, psSmartPointer<psMakeHole<T, D>>>(
      module, "psMakeHole")
      .def(pybind11::init([](DomainType domain, const T GridDelta,
                             const T xExtent, const T yExtent,
                             const T HoleRadius, const T HoleDepth,
                             const T TaperingAngle, const T BaseHeight,
                             const bool PeriodicBoundary, const bool MakeMask,
                             const psMaterial material) {
             return psSmartPointer<psMakeHole<T, D>>::New(
                 domain, GridDelta, xExtent, yExtent, HoleRadius, HoleDepth,
                 TaperingAngle, BaseHeight, PeriodicBoundary, MakeMask,
                 material);
           }),
           pybind11::arg("psDomain"), pybind11::arg("gridDelta"),
           pybind11::arg("xExtent"), pybind11::arg("yExtent"),
           pybind11::arg("holeRadius"), pybind11::arg("holeDepth"),
           pybind11::arg("taperingAngle") = 0.,
           pybind11::arg("baseHeight") = 0.,
           pybind11::arg("periodicBoundary") = false,
           pybind11::arg("makeMask") = false,
           pybind11::arg("material") = psMaterial::Undefined)
      .def("apply", &psMakeHole<T, D>::apply, "Create a hole geometry.");

  // psMakeFin
  pybind11::class_<psMakeFin<T, D>, psSmartPointer<psMakeFin<T, D>>>(
      module, "psMakeFin")
      .def(pybind11::init([](DomainType Domain, const T gridDelta,
                             const T xExtent, const T yExtent, const T FinWidth,
                             const T FinHeight, const T BaseHeight,
                             const bool PeriodicBoundary, const bool MakeMask,
                             const psMaterial material) {
             return psSmartPointer<psMakeFin<T, D>>::New(
                 Domain, gridDelta, xExtent, yExtent, FinWidth, FinHeight,
                 BaseHeight, PeriodicBoundary, MakeMask, material);
           }),
           pybind11::arg("psDomain"), pybind11::arg("gridDelta"),
           pybind11::arg("xExtent"), pybind11::arg("yExtent"),
           pybind11::arg("finWidth"), pybind11::arg("finHeight"),
           pybind11::arg("baseHeight") = 0.,
           pybind11::arg("periodicBoundary") = false,
           pybind11::arg("makeMask") = false,
           pybind11::arg("material") = psMaterial::Undefined)
      .def("apply", &psMakeFin<T, D>::apply, "Create a fin geometry.");

  // psMakeStack
  pybind11::class_<psMakeStack<T, D>, psSmartPointer<psMakeStack<T, D>>>(
      module, "psMakeStack")
      .def(pybind11::init(
               &psSmartPointer<psMakeStack<T, D>>::New<
                   DomainType &, const T /*gridDelta*/, const T /*xExtent*/,
                   const T /*yExtent*/, const int /*numLayers*/,
                   const T /*layerHeight*/, const T /*substrateHeight*/,
                   const T /*holeRadius*/, const T /*maskHeight*/,
                   const bool /*PeriodicBoundary*/>),
           pybind11::arg("psDomain"), pybind11::arg("gridDelta"),
           pybind11::arg("xExtent"), pybind11::arg("yExtent"),
           pybind11::arg("numLayers"), pybind11::arg("layerHeight"),
           pybind11::arg("substrateHeight"), pybind11::arg("holeRadius"),
           pybind11::arg("maskHeight"),
           pybind11::arg("periodicBoundary") = false)
      .def("apply", &psMakeStack<T, D>::apply,
           "Create a stack of alternating SiO2 and Si3N4 layers.")
      .def("getTopLayer", &psMakeStack<T, D>::getTopLayer,
           "Returns the number of layers included in the stack")
      .def("getHeight", &psMakeStack<T, D>::getHeight,
           "Returns the total height of the stack.");

  /****************************************************************************
   *                               MODELS                                     *
   ****************************************************************************/
  // Simple Deposition
  pybind11::class_<SimpleDeposition<T, D>,
                   psSmartPointer<SimpleDeposition<T, D>>>(
      module, "SimpleDeposition", processModel)
      .def(pybind11::init(
               &psSmartPointer<SimpleDeposition<T, D>>::New<const T, const T>),
           pybind11::arg("stickingProbability") = 0.1,
           pybind11::arg("sourceExponent") = 1.);

  // SF6O2 Etching
  pybind11::class_<SF6O2Etching<T, D>, psSmartPointer<SF6O2Etching<T, D>>>(
      module, "SF6O2Etching", processModel)
      .def(pybind11::init(
               &psSmartPointer<SF6O2Etching<T, D>>::New<
                   const double /*ionFlux*/, const double /*etchantFlux*/,
                   const double /*oxygenFlux*/, const T /*meanIonEnergy*/,
                   const T /*sigmaIonEnergy*/, const T /*ionExponent*/,
                   const T /*oxySputterYield*/, const T /*etchStopDepth*/>),
           pybind11::arg("ionFlux"), pybind11::arg("etchantFlux"),
           pybind11::arg("oxygenFlux"), pybind11::arg("meanIonEnergy") = 100.,
           pybind11::arg("sigmaIonEnergy") = 10.,
           pybind11::arg("ionExponent") = 100.,
           pybind11::arg("oxySputterYield") = 3.,
           pybind11::arg("etchStopDepth") = std::numeric_limits<T>::lowest());

  // Fluorocarbon Etching
  pybind11::class_<FluorocarbonEtching<T, D>,
                   psSmartPointer<FluorocarbonEtching<T, D>>>(
      module, "FluorocarbonEtching", processModel)
      .def(
          pybind11::init(&psSmartPointer<FluorocarbonEtching<T, D>>::New<
                         const double /*ionFlux*/, const double /*etchantFlux*/,
                         const double /*polyFlux*/, T /*meanEnergy*/,
                         const T /*sigmaEnergy*/, const T /*ionExponent*/,
                         const T /*deltaP*/, const T /*etchStopDepth*/>),
          pybind11::arg("ionFlux"), pybind11::arg("etchantFlux"),
          pybind11::arg("polyFlux"), pybind11::arg("meanIonEnergy") = 100.,
          pybind11::arg("sigmaIonEnergy") = 10.,
          pybind11::arg("ionExponent") = 100., pybind11::arg("deltaP") = 0.,
          pybind11::arg("etchStopDepth") = std::numeric_limits<T>::lowest());

  // Isotropic Process
  pybind11::class_<IsotropicProcess<T, D>,
                   psSmartPointer<IsotropicProcess<T, D>>>(
      module, "IsotropicProcess", processModel)
      .def(pybind11::init(
               &psSmartPointer<IsotropicProcess<T, D>>::New<const double,
                                                            const int>),
           pybind11::arg("isotropic rate"), pybind11::arg("maskId") = -1);

  // Directional Etching
  pybind11::class_<DirectionalEtching<T, D>,
                   psSmartPointer<DirectionalEtching<T, D>>>(
      module, "DirectionalEtching", processModel)
      .def(
          pybind11::init(
              &psSmartPointer<DirectionalEtching<T, D>>::New<
                  const std::array<T, 3> &, const T, const T, const int>),
          pybind11::arg("direction"), pybind11::arg("directionalVelocity") = 1.,
          pybind11::arg("isotropicVelocity") = 0., pybind11::arg("maskId") = 0);

  // Sphere Distribution
  pybind11::class_<SphereDistribution<T, D>,
                   psSmartPointer<SphereDistribution<T, D>>>(
      module, "SphereDistribution", processModel)
      .def(pybind11::init([](const T radius, const T gridDelta,
                             psSmartPointer<lsDomain<T, D>> mask = nullptr) {
        return psSmartPointer<SphereDistribution<T, D>>::New(radius, gridDelta,
                                                             mask);
      }));

  // Box Distribution
  pybind11::class_<BoxDistribution<T, D>,
                   psSmartPointer<BoxDistribution<T, D>>>(
      module, "BoxDistribution", processModel)
      .def(
          pybind11::init([](const std::array<T, 3> &halfAxes, const T gridDelta,
                            psSmartPointer<lsDomain<T, D>> mask = nullptr) {
            return psSmartPointer<BoxDistribution<T, D>>::New(halfAxes,
                                                              gridDelta, mask);
          }));

  // Plasma Damage
  pybind11::class_<PlasmaDamage<T, D>, psSmartPointer<PlasmaDamage<T, D>>>(
      module, "PlasmaDamage", processModel)
      .def(pybind11::init(
               &psSmartPointer<PlasmaDamage<T, D>>::New<const T, const T,
                                                        const int>),
           pybind11::arg("ionEnergy") = 100.,
           pybind11::arg("meanFreePath") = 1.,
           pybind11::arg("maskMaterial") = 0);

  pybind11::class_<psPlanarize<T, D>, psSmartPointer<psPlanarize<T, D>>>(
      module, "psPlanarize")
      .def(pybind11::init(
               &psSmartPointer<psPlanarize<T, D>>::New<DomainType &, const T>),
           pybind11::arg("geometry"), pybind11::arg("cutoffHeight") = 0.)
      .def("apply", &psPlanarize<T, D>::apply, "Apply the planarization.");

#if VIENNAPS_PYTHON_DIMENSION > 2
  pybind11::class_<WetEtching<T, D>, psSmartPointer<WetEtching<T, D>>>(
      module, "WetEtching", processModel)
      .def(pybind11::init(&psSmartPointer<WetEtching<T, D>>::New<const int>),
           pybind11::arg("maskId") = 0);

  // GDS file parsing
  pybind11::class_<psGDSGeometry<T, D>, psSmartPointer<psGDSGeometry<T, D>>>(
      module, "psGDSGeometry")
      // constructors
      .def(pybind11::init(&psSmartPointer<psGDSGeometry<T, D>>::New<>))
      .def(pybind11::init(&psSmartPointer<psGDSGeometry<T, D>>::New<const T>),
           pybind11::arg("gridDelta"))
      // methods
      .def("setGridDelta", &psGDSGeometry<T, D>::setGridDelta,
           "Set the grid spacing.")
      .def(
          "setBoundaryConditions",
          [](psGDSGeometry<T, D> &gds,
             std::vector<typename lsDomain<T, D>::BoundaryType> &bcs) {
            if (bcs.size() == D)
              gds.setBoundaryConditions(bcs.data());
          },
          "Set the boundary conditions")
      .def("setBoundaryPadding", &psGDSGeometry<T, D>::setBoundaryPadding,
           "Set padding between the largest point of the geometry and the "
           "boundary of the domain.")
      .def("print", &psGDSGeometry<T, D>::print, "Print the geometry contents.")
      .def("layerToLevelSet", &psGDSGeometry<T, D>::layerToLevelSet,
           "Convert a layer of the GDS geometry to a level set domain.")
      .def(
          "getBounds",
          [](psGDSGeometry<T, D> &gds) -> std::array<double, 6> {
            auto b = gds.getBounds();
            std::array<double, 6> bounds;
            for (unsigned i = 0; i < 6; ++i)
              bounds[i] = b[i];
            return bounds;
          },
          "Get the bounds of the geometry.");

  pybind11::class_<psGDSReader<T, D>, psSmartPointer<psGDSReader<T, D>>>(
      module, "psGDSReader")
      // constructors
      .def(pybind11::init(&psSmartPointer<psGDSReader<T, D>>::New<>))
      .def(pybind11::init(&psSmartPointer<psGDSReader<T, D>>::New<
                          psSmartPointer<psGDSGeometry<T, D>> &, std::string>))
      // methods
      .def("setGeometry", &psGDSReader<T, D>::setGeometry,
           "Set the domain to be parsed in.")
      .def("setFileName", &psGDSReader<T, D>::setFileName,
           "Set name of the GDS file.")
      .def("apply", &psGDSReader<T, D>::apply, "Parse the GDS file.");
#endif

  // rayReflection.hpp
  module.def("rayReflectionSpecular", &rayReflectionSpecular<T>,
             "Specular reflection,");
  module.def("rayReflectionDiffuse", &rayReflectionDiffuse<T, D>,
             "Diffuse reflection.");
  module.def("rayReflectionConedCosine", &rayReflectionConedCosine<T, D>,
             "Coned cosine reflection.");
}