#pragma once
#include "Partition.hpp"
#include "logging/Logger.hpp"
#include "mesh/Mesh.hpp"

namespace precice {
namespace partition {

/**
 * @brief this class is supposed to:
 * 1- gather bounding boxes around the mesh partition of each rank in the master
 * 2- send them to the other master
 * 3- receive the feedback map from the other master (i.e. who is connected to whom from the other participant's perspective)
 * 4- create own initial connection map (i.e. who is connected to whom from this participant's perspective)
 */
class ProvidedBoundingBox : public Partition {
public:
  /// Constructor
  ProvidedBoundingBox(mesh::PtrMesh mesh, bool hasToSend, double safetyFactor);

  virtual ~ProvidedBoundingBox() {}

  // These functions will be implemented in 3rd package
  virtual void communicate();
  virtual void compute();
  virtual void createOwnerInformation();

  /// The bounding box is gathered and sent to another participant (if required)
  virtual void communicateBoundingBox();

  /// The feedback from the other participant is received and the initial connection map is build
  virtual void computeBoundingBox();

private:
  logging::Logger _log{"partition::ProvidedBoundingBox"};

  bool _hasToSend;

  int _dimensions;

  double _safetyFactor;
};

} // namespace partition
} // namespace precice
