#pragma once

#include <string>
#include <unordered_map>

#include <psUtils.hpp>

template <typename T> struct Parameters {
  // Domain
  T gridDelta = 1.;
  T xExtent = 60.;
  T yExtent = 600.;

  // Geometry
  T trenchWidth = 20.;
  T trenchHeight = 4.;

  // Process
  Parameters() {}

  void fromMap(std::unordered_map<std::string, std::string> &m) {
    psUtils::AssignItems(                           //
        m,                                          //
        psUtils::Item{"gridDelta", gridDelta},      //
        psUtils::Item{"xExtent", xExtent},          //
        psUtils::Item{"yExtent", yExtent},          //
        psUtils::Item{"trenchWidth", trenchWidth},  //
        psUtils::Item{"trenchHeight", trenchHeight} //
    );
  }
};
