#include <psExtrude.hpp>
#include <psFluorocarbonEtching.hpp>
#include <psMakeStack.hpp>
#include <psProcess.hpp>

#include "parameters.hpp"

int main(int argc, char *argv[]) {
  using NumericType = double;
  constexpr int D = 2;

  // set process verbosity
  psLogger::setLogLevel(psLogLevel::INTERMEDIATE);

  // Parse the parameters
  Parameters<NumericType> params;
  if (argc > 1) {
    auto config = psUtils::readConfigFile(argv[1]);
    if (config.empty()) {
      std::cerr << "Empty config provided" << std::endl;
      return -1;
    }
    params.fromMap(config);
  }

  // geometry setup
  auto geometry = psSmartPointer<psDomain<NumericType, D>>::New();
  psMakeStack<NumericType, D>(geometry, params.gridDelta, params.xExtent,
                              params.yExtent, params.numLayers,
                              params.layerHeight, params.substrateHeight, 0.0,
                              params.trenchWidth, params.maskHeight, false)
      .apply();

  // copy top layer for deposition
  geometry->duplicateTopLevelSet(psMaterial::Polymer);

  // use pre-defined model Fluorocarbon etching model
  auto model = psSmartPointer<psFluorocarbonEtching<NumericType, D>>::New(
      params.ionFlux, params.etchantFlux, params.polymerFlux, params.energyMean,
      params.energySigma);

  // process setup
  psProcess<NumericType, D> process;
  process.setDomain(geometry);
  process.setProcessModel(model);
  process.setProcessDuration(params.processTime);
  process.setMaxCoverageInitIterations(10);
  process.setTimeStepRatio(0.25);

  // print initial surface
  geometry->saveVolumeMesh("initial");

  process.apply();

  // print final surface
  geometry->saveVolumeMesh("final");

  std::cout << "Extruding to 3D ..." << std::endl;
  auto extruded = psSmartPointer<psDomain<NumericType, 3>>::New();
  std::array<NumericType, 2> extrudeExtent = {-20., 20.};
  psExtrude<NumericType>(geometry, extruded, extrudeExtent, 0,
                         {lsBoundaryConditionEnum<3>::REFLECTIVE_BOUNDARY,
                          lsBoundaryConditionEnum<3>::REFLECTIVE_BOUNDARY,
                          lsBoundaryConditionEnum<3>::INFINITE_BOUNDARY})
      .apply();

  extruded->saveSurfaceMesh("surface.vtp");
  geometry->saveVolumeMesh("final_extruded");
}