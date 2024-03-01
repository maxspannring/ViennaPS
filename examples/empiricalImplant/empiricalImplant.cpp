#include <psDomain.hpp>
#include <psImplant.hpp>
#include <psImplantGaussModel.hpp>
#include <psMakeTrench.hpp>
#include <psMakePlane.hpp>

#include "parameters.hpp"

int main(int argc, char **argv) {
  using NumericType = double;
  constexpr int D = 2;

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

  auto geometry = psSmartPointer<psDomain<NumericType, D>>::New();
  psMakeTrench<NumericType, D>(geometry, params.gridDelta, params.xExtent,
                               params.yExtent, params.trenchWidth,
                               params.trenchHeight, 0., 0., false, true,
                               psMaterial::Si)
      .apply();

  psMakePlane<NumericType, D>(geometry, 30, psMaterial::Si).apply();
  geometry->generateCellSet(0., psMaterial::Si, false);

  auto implantModel =
      psSmartPointer<psImplantGaussModel<NumericType, D>>::New();

  psImplant<NumericType, D> implantProcess;
  implantProcess.setDomain(geometry);
  implantProcess.setImplantModel(implantModel);
  implantProcess.setMaskMaterials(psMaterial::Mask);
  implantProcess.apply();

  geometry->getCellSet()->writeVTU("empiricalImplant.vtu");
}