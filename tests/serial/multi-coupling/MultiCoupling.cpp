#ifndef PRECICE_NO_MPI

#include "testing/Testing.hpp"

#include <precice/SolverInterface.hpp>
#include <vector>

using namespace precice;

BOOST_AUTO_TEST_SUITE(Integration)
BOOST_AUTO_TEST_SUITE(Serial)
BOOST_AUTO_TEST_SUITE(MultiCoupling)
/// Four solvers are multi-coupled.
BOOST_AUTO_TEST_CASE(MultiCoupling)
{
  PRECICE_TEST("SOLIDZ1"_on(1_rank), "SOLIDZ2"_on(1_rank), "SOLIDZ3"_on(1_rank), "NASTIN"_on(1_rank));
  std::vector<Eigen::Vector2d> positions;
  Eigen::Vector2d              position;
  position << 0.0, 0.0;
  positions.push_back(position);
  position << 1.0, 0.0;
  positions.push_back(position);
  position << 1.0, 1.0;
  positions.push_back(position);
  position << 0.0, 1.0;
  positions.push_back(position);

  std::vector<Eigen::Vector2d> datas;
  Eigen::Vector2d              data;
  data << 1.0, 1.0;
  datas.push_back(data);
  data << 2.0, 2.0;
  datas.push_back(data);
  data << 3.0, 3.0;
  datas.push_back(data);
  data << 4.0, 5.0;
  datas.push_back(data);

  std::string writeIterCheckpoint(precice::constants::actionWriteIterationCheckpoint());
  std::string readIterCheckpoint(precice::constants::actionReadIterationCheckpoint());

  if (context.isNamed("SOLIDZ1") ||
      context.isNamed("SOLIDZ2") ||
      context.isNamed("SOLIDZ3")) {
    MeshID meshID      = -1;
    int    dataWriteID = -1;
    int    dataReadID  = -1;

    precice::SolverInterface precice(context.name, context.config(), 0, 1);
    BOOST_TEST(precice.getDimensions() == 2);

    if (context.isNamed("SOLIDZ1")) {
      meshID      = precice.getMeshID("SOLIDZ_Mesh1");
      dataWriteID = precice.getDataID("Displacements1", meshID);
      dataReadID  = precice.getDataID("Forces1", meshID);
    } else if (context.isNamed("SOLIDZ2")) {
      meshID      = precice.getMeshID("SOLIDZ_Mesh2");
      dataWriteID = precice.getDataID("Displacements2", meshID);
      dataReadID  = precice.getDataID("Forces2", meshID);
    } else if (context.isNamed("SOLIDZ3")) {
      meshID      = precice.getMeshID("SOLIDZ_Mesh3");
      dataWriteID = precice.getDataID("Displacements3", meshID);
      dataReadID  = precice.getDataID("Forces3", meshID);
    }

    std::vector<int> vertexIDs;
    int              vertexID = -1;
    for (size_t i = 0; i < 4; i++) {
      vertexID = precice.setMeshVertex(meshID, positions.at(i).data());
      vertexIDs.push_back(vertexID);
    }

    precice.initialize();

    for (size_t i = 0; i < 4; i++) {
      precice.writeVectorData(dataWriteID, vertexIDs.at(i), datas.at(i).data());
    }

    if (precice.isActionRequired(writeIterCheckpoint)) {
      precice.markActionFulfilled(writeIterCheckpoint);
    }
    precice.advance(0.0001);
    if (precice.isActionRequired(readIterCheckpoint)) {
      precice.markActionFulfilled(readIterCheckpoint);
    }

    for (size_t i = 0; i < 4; i++) {
      precice.readVectorData(dataReadID, vertexIDs.at(i), datas.at(i).data());
    }

    BOOST_TEST(datas.at(0)(0) == 1.00000000000000002082e-03);
    BOOST_TEST(datas.at(0)(1) == 1.00000000000000002082e-03);
    BOOST_TEST(datas.at(1)(0) == 2.00000000000000000000e-03);
    BOOST_TEST(datas.at(1)(1) == 2.00000000000000002082e-03);
    BOOST_TEST(datas.at(2)(0) == 3.00000000000000006245e-03);
    BOOST_TEST(datas.at(2)(1) == 3.00000000000000006245e-03);
    BOOST_TEST(datas.at(3)(0) == 4.00000000000000008327e-03);
    BOOST_TEST(datas.at(3)(1) == 5.00000000000000010408e-03);

    precice.finalize();

  } else {
    BOOST_TEST(context.isNamed("NASTIN"));
    precice::SolverInterface precice("NASTIN", context.config(), 0, 1);
    BOOST_TEST(precice.getDimensions() == 2);
    MeshID meshID1      = precice.getMeshID("NASTIN_Mesh1");
    MeshID meshID2      = precice.getMeshID("NASTIN_Mesh2");
    MeshID meshID3      = precice.getMeshID("NASTIN_Mesh3");
    int    dataWriteID1 = precice.getDataID("Forces1", meshID1);
    int    dataWriteID2 = precice.getDataID("Forces2", meshID2);
    int    dataWriteID3 = precice.getDataID("Forces3", meshID3);

    std::vector<int> vertexIDs1;
    int              vertexID = -1;
    for (size_t i = 0; i < 4; i++) {
      vertexID = precice.setMeshVertex(meshID1, positions.at(i).data());
      vertexIDs1.push_back(vertexID);
    }
    std::vector<int> vertexIDs2;
    for (size_t i = 0; i < 4; i++) {
      vertexID = precice.setMeshVertex(meshID2, positions.at(i).data());
      vertexIDs2.push_back(vertexID);
    }
    std::vector<int> vertexIDs3;
    for (size_t i = 0; i < 4; i++) {
      vertexID = precice.setMeshVertex(meshID3, positions.at(i).data());
      vertexIDs3.push_back(vertexID);
    }

    precice.initialize();

    for (size_t i = 0; i < 4; i++) {
      precice.writeVectorData(dataWriteID1, vertexIDs1.at(i), datas.at(i).data());
      precice.writeVectorData(dataWriteID2, vertexIDs2.at(i), datas.at(i).data());
      precice.writeVectorData(dataWriteID3, vertexIDs3.at(i), datas.at(i).data());
    }

    if (precice.isActionRequired(writeIterCheckpoint)) {
      precice.markActionFulfilled(writeIterCheckpoint);
    }
    precice.advance(0.0001);
    if (precice.isActionRequired(readIterCheckpoint)) {
      precice.markActionFulfilled(readIterCheckpoint);
    }

    precice.finalize();
  }
}

BOOST_AUTO_TEST_SUITE_END() // Integration
BOOST_AUTO_TEST_SUITE_END() // Serial
BOOST_AUTO_TEST_SUITE_END() // MultiCoupling

#endif // PRECICE_NO_MPI
