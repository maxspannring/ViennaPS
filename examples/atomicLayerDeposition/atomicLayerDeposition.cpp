#include <psAtomicLayerModel.hpp>
#include <psDomain.hpp>
#include <psMakeTrench.hpp>

#include "parameters.hpp"

int main(int argc, char *argv[]) {
  constexpr int D = 2;
  using NumericType = double;
  omp_set_num_threads(16);

  // Parse the parameters
  Parameters params;
  if (argc > 1) {
    params.readConfigFile(argv[1]);
  } else {
    std::cout << "Usage: " << argv[0] << " <config file>" << std::endl;
    return 1;
  }

  // Create a domain
  auto domain = psSmartPointer<psDomain<NumericType, D>>::New();
  psMakeTrench<NumericType, D>(
      domain, params.get("gridDelta"), params.get("xExtent"),
      params.get("yExtent"), params.get("trenchWidth"),
      params.get("trenchHeight"), params.get("taperAngle"), 0., false, false,
      psMaterial::Si)
      .apply();
  domain->generateCellSet(params.get("trenchHeight") + params.get("topSpace"),
                          psMaterial::GAS, true);
  auto &cellSet = domain->getCellSet();

  psAtomicLayerModel<NumericType, D> model(domain, "H2O", "TMA");

  double time = 0.;
  int i = 0;
  while (time < params.get("p1_time")) {
    time += model.timeStep(params.get("D_p1"), params.get("H2O_adsorptionRate"),
                           params.get("H2O_desorptionRate"), 1., false);
    if (time - i > 0.1) {
      cellSet->writeVTU("ALD_" + std::to_string(i++) + ".vtu");
      std::cout << "P1 Time: " << time << std::endl;
    }
  }

  time = 0.;
  int j = 0;
  while (time < params.get("purge_time")) {
    time +=
        model.timeStep(params.get("D_purge"), params.get("H2O_adsorptionRate"),
                       params.get("H2O_desorptionRate"), 0., false);
    if (time - j > 0.1) {
      cellSet->writeVTU("ALD_" + std::to_string(i++) + ".vtu");
      std::cout << "Purge Time: " << time << std::endl;
      ++j;
    }
  }

  time = 0.;
  int k = 0;
  {
    auto flux = cellSet->getScalarData("Flux");
    std::fill(flux->begin(), flux->end(), 0.);
  }
  while (time < params.get("p2_time")) {
    time += model.timeStep(params.get("D_p2"), params.get("TMA_adsorptionRate"),
                           params.get("TMA_desorptionRate"), 1., true);
    if (time - k > 0.1) {
      cellSet->writeVTU("ALD_" + std::to_string(i++) + ".vtu");
      std::cout << "P2 Time: " << time << std::endl;
      ++k;
    }
  }
}