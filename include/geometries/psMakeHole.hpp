#pragma once

#include <lsBooleanOperation.hpp>
#include <lsDomain.hpp>
#include <lsMakeGeometry.hpp>

#include <psDomain.hpp>
#include <psMakeTrench.hpp>
#include <psMaterials.hpp>

/// Generates new a hole geometry in the z direction, which, in 2D mode,
/// corresponds to a trench geometry. Positioned at the origin, the hole is
/// centered, with the total extent defined in the x and y directions. The
/// normal direction for the hole creation is in the positive z direction in 3D
/// and the positive y direction in 2D. Users can specify the hole's radius,
/// depth, and opt for tapering with a designated angle. The hole configuration
/// may include periodic boundaries in both the x and y directions.
/// Additionally, the hole can serve as a mask, with the specified material only
/// applied to the bottom of the hole, while the remainder adopts the mask
/// material.
template <class NumericType, int D> class psMakeHole {
  using LSPtrType = psSmartPointer<lsDomain<NumericType, D>>;
  using psDomainType = psSmartPointer<psDomain<NumericType, D>>;

  psDomainType domain = nullptr;

  const NumericType gridDelta;
  const NumericType xExtent;
  const NumericType yExtent;
  const NumericType baseHeight = 0.;

  const NumericType holeRadius;
  const NumericType holeDepth;
  const NumericType taperAngle = 0; // tapering angle in degrees
  const bool makeMask = true;
  const bool periodicBoundary = false;

  const psMaterial material = psMaterial::None;

public:
  psMakeHole(psDomainType passedDomain, const NumericType passedGridDelta,
             const NumericType passedXExtent, const NumericType passedYExtent,
             const NumericType passedHoleRadius,
             const NumericType passedHoleDepth,
             const NumericType passedTaperAngle = 0.,
             const NumericType passedBaseHeight = 0.,
             const bool passedPeriodicBoundary = false,
             const bool passedMakeMask = false,
             const psMaterial passedMaterial = psMaterial::None)
      : domain(passedDomain), gridDelta(passedGridDelta),
        xExtent(passedXExtent), yExtent(passedYExtent),
        holeRadius(passedHoleRadius), holeDepth(passedHoleDepth),
        taperAngle(passedTaperAngle), baseHeight(passedBaseHeight),
        periodicBoundary(passedPeriodicBoundary), makeMask(passedMakeMask),
        material(passedMaterial) {}

  void apply() {
    if (D != 3) {
      psLogger::getInstance()
          .addWarning("psMakeHole: Hole geometry can only be created in 3D! "
                      "Falling back to trench geometry.")
          .print();
      psMakeTrench<NumericType, D>(
          domain, gridDelta, xExtent, yExtent, 2 * holeRadius, holeDepth,
          taperAngle, baseHeight, periodicBoundary, makeMask, material)
          .apply();

      return;
    }
    domain->clear();
    double bounds[2 * D];
    bounds[0] = -xExtent / 2.;
    bounds[1] = xExtent / 2.;

    if constexpr (D == 3) {
      bounds[2] = -yExtent / 2.;
      bounds[3] = yExtent / 2.;
      bounds[4] = baseHeight - gridDelta;
      bounds[5] = baseHeight + holeDepth + gridDelta;
    } else {
      bounds[2] = baseHeight - gridDelta;
      bounds[3] = baseHeight + holeDepth + gridDelta;
    }

    typename lsDomain<NumericType, D>::BoundaryType boundaryCons[D];

    for (int i = 0; i < D - 1; i++) {
      if (periodicBoundary) {
        boundaryCons[i] =
            lsDomain<NumericType, D>::BoundaryType::PERIODIC_BOUNDARY;
      } else {
        boundaryCons[i] =
            lsDomain<NumericType, D>::BoundaryType::REFLECTIVE_BOUNDARY;
      }
    }
    boundaryCons[D - 1] =
        lsDomain<NumericType, D>::BoundaryType::INFINITE_BOUNDARY;

    // substrate
    auto substrate = LSPtrType::New(bounds, boundaryCons, gridDelta);
    NumericType normal[D] = {0.};
    NumericType origin[D] = {0.};
    normal[D - 1] = 1.;
    origin[D - 1] = baseHeight;
    lsMakeGeometry<NumericType, D>(
        substrate, lsSmartPointer<lsPlane<NumericType, D>>::New(origin, normal))
        .apply();

    // mask layer
    auto mask = LSPtrType::New(bounds, boundaryCons, gridDelta);
    origin[D - 1] = holeDepth + baseHeight;
    lsMakeGeometry<NumericType, D>(
        mask, lsSmartPointer<lsPlane<NumericType, D>>::New(origin, normal))
        .apply();

    auto maskAdd = LSPtrType::New(bounds, boundaryCons, gridDelta);
    origin[D - 1] = baseHeight;
    normal[D - 1] = -1.;
    lsMakeGeometry<NumericType, D>(
        maskAdd, lsSmartPointer<lsPlane<NumericType, D>>::New(origin, normal))
        .apply();

    lsBooleanOperation<NumericType, D>(mask, maskAdd,
                                       lsBooleanOperationEnum::INTERSECT)
        .apply();

    // cylinder cutout
    normal[D - 1] = 1.;
    origin[D - 1] = baseHeight;

    NumericType topRadius = holeRadius;
    if (taperAngle) {
      topRadius += std::tan(taperAngle * rayInternal::PI / 180.) * holeDepth;
    }

    lsMakeGeometry<NumericType, D>(
        maskAdd,
        lsSmartPointer<lsCylinder<NumericType, D>>::New(
            origin, normal, holeDepth + 2 * gridDelta, holeRadius, topRadius))
        .apply();

    lsBooleanOperation<NumericType, D>(
        mask, maskAdd, lsBooleanOperationEnum::RELATIVE_COMPLEMENT)
        .apply();

    lsBooleanOperation<NumericType, D>(substrate, mask,
                                       lsBooleanOperationEnum::UNION)
        .apply();

    if (material == psMaterial::None) {
      if (makeMask)
        domain->insertNextLevelSet(mask);
      domain->insertNextLevelSet(substrate, false);
    } else {
      if (makeMask)
        domain->insertNextLevelSetAsMaterial(mask, psMaterial::Mask);
      domain->insertNextLevelSetAsMaterial(substrate, material, false);
    }
  }
};
