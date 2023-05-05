#pragma once

#include <vector>
#include "logging/Logger.hpp"
#include "mapping/Mapping.hpp"

namespace precice {
namespace mapping {

/// Geometric multiscale mapping in axial direction
class AxialGeoMultiscaleMapping : public Mapping {
public:
  /**
   * @brief Geometric multiscale nature of the mapping (spread or collect).
   *
   * A geometric multiscale mapping can either go from the 1D to the 2D/3D solver. Then, we call it "spread".
   * Or from the 2D/3D to the 1D solver, which we call "collect".
   */
  enum MultiscaleType {
    SPREAD,
    COLLECT
  };
  enum AxialAxis {
    X,
    Y,
    Z
  };

  /**
   * @brief Constructor.
   *
   * @param[in] constraint Specifies mapping to be consistent or conservative.
   * @param[in] dimensions Dimensionality of the meshes
   * @param[in] type Geometric multiscale type of the mapping
   * @param[in] axis Main axis along which axial geometric multiscale coupling happens
   * @param[in] radius Radius of the 1D solver "tube"
   */
  AxialGeoMultiscaleMapping(Constraint constraint, int dimensions, MultiscaleType type, AxialAxis axis, double radius);

  /// Destructor, empty.
  virtual ~AxialGeoMultiscaleMapping() {}

  /// Computes the mapping coefficients from the in- and output mesh.
  void computeMapping() override;

  /// Returns true, if computeMapping() has been called.
  // bool hasComputedMapping() const override; // check if needed at all

  /// Removes a computed mapping.
  void clear() override;

  void tagMeshFirstRound() override;
  void tagMeshSecondRound() override;

  /// Returns name of the mapping - TODO: needed for porting to develop
  // std::string getName() const final override;

protected:
  /// @copydoc Mapping::mapConservative
  void mapConservative(DataID inputDataID, DataID outputDataID) override;

  /// @copydoc Mapping::mapConsistent
  void mapConsistent(DataID inputDataID, DataID outputDataID) override;

private:
  mutable logging::Logger _log{"mapping::AxialGeoMultiscaleMapping"};

  MultiscaleType _type;

  /// radius of the "tube" from or to which the data is mapped, i.e., radius of the circular interface between the two participants
  double _radius;

  /// scaling to make up for difference between max and avg value for a certain shape function
  double _scaling = 0.0;

  /// main axis along which axial geometric multiscale coupling happens
  AxialAxis _axis;
};

} // namespace mapping
} // namespace precice
