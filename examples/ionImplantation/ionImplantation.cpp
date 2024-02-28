// this is my testing file for playing around with ViennaPS
#include <iostream>
#include <psDomain.hpp>
#include <psMakePlane.hpp>
#include <psProcess.hpp>
#include <psDirectionalEtching.hpp>


int main(){
    using NumericType = double;
    constexpr int D = 2;    // only in two dimensions for now
    psLogger::setLogLevel(psLogLevel::DEBUG);

    // creating the domain
    auto domain = psSmartPointer<psDomain<NumericType, D>>::New();

    // creating planar geometry
    psMakePlane<NumericType, D>(domain, 0.25, 5.0, 10.0, 15.0,
                                false, psMaterial::Si).apply();

    // outputting the level set mesh
    domain->saveLevelSetMesh("LS_mesh");

    // why does this only work with doubles tho?
    domain->generateCellSet(0., psMaterial::Si, false);


    // crystal surface direction
    const std::array<NumericType, 3> direction100 = {-0.707106781187,
                                                     -0.707106781187, 0.};
    const std::array<NumericType, 3> direction010 = {-0.707106781187,
                                                     0.707106781187, 0.};
    // create Process model
    auto model = psSmartPointer<psDirectionalEtching<NumericType, D>>::New(
            direction100,1, 0, psMaterial::Si
            );

    psProcess<NumericType, D> process;
    process.setDomain(domain);
    process.setProcessModel(model);
    process.setProcessDuration(10.);
    process.apply();
    domain -> saveSurfaceMesh("etchySurface.vtp");
}