#pragma once

template <typename NumericType, int D> class psImplantModel {

public:
  virtual NumericType getDepthProfile(NumericType depth) {
    // profile of empirical implant model in z direction
    return 0.;
  }

  virtual NumericType getLateralProfile(NumericType offset, NumericType depth) {
    // profile of empirical implant model in lateral directions
    return 0.;
  }

  virtual NumericType getMaxDepth() {
    // maximum depth of the implant
    return 0.;
  }
};