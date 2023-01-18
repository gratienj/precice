#pragma once

#include <string>
#include <vector>
#include "logging/Logger.hpp"
#include "mapping/SharedPointer.hpp"
#include "mapping/config/MappingConfigurationTypes.hpp"
#include "mesh/SharedPointer.hpp"
#include "xml/XMLTag.hpp"

namespace precice {
namespace mapping {

/// Performs XML configuration and holds configured mappings.
class MappingConfiguration : public xml::XMLTag::Listener {
public:
  /// Constants defining the direction of a mapping.
  enum Direction {
    WRITE,
    READ
  };

  /// Configuration data for one mapping.
  struct ConfiguredMapping {
    /// Mapping object.
    PtrMapping mapping;
    /// Remote mesh to map from
    mesh::PtrMesh fromMesh;
    /// Remote mesh to map to
    mesh::PtrMesh toMesh;
    /// Direction of mapping (important to set input and output mesh).
    Direction direction;
    /// true for RBF mapping
    bool isRBF;
  };

  struct RBFParameter {

    enum struct Type {
      ShapeParameter,
      SupportRadius
    };

    Type   type{};
    double value{};
  };

  MappingConfiguration(
      xml::XMLTag &              parent,
      mesh::PtrMeshConfiguration meshConfiguration);

  /**
   * @brief Callback function required for use of automatic configuration.
   *
   * @return True, if successful.
   */
  virtual void xmlTagCallback(
      const xml::ConfigurationContext &context,
      xml::XMLTag &                    callingTag);

  /**
   * @brief Callback function required for use of automatic configuration.
   *
   * @return True, if successful.
   */
  virtual void xmlEndTagCallback(
      const xml::ConfigurationContext &context,
      xml::XMLTag &                    callingTag);

  /// Returns all configured mappings.
  const std::vector<ConfiguredMapping> &mappings();

  void resetMappings()
  {
    _mappings.clear();
  }

private:
  mutable logging::Logger _log{"config:MappingConfiguration"};

  const std::string TAG = "mapping";

  // First, declare common attributes and associated options
  const std::string ATTR_TYPE                      = "type";
  const std::string TYPE_NEAREST_NEIGHBOR          = "nearest-neighbor";
  const std::string TYPE_NEAREST_NEIGHBOR_GRADIENT = "nearest-neighbor-gradient";
  const std::string TYPE_NEAREST_PROJECTION        = "nearest-projection";
  const std::string TYPE_LINEAR_CELL_INTERPOLATION = "linear-cell-interpolation";
  const std::string TYPE_RBF_GLOBAL_DIRECT         = "rbf-global-direct";
  const std::string TYPE_RBF_GLOBAL_ITERATIVE      = "rbf-global-iterative";

  const std::string ATTR_DIRECTION  = "direction";
  const std::string DIRECTION_WRITE = "write";
  const std::string DIRECTION_READ  = "read";

  const std::string ATTR_FROM = "from";
  const std::string ATTR_TO   = "to";

  const std::string ATTR_CONSTRAINT                      = "constraint";
  const std::string CONSTRAINT_CONSISTENT                = "consistent";
  const std::string CONSTRAINT_CONSERVATIVE              = "conservative";
  const std::string CONSTRAINT_SCALED_CONSISTENT_SURFACE = "scaled-consistent-surface";
  const std::string CONSTRAINT_SCALED_CONSISTENT_VOLUME  = "scaled-consistent-volume";

  // Next, we have RBF specific options
  const std::string ATTR_BASIS_FUNCTION   = "basis-function";
  const std::string RBF_TPS               = "thin-plate-splines";
  const std::string RBF_MULTIQUADRICS     = "multiquadrics";
  const std::string RBF_INV_MULTIQUADRICS = "inverse-multiquadrics";
  const std::string RBF_VOLUME_SPLINES    = "volume-splines";
  const std::string RBF_GAUSSIAN          = "gaussian";
  const std::string RBF_CTPS_C2           = "compact-tps-c2";
  const std::string RBF_CPOLYNOMIAL_C0    = "compact-polynomial-c0";
  const std::string RBF_CPOLYNOMIAL_C2    = "compact-polynomial-c2";
  const std::string RBF_CPOLYNOMIAL_C4    = "compact-polynomial-c4";
  const std::string RBF_CPOLYNOMIAL_C6    = "compact-polynomial-c6";

  const std::string ATTR_SHAPE_PARAM    = "shape-parameter";
  const std::string ATTR_SUPPORT_RADIUS = "support-radius";
  const std::string ATTR_X_DEAD         = "x-dead";
  const std::string ATTR_Y_DEAD         = "y-dead";
  const std::string ATTR_Z_DEAD         = "z-dead";

  const std::string ATTR_POLYNOMIAL     = "polynomial";
  const std::string POLYNOMIAL_SEPARATE = "separate";
  const std::string POLYNOMIAL_ON       = "on";
  const std::string POLYNOMIAL_OFF      = "off";

  // For iterative RBFs
  const std::string ATTR_SOLVER_RTOL = "solver-rtol";
  // const std::string ATTR_MAX_ITERATIONS="";

  const std::string ATTR_PREALLOCATION     = "preallocation";
  const std::string PREALLOCATION_ESTIMATE = "estimate";
  const std::string PREALLOCATION_COMPUTE  = "compute";
  const std::string PREALLOCATION_SAVE     = "save";
  const std::string PREALLOCATION_TREE     = "tree";
  const std::string PREALLOCATION_OFF      = "off";

  // For the future
  // const std::string ATTR_PARALLELISM           = "parallelism";
  // const std::string PARALLELISM_GATHER_SCATTER = "gather-scatter";
  // const std::string PARALLELISM                = "distributed";

  mesh::PtrMeshConfiguration _meshConfig;

  std::vector<ConfiguredMapping> _mappings;

  ConfiguredMapping createMapping(
      const xml::ConfigurationContext &context,
      const std::string &              direction,
      const std::string &              type,
      const std::string &              constraint,
      const std::string &              fromMeshName,
      const std::string &              toMeshName,
      const RBFParameter &             rbfParameter,
      double                           solverRtol,
      bool                             xDead,
      bool                             yDead,
      bool                             zDead,
      bool                             useLU,
      Polynomial                       polynomial,
      Preallocation                    preallocation) const;

  /// Check whether a mapping to and from the same mesh already exists
  void checkDuplicates(const ConfiguredMapping &mapping);
};
} // namespace mapping
} // namespace precice
