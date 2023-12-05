#pragma once

#include <lsMaterialMap.hpp>
#include <psLogger.hpp>
#include <psSmartPointer.hpp>
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
  std::string label = "Undefined";
  NumericType rate = 0.;
  void *data = nullptr;
};

template <class NumericType> class psMaterialMap {
  psSmartPointer<lsMaterialMap> map;
  std::vector<psMaterial<NumericType>> uniqueMaterials;

public:
  psMaterialMap() { map = psSmartPointer<lsMaterialMap>::New(); };

  void insertNextMaterial(psMaterial<NumericType> material) {
    auto id = containsMaterial(material.label);
    if (id < 0) {
      uniqueMaterials.push_back(material);
      id = static_cast<int>(uniqueMaterials.size()) - 1;
    }
    map->insertNextMaterial(id);
  }

  int containsMaterial(std::string material) {
    for (int idx = 0; idx < uniqueMaterials.size(); idx++) {
      if (uniqueMaterials[idx].label == material)
        return idx;
    }
    return -1;
  }

  auto getMaterialAtIdx(std::size_t idx) const {
    if (idx >= size()) {
      psLogger::getInstance()
          .addError("Accessing material with out-of-bounds index.")
          .print();
    }
    auto id = map->getMaterialId(idx);
    return uniqueMaterials[id];
  }

  void setMaterialAtIdx(std::size_t idx, psMaterial<NumericType> material) {
    if (idx >= size()) {
      psLogger::getInstance()
          .addError("Setting material with out-of-bounds index.")
          .print();
    }
    int id = containsMaterial(material.label);
    if (id < 0) {
      uniqueMaterials.push_back(material);
      id = static_cast<int>(uniqueMaterials.size()) - 1;
    }
    map->setMaterialId(idx, id);
  }

  psSmartPointer<lsMaterialMap> getMaterialMap() const { return map; }

  inline std::size_t size() const { return map->getNumberOfLayers(); }

  inline auto getMaterial(const int id) const {
    if (id >= uniqueMaterials.size()) {
      psLogger::getInstance().addError("Can not get unknown material.").print();
    }
    return uniqueMaterials[id];
  }

  auto getMaterial(const std::string searchLabel) const {
    int id = containsMaterial({0., searchLabel, nullptr});
    if (id < 0) {
      psLogger::getInstance().addError("Can not get unknown material.").print();
    }
    return uniqueMaterials[id];
  }

  void print() const {
    std::cout << std::setprecision(4);
    std::cout << "Materials:\nID\t\tLabel\t\t\t\tRate\t\tData\n";
    for (int idx = 0; idx < uniqueMaterials.size(); idx++) {
      std::cout << idx << "\t\t" << uniqueMaterials[idx].label << "\t\t\t\t"
                << uniqueMaterials[idx].rate << "\t\t";
      if (uniqueMaterials[idx].data) {
        std::cout << "Yes\n";
      } else {
        std::cout << "No\n";
      }
    }
  }
};
