#pragma once

#include "logging/Logger.hpp"
#include "m2n/SharedPointer.hpp"
#include "mapping/SharedPointer.hpp"
#include "mesh/SharedPointer.hpp"

#include <vector>

// ----------------------------------------------------------- CLASS DEFINITION

namespace precice
{
namespace partition
{

/**
 * @brief Abstract base class for partitions.
 *
 * A Partition describes how a mesh is decomposed among multiple ranks and
 * is associated to a "use-mesh" a participant holds. This class holds the
 * structures that describe the decomposition (if not the mesh) and compute
 * them.
 *
 * A Partition can come in two flavors: Either defined by a participants
 * (provided=true in the config) or received from another participant
 * (from=... in the config).
 *
 * Access to the associated mesh, to both mappings (from and to this mesh),
 * and to an m2n communication to another participant is necessary.
 */
class Partition
{
public:
  /// Constructor.
  Partition(mesh::PtrMesh mesh);

  Partition& operator=(Partition &&) = delete;

  virtual ~Partition() {}

  /// The mesh is communicated between both master ranks (if required)
  virtual void communicate() = 0;

  /// The partition is computed, i.e. the mesh re-partitioned if required and all data structures are set up.
  virtual void compute() = 0;

  void setFromMapping(mapping::PtrMapping fromMapping)
  {
    _fromMapping = fromMapping;
  }

  void setToMapping(mapping::PtrMapping toMapping)
  {
    _toMapping = toMapping;
  }

  void addM2N(m2n::PtrM2N m2n)
  {
    _m2ns.push_back(m2n);
  }

protected:
  mesh::PtrMesh _mesh;

  mapping::PtrMapping _fromMapping;

  mapping::PtrMapping _toMapping;

  /// m2n connection to each connected participant
  std::vector<m2n::PtrM2N> _m2ns;

  /// Decides which rank owns which vertex, information stored at each rank.
  virtual void createOwnerInformation() = 0;

  /// Generate vertex offsets from the vertexDistribution, broadcast it to all slaves
  void computeVertexOffsets();

private:
  logging::Logger _log{"partition::Partition"};
};

} // namespace partition
} // namespace precice
