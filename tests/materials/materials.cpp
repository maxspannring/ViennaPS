#include <psMaterials.hpp>
#include <psTestAssert.hpp>

template <class NumericType, int D> void psRunTest() {
  auto materialMap = psSmartPointer<psMaterialMap<NumericType>>::New();

  materialMap->insertNextMaterial({"mat1", 1.5, nullptr});
  PSTEST_ASSERT(materialMap->containsMaterial("mat1") >= 0);
  PSTEST_ASSERT(materialMap->getMaterialAtIdx(0).label == "mat1");

  materialMap->insertNextMaterial({"mat2", 2.5, nullptr});
  PSTEST_ASSERT(materialMap->containsMaterial("mat2") == 1);
  PSTEST_ASSERT(materialMap->getMaterialAtIdx(1).label == "mat2");

  materialMap->insertNextMaterial({"mat1", 1.5, nullptr});
  PSTEST_ASSERT(materialMap->containsMaterial("mat1") == 0);
  PSTEST_ASSERT(materialMap->getMaterialAtIdx(2).label == "mat1");

  materialMap->insertNextMaterial({"mat1", 1.5, nullptr});
  PSTEST_ASSERT(materialMap->containsMaterial("mat1") == 0);
  PSTEST_ASSERT(materialMap->getMaterialAtIdx(3).label == "mat1");

  materialMap->setMaterialAtIdx(3, {"mat3", 3.5, nullptr});
  PSTEST_ASSERT(materialMap->containsMaterial("mat3") == 2);
  PSTEST_ASSERT(materialMap->getMaterialAtIdx(3).label == "mat3");

  materialMap->print();
}

int main() { psRunTest<double, 2>(); }