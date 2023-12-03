#pragma once

#include <lsMaterialMap.hpp>
#include <psLogger.hpp>
#include <psSmartPointer.hpp>

#include <unordered_map>
// None = -1,
// Mask = 0,
// Si = 1,
// SiO2 = 2,
// Si3N4 = 3,
// SiN = 4,
// SiON = 5,
// SiC = 6,
// SiGe = 7,
// PolySi = 8,
// GaN = 9,
// W = 10,
// Al2O3 = 11,
// TiN = 12,
// Cu = 13,
// Polymer = 14,
// Dielectric = 15,
// Metal = 16,
// Air = 17,
// GAS = 18

template <class NumericType> struct psMaterial {
public:
  virtual int getId() const { return -1; }
  virtual NumericType getRate() const { return 0.; }
};

template <class NumericType> class psMaterialMap {
  psSmartPointer<lsMaterialMap> map;
  std::unordered_map<int, psSmartPointer<psMaterial<NumericType>>> materials;

public:
  psMaterialMap() { map = psSmartPointer<lsMaterialMap>::New(); };

  template <class MaterialType>
  void insertNextMaterial(psSmartPointer<MaterialType> material) {
    map->insertNextMaterial(material->getId());
    materials.insert(
        {material->getId(),
         std::dynamic_pointer_cast<psMaterial<NumericType>>(material)});
  }

  auto getMaterialAtIdx(std::size_t idx) const {
    if (idx >= size())
      return psSmartPointer<psMaterial<NumericType>>::New();
    auto matId = map->getMaterialId(idx);
    return getMaterial(matId);
  }

  template <class MaterialType>
  void setMaterialAtIdx(std::size_t idx,
                        psSmartPointer<MaterialType> material) {
    if (idx >= size()) {
      psLogger::getInstance()
          .addWarning("Setting material with out-of-bounds index.")
          .print();
    }
    map->setMaterialId(idx, material->getId());
    materials.insert(
        {material->getId(),
         std::dynamic_pointer_cast<psMaterial<NumericType>>(material)});
  }

  psSmartPointer<lsMaterialMap> getMaterialMap() const { return map; }

  inline std::size_t size() const { return map->getNumberOfLayers(); }

  auto getMaterial(const int matId) const {
    if (auto search = materials.find(matId); search != materials.end()) {
      return search->second;
    }
    psLogger::getInstance().addWarning("Can not get unknown material.").print();
    return psSmartPointer<psMaterial<NumericType>>::New();
  }

  template <class T> auto getMaterial(const T matId) const {
    return getMaterial(static_cast<int>(matId));
  }

  // template <class T>
  // static inline bool isMaterial(const T matId, const psMaterial material) {
  //   return mapToMaterial(matId) == material;
  // }
};
