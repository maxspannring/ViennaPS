#ifndef PS_DOMAIN_HPP
#define PS_DOMAIN_HPP

#include <lsBooleanOperation.hpp>
#include <lsDomain.hpp>
#include <lsMakeGeometry.hpp>
#include <lsToDiskMesh.hpp>
#include <lsToSurfaceMesh.hpp>
#include <lsVTKWriter.hpp>
#include <lsWriter.hpp>

#include <csDenseCellSet.hpp>

#include <psPointValuesToLevelSet.hpp>
#include <psSmartPointer.hpp>

/**
  This class represents all materials in the simulation domain.
  It contains level sets for the accurate surface representation
  and a cell-based structure for the storage of volume information.
  These structures are used depending on the process applied to the material.
  Processes may use one of either structures or both.
*/
template <class NumericType = float, int D = 3> class psDomain {
public:
  typedef psSmartPointer<lsDomain<NumericType, D>> lsDomainType;
  typedef psSmartPointer<std::vector<lsDomainType>> lsDomainsType;
  typedef psSmartPointer<csDenseCellSet<NumericType, D>> csDomainType;
  typedef std::pair<std::string, int> psMatMapping;

  static constexpr char materialIdsLabel[] = "MaterialIds";

private:
  lsDomainsType levelSets = nullptr;
  lsDomainType wrappingLevelSet = nullptr;
  csDomainType cellSet = nullptr;
  std::multimap<std::string, int> materialIdMapping;
  bool useCellSet = false;
  NumericType cellSetDepth = 0.;

public:
  psDomain(bool passedUseCellSet = false) : useCellSet(passedUseCellSet) {
    levelSets = lsDomainsType::New();
    if (useCellSet) {
      cellSet = csDomainType::New();
    }
  }

  psDomain(lsDomainType passedLevelSet,
           std::string material = "defaultMaterial",
           bool passedUseCellSet = false, const NumericType passedDepth = 0.,
           const bool passedCellSetPosition = false)
      : useCellSet(passedUseCellSet), cellSetDepth(passedDepth) {

    wrappingLevelSet = lsDomainType::New(passedLevelSet);
    levelSets = lsDomainsType::New();
    levelSets->push_back(passedLevelSet);
    materialIdMapping.insert(psMatMapping(material, 0));

    // generate CellSet
    if (useCellSet) {
      cellSet =
          csDomainType::New(levelSets, cellSetDepth, passedCellSetPosition);
    }
  }

  // psDomain(lsDomainsType passedLevelSets, bool passedUseCellSet = false,
  //          const NumericType passedDepth = 0.,
  //          const bool passedCellSetPosition = false)
  //     : useCellSet(passedUseCellSet), cellSetDepth(passedDepth) {
  //   levelSets = passedLevelSets;
  //   wrappingLevelSet = lsDomainType::New(levelSets->front());
  //   for (size_t i = 1; i < levelSets->size(); i++) {
  //     lsBooleanOperation<NumericType, D>(wrappingLevelSet, levelSets->at(i),
  //                                        lsBooleanOperationEnum::UNION)
  //         .apply();
  //   }
  //   // generate CellSet
  //   if (useCellSet) {
  //     cellSet =
  //         csDomainType::New(levelSets, cellSetDepth, passedCellSetPosition);
  //   }
  // }

  // void deepCopy(psSmartPointer<psDomain> passedDomain) {
  //   levelSets->resize(passedDomain->levelSets->size());
  //   for (unsigned i = 0; i < levelSets->size(); ++i) {
  //     levelSets[i]->deepCopy(passedDomain->levelSets[i]);
  //   }
  //   useCellSet = passedDomain->useCellSet;
  //   if (useCellSet) {
  //     cellSetDepth = passedDomain->cellSetDepth;
  //     cellSet->fromLevelSets(passedDomain->levelSets, cellSetDepth);
  //   }
  // }

  void insertNextLevelSet(lsDomainType passedLevelSet,
                          std::string material = "defaultMaterial") {
    if (!wrappingLevelSet) {
      wrappingLevelSet = lsDomainType::New(passedLevelSet);
    } else {
      lsBooleanOperation<NumericType, D>(wrappingLevelSet, passedLevelSet,
                                         lsBooleanOperationEnum::UNION)
          .apply();
    }
    levelSets->push_back(passedLevelSet);
    materialIdMapping.insert({material, levelSets->size() - 1});
  }

  void removeLevelSet(const size_t idx) {
    levelSets->erase(levelSets->begin() + idx);
    wrappingLevelSet = lsDomainType::New(levelSets->front());
    for (size_t i = 1; i < levelSets->size(); i++) {
      lsBooleanOperation<NumericType, D>(wrappingLevelSet, levelSets->at(i),
                                         lsBooleanOperationEnum::UNION)
          .apply();
    }

    for (auto it = materialIdMapping.begin(); it != materialIdMapping.end();
         ++it) {
      if (it->second == idx) {
#ifdef VIENNAPS_VERBOSE
        std::cout << "Removing material " << it->first << " on layer " << idx
                  << " from domain." << std::endl;
#endif
        materialIdMapping.erase(it);
        break;
      }
    }
  }

  void removeMaterial(std::string material) {
    std::vector<int> removeLayers;

    for (auto it = materialIdMapping.begin(); it != materialIdMapping.end();) {
      if (it->first == material) {
#ifdef VIENNAPS_VERBOSE
        std::cout << "Removing material " << it->first << " on layer "
                  << it->second << " from domain." << std::endl;
#endif
        removeLayers.push_back(it->second);
        it = materialIdMapping.erase(it);
      } else {
        ++it;
      }
    }
    std::sort(removeLayers.begin(), removeLayers.end());

    for (auto it = removeLayers.crbegin(); it != removeLayers.crend(); ++it) {
      levelSets->erase(levelSets->begin() + *it);
    }

    if (!levelSets->empty()) {
      wrappingLevelSet = lsDomainType::New(levelSets->front());
      for (size_t i = 1; i < levelSets->size(); i++) {
        lsBooleanOperation<NumericType, D>(wrappingLevelSet, levelSets->at(i),
                                           lsBooleanOperationEnum::UNION)
            .apply();
      }
    }
  }

  int getMaskMaterialId(std::string maskMaterial = "mask") const {
    int maskId = -1;

    for (auto itr = materialIdMapping.cbegin(); itr != materialIdMapping.cend();
         itr++) {
      if (itr->first == maskMaterial) {
        if (maskId == -1) {
          maskId = itr->second;
        } else {
          lsMessage::getInstance()
              .addWarning("Found multiple mask materials " +
                          std::to_string(itr->second) +
                          ". Using only first one: " + std::to_string(maskId))
              .print();
        }
      }
    }

    if (maskId == -1) {
      lsMessage::getInstance().addWarning("No mask material found.").print();
    }

    return maskId;
  }

  void generateCellSet(const NumericType depth = 0.,
                       const bool passedCellSetPosition = false) {
    useCellSet = true;
    cellSetDepth = depth;
    if (cellSet == nullptr) {
      cellSet = csDomainType::New();
    }
    cellSet->setCellSetPosition(passedCellSetPosition);
    cellSet->fromLevelSets(levelSets, cellSetDepth);
  }

  auto &getWrappingLevelSet() { return wrappingLevelSet; }

  auto &getLevelSets() { return levelSets; }

  auto &getCellSet() { return cellSet; }

  auto &getGrid() { return wrappingLevelSet->getGrid(); }

  void setUseCellSet(bool useCS) { useCellSet = useCS; }

  bool getUseCellSet() { return useCellSet; }

  void print() {
    std::cout << "Process Simulation Domain:" << std::endl;
    std::cout << "**************************" << std::endl;
    for (auto &ls : *levelSets) {
      ls->print();
    }
    std::cout << "**************************" << std::endl;
  }

  void printSurface(std::string name, bool addMaterialIds = false) {
    auto mesh = psSmartPointer<lsMesh<NumericType>>::New();

    if (addMaterialIds) {
      auto translator = psSmartPointer<
          std::unordered_map<unsigned long, unsigned long>>::New();
      lsToDiskMesh<NumericType, D> meshConverter;
      meshConverter.setMesh(mesh);
      meshConverter.setTranslator(translator);
      for (const auto ls : *levelSets) {
        meshConverter.insertNextLevelSet(ls);
      }
      meshConverter.insertNextLevelSet(wrappingLevelSet);
      meshConverter.apply();
      auto matIds = mesh->getCellData().getScalarData(materialIdsLabel);
      if (matIds && matIds->size() == wrappingLevelSet->getNumberOfPoints())
        psPointValuesToLevelSet<NumericType, D>(wrappingLevelSet, translator,
                                                matIds, "Material")
            .apply();
      else
        std::cout << "Scalar data '" << materialIdsLabel
                  << "' not found in mesh cellData.\n";
    }

    lsToSurfaceMesh<NumericType, D>(wrappingLevelSet, mesh).apply();
    lsVTKWriter<NumericType>(mesh, name).apply();
  }

  void writeLevelSets(std::string fileName) {
    for (int i = 0; i < levelSets->size(); i++) {
      lsWriter<NumericType, D>(
          levelSets->at(i), fileName + "_layer" + std::to_string(i) + ".lvst")
          .apply();
    }
  }

  void clear() {
    wrappingLevelSet = nullptr;
    levelSets = lsDomainsType::New();
    if (useCellSet) {
      cellSet = csDomainType::New();
    }
  }

private:
  int internalMaterialIdCounter = 0;
};

#endif // PS_DOMAIN_HPP
