#include <psMaterials.hpp>
#include <psTestAssert.hpp>

#include <iostream>

template <class NumericType>
struct myMaterial : public psMaterial<NumericType> {
public:
  int getId() const override { return 1; }
  NumericType getRate() const override { return 1.; }
};

template <class NumericType, int D> void psRunTest() {
  auto materialMap = psSmartPointer<psMaterialMap<NumericType>>::New();

  materialMap->insertNextMaterial(
      psSmartPointer<myMaterial<NumericType>>::New());

  PSTEST_ASSERT(materialMap->getMaterialAtIdx(0)->getId() == 1);
  PSTEST_ASSERT(materialMap->getMaterial(1)->getId() == 1);
}

int main() { psRunTest<double, 2>(); }