#pragma once

#include <psDomain.hpp>
#include <psImplantModel.hpp>
#include <psLogger.hpp>

template <class NumericType, int D> class psImplant {
  psSmartPointer<psDomain<NumericType, D>> domain;
  psSmartPointer<psImplantModel<NumericType, D>> model;
  std::vector<psMaterial> maskMaterials;

public:
  psImplant() = default;

  void setDomain(psSmartPointer<psDomain<NumericType, D>> passedDomain) {
    domain = passedDomain;
  }

  template <typename ImplantModelType>
  void setImplantModel(psSmartPointer<ImplantModelType> passedImplantModel) {
    model = std::dynamic_pointer_cast<psImplantModel<NumericType, D>>(
        passedImplantModel);
  }

  template <class... Mats> void setMaskMaterials(Mats... mats) {
    maskMaterials = {mats...};
  }

  void apply() {
    if (!model) {
      psLogger::getInstance()
          .addWarning("No implant model passed to psImplant.")
          .print();
      return;
    }
    if (!domain) {
      psLogger::getInstance()
          .addWarning("No domain passed to psImplant.")
          .print();
      return;
    }
    if (!domain->getCellSet()) {
      psLogger::getInstance()
          .addWarning("No cellSet in domain passed to psImplant.")
          .print();
      return;
    }

    // apply the implant model to the domain
    psLogger& logger = psLogger::getInstance(); // get reference to logger instance
    lsSmartPointer<csDenseCellSet<NumericType, D>> cellSet =  domain->getCellSet();
    domain->generateCellSet(0, psMaterial::Si, false);
    logger.addInfo("Ion Implantation Process started!").print();
    size_t number = cellSet->getNumberOfCells();
    logger.addInfo("number of cells: "  + std::to_string(number)).print();

    std::vector<std::array<unsigned, (1<<D)>> elements = domain->getCellSet() -> getElements();
    cellSet->setFillingFraction(5, true);
    for (int i=0;i<number;i++) {
        cellSet->setFillingFraction(i, (float) i / (number));
        //logger.addInfo(std::to_string(1.0 / sqrt(i)));
    }
    logger.addInfo("Ion implantation finished").print();
  }
};