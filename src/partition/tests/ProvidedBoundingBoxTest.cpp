#ifndef PRECICE_NO_MPI
#include "testing/Fixtures.hpp"
#include "testing/Testing.hpp"

#include "com/CommunicateBoundingBox.hpp"
#include "com/CommunicationFactory.hpp"
#include "com/MPIDirectCommunication.hpp"
#include "com/MPIPortsCommunication.hpp"
#include "com/MPIPortsCommunicationFactory.hpp"
#include "com/SocketCommunication.hpp"
#include "com/SocketCommunicationFactory.hpp"
#include "mapping/NearestNeighborMapping.hpp"
#include "mapping/NearestProjectionMapping.hpp"
#include "mapping/PetRadialBasisFctMapping.hpp"
#include "partition/ProvidedBoundingBox.hpp"
#include "partition/ReceivedBoundingBox.hpp"
#include "partition/SharedPointer.hpp"
#include "utils/MasterSlave.hpp"
#include "utils/Parallel.hpp"

using namespace precice;
using namespace partition;

BOOST_AUTO_TEST_SUITE(PartitionTests)
BOOST_AUTO_TEST_SUITE(ProvidedBoundingBoxTests)

void setupParallelEnvironment(m2n::PtrM2N m2n)
{
  PRECICE_ASSERT(utils::Parallel::getCommunicatorSize() == 4);

  com::PtrCommunication masterSlaveCom = com::PtrCommunication(new com::MPIDirectCommunication());
  utils::MasterSlave::_communication   = masterSlaveCom;

  if (utils::Parallel::getProcessRank() == 0) {
    utils::Parallel::splitCommunicator("Fluid");
    m2n->acceptMasterConnection("Fluid", "SolidMaster");
  } else if (utils::Parallel::getProcessRank() == 1) { //Master
    utils::Parallel::splitCommunicator("SolidMaster");
    m2n->requestMasterConnection("Fluid", "SolidMaster");
    utils::MasterSlave::configure(0, 3);
  } else if (utils::Parallel::getProcessRank() == 2) { //Slave1
    utils::Parallel::splitCommunicator("SolidSlaves");
    utils::MasterSlave::configure(1, 3);
  } else if (utils::Parallel::getProcessRank() == 3) { //Slave2
    utils::Parallel::splitCommunicator("SolidSlaves");
    utils::MasterSlave::configure(2, 3);
  }

  if (utils::Parallel::getProcessRank() == 1) { //Master
    masterSlaveCom->acceptConnection("SolidMaster", "SolidSlaves", utils::Parallel::getProcessRank());
    masterSlaveCom->setRankOffset(1);
  } else if (utils::Parallel::getProcessRank() == 2) { //Slave1
    masterSlaveCom->requestConnection("SolidMaster", "SolidSlaves", 0, 2);
  } else if (utils::Parallel::getProcessRank() == 3) { //Slave2
    masterSlaveCom->requestConnection("SolidMaster", "SolidSlaves", 1, 2);
  }
}

void tearDownParallelEnvironment()
{
  utils::MasterSlave::_communication = nullptr;
  utils::MasterSlave::reset();
  //utils::Parallel::synchronizeProcesses();
  utils::Parallel::clearGroups();
  mesh::Mesh::resetGeometryIDsGlobally();
  mesh::Data::resetDataCount();
  utils::Parallel::setGlobalCommunicator(utils::Parallel::getCommunicatorWorld());
}

