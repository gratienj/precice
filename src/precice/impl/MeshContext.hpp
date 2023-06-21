#pragma once

#include <ostream>
#include <vector>
#include "MappingContext.hpp"
#include "SharedPointer.hpp"
#include "com/Communication.hpp"
#include "mapping/Mapping.hpp"
#include "mesh/SharedPointer.hpp"
#include "partition/ReceivedPartition.hpp"
#include "partition/SharedPointer.hpp"

namespace precice {
namespace impl {

/// Stores a mesh and related objects and data.
struct MeshContext {
  /** Upgrades the mesh requirement to a more specific level.
    * @param[in] requirement The requirement to upgrade to.
    */
  void require(mapping::Mapping::MeshRequirement requirement);

  /// Mesh holding the geometry data structure.
  mesh::PtrMesh mesh;

  /// Determines which mesh type has to be provided by the accessor.
  mapping::Mapping::MeshRequirement meshRequirement = mapping::Mapping::MeshRequirement::UNDEFINED;

  /// Name of participant that creates the mesh.
  std::string receiveMeshFrom;

  /// bounding box to speed up decomposition of received mesh is increased by this safety factor
  double safetyFactor = -1;

  /// In case a mapping done by the solver is favored over a preCICE mapping, apply user-defined
  /// bounding-boxes.
  bool allowDirectAccess = false;

  /// True, if accessor does create the mesh.
  bool provideMesh = false;

  /// The kind of dynamicity of this mesh
  enum struct Dynamicity {
    No,          ///< static mesh, mappings will stay untouched too
    Yes,         ///< dynamic mesh, which is provided or received
    Transitively ///< static mesh, but has a mapping to a dynamic mesh
  };

  /// Whether the mesh is static, dynamically provided, dynamically received or transitively dynamic via mappings.
  Dynamicity dynamic = Dynamicity::No;

  /// type of geometric filter
  partition::ReceivedPartition::GeometricFilter geoFilter = partition::ReceivedPartition::GeometricFilter::UNDEFINED;

  /// Partition creating the parallel decomposition of the mesh
  partition::PtrPartition partition;

  /// Mappings used when mapping data from the mesh. Can be empty.
  std::vector<MappingContext> fromMappingContexts;

  /// Mappings used when mapping data to the mesh. Can be empty.
  std::vector<MappingContext> toMappingContexts;

  void clearMappings()
  {
    for (auto &mc : fromMappingContexts) {
      mc.mapping->clear();
    }
    for (auto &mc : toMappingContexts) {
      mc.mapping->clear();
    }
  }
};

inline std::ostream &operator<<(std::ostream &os, MeshContext::Dynamicity d)
{
  switch (d) {
  case MeshContext::Dynamicity::Yes:
    return os << "Dynamic";
  case MeshContext::Dynamicity::No:
    return os << "Static";
  case MeshContext::Dynamicity::Transitively:
    return os << "Transitively dynamic";
  default:
    return os;
  };
}

inline void MeshContext::require(mapping::Mapping::MeshRequirement requirement)
{
  meshRequirement = std::max(meshRequirement, requirement);
}

} // namespace impl
} // namespace precice
