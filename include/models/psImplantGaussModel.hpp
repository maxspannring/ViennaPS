#pragma once

#include <psImplantModel.hpp>

template <typename NumericType, int D>
class psImplantGaussModel : public psImplantModel<NumericType, D> {

public:
  NumericType getDepthProfile(NumericType depth) override {
    // implement Gaussian profile here
    return 0.;
  }

  NumericType getLateralProfile(NumericType offset,
                                NumericType depth) override {
    // implement Gaussian profilel here
    return 0.;
  }

  NumericType getMaxDepth() override { return 10.; }
};