BOOST_AUTO_TEST_CASE(TestCommunicateBoundingBox2D, *testing::OnSize(4))
{
  com::PtrCommunication participantCom =
      com::PtrCommunication(new com::SocketCommunication());
  m2n::DistributedComFactory::SharedPointer distrFactory = m2n::DistributedComFactory::SharedPointer(
      new m2n::GatherScatterComFactory(participantCom));
  m2n::PtrM2N m2n = m2n::PtrM2N(new m2n::M2N(participantCom, distrFactory));

  setupParallelEnvironment(m2n);

  int  dimensions  = 2;
  bool flipNormals = true;

  BOOST_TEST(utils::Parallel::getCommunicatorSize() == 4);

  if (utils::Parallel::getProcessRank() != 0) { //NASTIN

    mesh::PtrMesh pSolidzMesh(new mesh::Mesh("SolidzMesh", dimensions, flipNormals));

    if (utils::Parallel::getProcessRank() == 1) { //Master
      Eigen::VectorXd position(dimensions);
      position << -1.0, 0.0;
      mesh::Vertex &v0 = pSolidzMesh->createVertex(position);
      position << 1.0, 2.0;
      mesh::Vertex &v1 = pSolidzMesh->createVertex(position);
      position << 5.0, 3.0;
      mesh::Vertex &v2 = pSolidzMesh->createVertex(position);
      pSolidzMesh->createEdge(v0, v1);
      pSolidzMesh->createEdge(v1, v2);
    }

    else if (utils::Parallel::getProcessRank() == 2) { //Slave1
      Eigen::VectorXd position(dimensions);
      position << 1.0, 3.5;
      mesh::Vertex &v3 = pSolidzMesh->createVertex(position);
      position << 0.0, 4.5;
      mesh::Vertex &v4 = pSolidzMesh->createVertex(position);
      pSolidzMesh->createEdge(v3, v4);
    } else if (utils::Parallel::getProcessRank() == 3) { //Slave2
      Eigen::VectorXd position(dimensions);
      position << 2.5, 5.5;
      mesh::Vertex &v5 = pSolidzMesh->createVertex(position);
      position << 4.5, 7.0;
      mesh::Vertex &v6 = pSolidzMesh->createVertex(position);
      pSolidzMesh->createEdge(v5, v6);
    }

    double safetyFactor = 0.0;
    bool   hasToSend    = true;
    pSolidzMesh->computeState();

    ProvidedBoundingBox part(pSolidzMesh, hasToSend, safetyFactor);
    part.addM2N(m2n);
    part.communicateBoundingBox();

  } else { //SOLIDZ

    mesh::Mesh::BoundingBoxMap receivedGlobalBB;
    mesh::Mesh::BoundingBox    localBB;

    // we receive other participants communicator size
    int receivedFeedbackSize = 3;
    m2n->getMasterCommunication()->receive(receivedFeedbackSize, 0);

    for (int j = 0; j < dimensions; j++) {
      localBB.push_back(std::make_pair(-1, -1));
    }
    for (int i = 0; i < receivedFeedbackSize; i++) {
      receivedGlobalBB[i] = localBB;
    }

    // we receive golbal bounding box from othe participant!
    com::CommunicateBoundingBox(m2n->getMasterCommunication()).receiveBoundingBoxMap(receivedGlobalBB, 0);

    // check wether we have received the correct com size
    BOOST_TEST(receivedFeedbackSize == 3);

    //check the validity of received golbal bounding box (globalBB)
    BOOST_TEST(receivedGlobalBB[0][0].first == -1);
    BOOST_TEST(receivedGlobalBB[0][0].second == 5);
    BOOST_TEST(receivedGlobalBB[0][1].first == 0);
    BOOST_TEST(receivedGlobalBB[0][1].second == 3);
    BOOST_TEST(receivedGlobalBB[1][0].first == 0);
    BOOST_TEST(receivedGlobalBB[1][0].second == 1);
    BOOST_TEST(receivedGlobalBB[1][1].first == 3.5);
    BOOST_TEST(receivedGlobalBB[1][1].second == 4.5);
    BOOST_TEST(receivedGlobalBB[2][0].first == 2.5);
    BOOST_TEST(receivedGlobalBB[2][0].second == 4.5);
    BOOST_TEST(receivedGlobalBB[2][1].first == 5.5);
    BOOST_TEST(receivedGlobalBB[2][1].second == 7.0);
  }

  tearDownParallelEnvironment();
}

