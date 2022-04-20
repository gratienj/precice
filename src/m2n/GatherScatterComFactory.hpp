#pragma once

#include "DistributedComFactory.hpp"
#include "com/SharedPointer.hpp"
#include "m2n/DistributedCommunication.hpp"
#include "mesh/SharedPointer.hpp"

namespace precice {
namespace m2n {
class GatherScatterComFactory : public DistributedComFactory {
public:
  GatherScatterComFactory(com::PtrCommunication primaryCom);

  DistributedCommunication::SharedPointer newDistributedCommunication(
      mesh::PtrMesh mesh);

private:
  /// communication between the primary processes
  com::PtrCommunication _primaryCom;
};
} // namespace m2n
} // namespace precice
