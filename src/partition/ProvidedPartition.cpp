#include <algorithm>
#include <map>
#include <memory>
#include <ostream>
#include <utility>
#include <vector>

#include "com/CommunicateBoundingBox.hpp"
#include "com/CommunicateMesh.hpp"
#include "com/Communication.hpp"
#include "com/SharedPointer.hpp"
#include "logging/LogMacros.hpp"
#include "m2n/M2N.hpp"
#include "m2n/SharedPointer.hpp"
#include "mesh/BoundingBox.hpp"
#include "mesh/Mesh.hpp"
#include "mesh/Vertex.hpp"
#include "partition/Partition.hpp"
#include "partition/ProvidedPartition.hpp"
#include "precice/types.hpp"
#include "utils/Event.hpp"
#include "utils/IntraComm.hpp"
#include "utils/assertion.hpp"

using precice::utils::Event;

namespace precice {
extern bool syncMode;

namespace partition {

ProvidedPartition::ProvidedPartition(
    mesh::PtrMesh mesh)
    : Partition(std::move(mesh))
{
}

void ProvidedPartition::communicate()
{
  PRECICE_TRACE();

  prepare();

  if (_m2ns.empty())
    return;

  // Temporary globalMesh such that the primary rank also keeps his local mesh
  mesh::Mesh globalMesh(_mesh->getName(), _mesh->getDimensions(), mesh::Mesh::MESH_ID_UNDEFINED);
  bool       hasMeshBeenGathered = false;

  bool twoLevelInitAlreadyUsed = false;

  for (auto &m2n : _m2ns) {
    if (m2n->usesTwoLevelInitialization()) {

      PRECICE_CHECK(not twoLevelInitAlreadyUsed, "Two-level initialization does not yet support multiple receivers of a provided mesh. "
                                                 "Please either switch two-level initialization off in your m2n definition, or "
                                                 "adapt your mesh setup such that each provided mesh is only received by maximum one "
                                                 "participant.");
      twoLevelInitAlreadyUsed = true;

      Event e("partition.broadcastMeshPartitions." + _mesh->getName(), precice::syncMode);

      // communicate the total number of vertices to the other participants primary rank
      if (utils::IntraComm::isPrimary()) {
        _m2ns[0]->getPrimaryRankCommunication()->send(_mesh->getGlobalNumberOfVertices(), 0);
      }

      // the min and max of global vertex IDs of this rank's partition
      int minGlobalVertexID = _mesh->getVertexOffsets()[utils::IntraComm::getRank()] - _mesh->vertices().size();
      int maxGlobalVertexID = _mesh->getVertexOffsets()[utils::IntraComm::getRank()] - 1;

      // each rank sends its min/max global vertex index to connected remote ranks
      _m2ns[0]->broadcastSend(minGlobalVertexID, *_mesh);
      _m2ns[0]->broadcastSend(maxGlobalVertexID, *_mesh);

      // each rank sends its mesh partition to connected remote ranks
      _m2ns[0]->broadcastSendMesh(*_mesh);

    } else {

      if (not hasMeshBeenGathered) {
        //Gather mesh
        Event e("partition.gatherMesh." + _mesh->getName(), precice::syncMode);
        if (not utils::IntraComm::isSecondary()) {
          globalMesh.addMesh(*_mesh); // Add local primary mesh to global mesh
        }
        PRECICE_INFO("Gather mesh {}", _mesh->getName());
        if (utils::IntraComm::isPrimary()) {
          PRECICE_ASSERT(utils::IntraComm::getRank() == 0);
          PRECICE_ASSERT(utils::IntraComm::getSize() > 1);

          for (Rank secondaryRank : utils::IntraComm::allSecondaryRanks()) {
            com::CommunicateMesh(utils::IntraComm::getCommunication()).receiveMesh(globalMesh, secondaryRank);
            PRECICE_DEBUG("Received sub-mesh, from secondary rank: {}, global vertexCount: {}", secondaryRank, globalMesh.vertices().size());
          }
        }
        if (utils::IntraComm::isSecondary()) {
          com::CommunicateMesh(utils::IntraComm::getCommunication()).sendMesh(*_mesh, 0);
        }
        hasMeshBeenGathered = true;
      }

      // Send (global) Mesh
      PRECICE_INFO("Send global mesh {}", _mesh->getName());
      Event e("partition.sendGlobalMesh." + _mesh->getName(), precice::syncMode);

      if (not utils::IntraComm::isSecondary()) {
        PRECICE_CHECK(globalMesh.vertices().size() > 0,
                      "The provided mesh \"{}\" is empty. Please set the mesh using setMeshXXX() prior to calling initialize().",
                      globalMesh.getName());
        com::CommunicateMesh(m2n->getPrimaryRankCommunication()).sendMesh(globalMesh, 0);
      }
    }
  }
}

void ProvidedPartition::prepare()
{
  PRECICE_TRACE();
  PRECICE_INFO("Prepare partition for mesh {}", _mesh->getName());
  Event e("partition.prepareMesh." + _mesh->getName(), precice::syncMode);

  int numberOfVertices = _mesh->vertices().size();

  if (utils::IntraComm::isPrimary()) {
    PRECICE_ASSERT(utils::IntraComm::getSize() > 1);

    // set globals IDs on primary rank
    for (int i = 0; i < numberOfVertices; i++) {
      _mesh->vertices()[i].setGlobalIndex(i);
    }

    _mesh->getVertexOffsets().resize(utils::IntraComm::getSize());
    _mesh->getVertexOffsets()[0] = numberOfVertices;
    int globalNumberOfVertices   = numberOfVertices;

    // receive number of secondary vertices and fill vertex offsets
    for (Rank secondaryRank : utils::IntraComm::allSecondaryRanks()) {
      int numberOfSecondaryRankVertices = -1;
      utils::IntraComm::getCommunication()->receive(numberOfSecondaryRankVertices, secondaryRank);
      _mesh->getVertexOffsets()[secondaryRank] = numberOfSecondaryRankVertices + _mesh->getVertexOffsets()[secondaryRank - 1];
      utils::IntraComm::getCommunication()->send(globalNumberOfVertices, secondaryRank);
      globalNumberOfVertices += numberOfSecondaryRankVertices;
    }

    // set and broadcast global number of vertices
    _mesh->setGlobalNumberOfVertices(globalNumberOfVertices);
    PRECICE_DEBUG("Broadcast global number of vertices: {}", globalNumberOfVertices);
    utils::IntraComm::getCommunication()->broadcast(globalNumberOfVertices);

    // broadcast vertex offsets
    PRECICE_DEBUG("My vertex offsets: {}", _mesh->getVertexOffsets());
    utils::IntraComm::getCommunication()->broadcast(_mesh->getVertexOffsets());

    // fill vertex distribution
    if (std::any_of(_m2ns.begin(), _m2ns.end(), [](const m2n::PtrM2N &m2n) { return not m2n->usesTwoLevelInitialization(); })) {
      if (utils::IntraComm::isPrimary()) {
        PRECICE_DEBUG("Fill vertex distribution");
        auto &localIds = _mesh->getVertexDistribution()[0];
        for (int i = 0; i < _mesh->getVertexOffsets()[0]; i++) {
          localIds.push_back(i);
        }
        for (Rank secondaryRank : utils::IntraComm::allSecondaryRanks()) {
          // This always creates an entry for each secondary rank
          auto &secondaryIds = _mesh->getVertexDistribution()[secondaryRank];
          for (int i = _mesh->getVertexOffsets()[secondaryRank - 1]; i < _mesh->getVertexOffsets()[secondaryRank]; i++) {
            secondaryIds.push_back(i);
          }
        }
        PRECICE_ASSERT(_mesh->getVertexDistribution().size() == static_cast<decltype(_mesh->getVertexDistribution().size())>(utils::IntraComm::getSize()));
      }
    }
  } else if (utils::IntraComm::isSecondary()) {

    // send number of own vertices
    PRECICE_DEBUG("Send number of vertices: {}", numberOfVertices);
    utils::IntraComm::getCommunication()->send(numberOfVertices, 0);

    // set global IDs
    int globalVertexCounter = -1;
    utils::IntraComm::getCommunication()->receive(globalVertexCounter, 0);
    PRECICE_DEBUG("Set global vertex indices");
    for (int i = 0; i < numberOfVertices; i++) {
      _mesh->vertices()[i].setGlobalIndex(globalVertexCounter + i);
    }

    // set global number of vertices
    int globalNumberOfVertices = -1;
    utils::IntraComm::getCommunication()->broadcast(globalNumberOfVertices, 0);
    PRECICE_ASSERT(globalNumberOfVertices != -1);
    _mesh->setGlobalNumberOfVertices(globalNumberOfVertices);

    // set vertex offsets
    utils::IntraComm::getCommunication()->broadcast(_mesh->getVertexOffsets(), 0);
    PRECICE_DEBUG("My vertex offsets: {}", _mesh->getVertexOffsets());

  } else { // Coupling mode

    for (int i = 0; i < numberOfVertices; i++) {
      _mesh->getVertexDistribution()[0].push_back(i);
      _mesh->vertices()[i].setGlobalIndex(i);
    }
    _mesh->getVertexOffsets().push_back(numberOfVertices);
    _mesh->setGlobalNumberOfVertices(numberOfVertices);
  }

  PRECICE_DEBUG("Set owner information");
  for (mesh::Vertex &v : _mesh->vertices()) {
    v.setOwner(true);
  }
}

void ProvidedPartition::compute()
{
  PRECICE_TRACE();
  for (const auto &m2n : _m2ns) {
    if (m2n->usesTwoLevelInitialization()) {
      // @todo this will probably not work for more than one m2n
      PRECICE_ASSERT(_m2ns.size() <= 1);
      // receive communication map from all remote connected ranks
      m2n->gatherAllCommunicationMap(_mesh->getCommunicationMap(), *_mesh);
    }
  }
}

void ProvidedPartition::compareBoundingBoxes()
{
  PRECICE_TRACE();
  if (_m2ns.empty())
    return;

  _mesh->clearPartitioning();

  //@todo coupling mode

  //@todo treatment of multiple m2ns
  if (not _m2ns[0]->usesTwoLevelInitialization())
    return;

  // each secondary rank sends its bb to the primary rank
  if (utils::IntraComm::isSecondary()) { //secondary
    PRECICE_ASSERT(_mesh->getBoundingBox().getDimension() == _mesh->getDimensions(), "The boundingbox of the local mesh is invalid!");
    com::CommunicateBoundingBox(utils::IntraComm::getCommunication()).sendBoundingBox(_mesh->getBoundingBox(), 0);
  } else { // Primary

    PRECICE_ASSERT(utils::IntraComm::getRank() == 0);
    PRECICE_ASSERT(utils::IntraComm::getSize() > 1);

    // to store the collection of bounding boxes
    mesh::Mesh::BoundingBoxMap bbm;
    mesh::BoundingBox          bb(_mesh->getDimensions());
    bbm.emplace(0, _mesh->getBoundingBox());
    PRECICE_ASSERT(!bbm.empty(), "The bounding box of the local mesh is invalid!");

    // primary rank receives bbs from secondary ranks and stores them in bbm
    for (Rank secondaryRank : utils::IntraComm::allSecondaryRanks()) {
      // initialize bbm
      bbm.emplace(secondaryRank, bb);
      com::CommunicateBoundingBox(utils::IntraComm::getCommunication()).receiveBoundingBox(bbm.at(secondaryRank), secondaryRank);
    }

    // primary rank sends number of ranks and bbm to the other primary rank
    _m2ns[0]->getPrimaryRankCommunication()->send(utils::IntraComm::getSize(), 0);
    com::CommunicateBoundingBox(_m2ns[0]->getPrimaryRankCommunication()).sendBoundingBoxMap(bbm, 0);
  }

  // size of the feedbackmap
  int remoteConnectionMapSize = 0;

  if (utils::IntraComm::isPrimary()) {

    // primary rank receives feedback map (map of other participant ranks -> connected ranks at this participant)
    // from other participants primary rank
    std::vector<int> connectedRanksList = _m2ns[0]->getPrimaryRankCommunication()->receiveRange(0, com::AsVectorTag<int>{});
    remoteConnectionMapSize             = connectedRanksList.size();

    std::map<int, std::vector<int>> remoteConnectionMap;
    for (auto &rank : connectedRanksList) {
      remoteConnectionMap[rank] = {-1};
    }
    if (remoteConnectionMapSize != 0) {
      com::CommunicateBoundingBox(_m2ns[0]->getPrimaryRankCommunication()).receiveConnectionMap(remoteConnectionMap, 0);
    }

    // broadcast the received feedbackMap
    utils::IntraComm::getCommunication()->broadcast(connectedRanksList);
    if (remoteConnectionMapSize != 0) {
      com::CommunicateBoundingBox(utils::IntraComm::getCommunication()).broadcastSendConnectionMap(remoteConnectionMap);
    }

    // primary rank checks which ranks are connected to it
    _mesh->getConnectedRanks().clear();
    for (auto &remoteRank : remoteConnectionMap) {
      for (auto &includedRank : remoteRank.second) {
        if (utils::IntraComm::getRank() == includedRank) {
          _mesh->getConnectedRanks().push_back(remoteRank.first);
        }
      }
    }

  } else { // Secondary rank
    std::vector<int> connectedRanksList;
    utils::IntraComm::getCommunication()->broadcast(connectedRanksList, 0);

    std::map<int, std::vector<int>> remoteConnectionMap;
    if (!connectedRanksList.empty()) {
      for (Rank rank : connectedRanksList) {
        remoteConnectionMap[rank] = {-1};
      }
      com::CommunicateBoundingBox(utils::IntraComm::getCommunication()).broadcastReceiveConnectionMap(remoteConnectionMap);
    }

    _mesh->getConnectedRanks().clear();
    for (const auto &remoteRank : remoteConnectionMap) {
      for (int includedRanks : remoteRank.second) {
        if (utils::IntraComm::getRank() == includedRanks) {
          _mesh->getConnectedRanks().push_back(remoteRank.first);
        }
      }
    }
  }
}

} // namespace partition
} // namespace precice