BOOST_AUTO_TEST_CASE(TestCommunicateBoundingBox3D, *testing::OnSize(4))
{
  com::PtrCommunication participantCom =
      com::PtrCommunication(new com::SocketCommunication());
  m2n::DistributedComFactory::SharedPointer distrFactory = m2n::DistributedComFactory::SharedPointer(
      new m2n::GatherScatterComFactory(participantCom));
  m2n::PtrM2N m2n = m2n::PtrM2N(new m2n::M2N(participantCom, distrFactory));

  setupParallelEnvironment(m2n);

  int  dimensions  = 3;
  bool flipNormals = true;

  BOOST_TEST(utils::Parallel::getCommunicatorSize() == 4);

  if (utils::Parallel::getProcessRank() != 0) { //NASTIN

    mesh::PtrMesh pSolidzMesh(new mesh::Mesh("SolidzMesh", dimensions, flipNormals));

    if (utils::Parallel::getProcessRank() == 1) { //Master
      Eigen::VectorXd position(dimensions);
      position << -1.0, 0.0, -1.0;
      mesh::Vertex &v0 = pSolidzMesh->createVertex(position);
      position << 1.0, 2.0, 1.0;
      mesh::Vertex &v1 = pSolidzMesh->createVertex(position);
      position << 5.0, 3.0, 5.0;
      mesh::Vertex &v2 = pSolidzMesh->createVertex(position);
      pSolidzMesh->createEdge(v0, v1);
      pSolidzMesh->createEdge(v1, v2);
    }

    else if (utils::Parallel::getProcessRank() == 2) { //Slave1
      Eigen::VectorXd position(dimensions);
      position << 1.0, 3.5, 1.0;
      mesh::Vertex &v3 = pSolidzMesh->createVertex(position);
      position << 0.0, 4.5, 0.0;
      mesh::Vertex &v4 = pSolidzMesh->createVertex(position);
      pSolidzMesh->createEdge(v3, v4);
    } else if (utils::Parallel::getProcessRank() == 3) { //Slave2
      Eigen::VectorXd position(dimensions);
      position << 2.5, 5.5, 2.5;
      mesh::Vertex &v5 = pSolidzMesh->createVertex(position);
      position << 4.5, 7.0, 4.5;
      mesh::Vertex &v6 = pSolidzMesh->createVertex(position);
      pSolidzMesh->createEdge(v5, v6);
    }

    double safetyFactor = 0.0;
    bool   hasToSend    = true;
    pSolidzMesh->computeState();

    ProvidedBoundingBox part(pSolidzMesh, hasToSend, safetyFactor);
    part.addM2N(m2n);
    part.communicateBoundingBox();

  } else { //SOLIDZ

    mesh::Mesh::BoundingBoxMap receivedGlobalBB;
    mesh::Mesh::BoundingBox    localBB;

    // we receive other participants communicator size
    int remoteParComSize = 3;
    m2n->getMasterCommunication()->receive(remoteParComSize, 0);

    for (int j = 0; j < dimensions; j++) {
      localBB.push_back(std::make_pair(-1, -1));
    }
    for (int i = 0; i < remoteParComSize; i++) {
      receivedGlobalBB[i] = localBB;
    }

    // we receive golbal bounding box from othe participant!
    com::CommunicateBoundingBox(m2n->getMasterCommunication()).receiveBoundingBoxMap(receivedGlobalBB, 0);

    // check wether we have received the correct com size
    BOOST_TEST(remoteParComSize == 3);

    //check the validity of received golbal bounding box (globalBB)
    BOOST_TEST(receivedGlobalBB[0][0].first == -1);
    BOOST_TEST(receivedGlobalBB[0][0].second == 5);
    BOOST_TEST(receivedGlobalBB[0][1].first == 0);
    BOOST_TEST(receivedGlobalBB[0][1].second == 3);
    BOOST_TEST(receivedGlobalBB[0][2].first == -1);
    BOOST_TEST(receivedGlobalBB[0][2].second == 5);
    BOOST_TEST(receivedGlobalBB[1][0].first == 0);
    BOOST_TEST(receivedGlobalBB[1][0].second == 1);
    BOOST_TEST(receivedGlobalBB[1][1].first == 3.5);
    BOOST_TEST(receivedGlobalBB[1][1].second == 4.5);
    BOOST_TEST(receivedGlobalBB[1][2].first == 0);
    BOOST_TEST(receivedGlobalBB[1][2].second == 1);
    BOOST_TEST(receivedGlobalBB[2][0].first == 2.5);
    BOOST_TEST(receivedGlobalBB[2][0].second == 4.5);
    BOOST_TEST(receivedGlobalBB[2][1].first == 5.5);
    BOOST_TEST(receivedGlobalBB[2][1].second == 7.0);
    BOOST_TEST(receivedGlobalBB[2][2].first == 2.5);
    BOOST_TEST(receivedGlobalBB[2][2].second == 4.5);
  }

  tearDownParallelEnvironment();
}

BOOST_AUTO_TEST_CASE(TestComputeBoundingBox, *testing::OnSize(4))
{
  com::PtrCommunication participantCom =
      com::PtrCommunication(new com::SocketCommunication());
  m2n::DistributedComFactory::SharedPointer distrFactory = m2n::DistributedComFactory::SharedPointer(
      new m2n::GatherScatterComFactory(participantCom));
  m2n::PtrM2N m2n = m2n::PtrM2N(new m2n::M2N(participantCom, distrFactory));

  setupParallelEnvironment(m2n);

  int  dimensions  = 2;
  bool flipNormals = true;

  BOOST_TEST(utils::Parallel::getCommunicatorSize() == 4);

  if (utils::Parallel::getProcessRank() == 0) {
    std::vector<int> connectedRanks = {0, 1, 2};
    m2n->getMasterCommunication()->send(connectedRanks, 0);

    // construct connection map
    std::map<int, std::vector<int>> sendConnectionMap;
    sendConnectionMap[0].push_back(1);
    sendConnectionMap[0].push_back(2);
    sendConnectionMap[1].push_back(0);
    sendConnectionMap[1].push_back(2);
    sendConnectionMap[2].push_back(0);
    sendConnectionMap[2].push_back(1);

    com::CommunicateBoundingBox(m2n->getMasterCommunication()).sendConnectionMap(sendConnectionMap, 0);
  } else { //NASTIN

    mesh::PtrMesh pSolidzMesh(new mesh::Mesh("SolidzMesh", dimensions, flipNormals));

    double safetyFactor = 0.0;
    bool   hasToSend    = true;
    pSolidzMesh->computeState();

    ProvidedBoundingBox part(pSolidzMesh, hasToSend, safetyFactor);
    part.addM2N(m2n);
    part.computeBoundingBox();

    if (utils::Parallel::getProcessRank() == 1) { //Master
      BOOST_TEST(pSolidzMesh->getConnectedRanks().size() == 2);
      BOOST_TEST(pSolidzMesh->getConnectedRanks()[0] == 1);
      BOOST_TEST(pSolidzMesh->getConnectedRanks()[1] == 2);
    } else if (utils::Parallel::getProcessRank() == 2) { //Slave1
      BOOST_TEST(pSolidzMesh->getConnectedRanks().size() == 2);
      BOOST_TEST(pSolidzMesh->getConnectedRanks()[0] == 0);
      BOOST_TEST(pSolidzMesh->getConnectedRanks()[1] == 2);
    } else if (utils::Parallel::getProcessRank() == 3) { //Slave2
      BOOST_TEST(pSolidzMesh->getConnectedRanks().size() == 2);
      BOOST_TEST(pSolidzMesh->getConnectedRanks()[0] == 0);
      BOOST_TEST(pSolidzMesh->getConnectedRanks()[1] == 1);
    }
  }
  tearDownParallelEnvironment();
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
#endif // PRECICE_NO_MPI
