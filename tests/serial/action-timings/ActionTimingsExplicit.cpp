#ifndef PRECICE_NO_MPI

#include "testing/Testing.hpp"

#include <action/RecorderAction.hpp>
#include <precice/SolverInterface.hpp>
#include <vector>

/**
 * @brief Test to make sure that actions are called in the right order for explicit coupling via RecorderAction
 */
BOOST_AUTO_TEST_SUITE(Integration)
BOOST_AUTO_TEST_SUITE(Serial)
BOOST_AUTO_TEST_SUITE(ActionTimings)
BOOST_AUTO_TEST_CASE(ActionTimingsExplicit)
{
  PRECICE_TEST("SolverOne"_on(1_rank), "SolverTwo"_on(1_rank));

  using namespace precice;
  SolverInterface interface(context.name, context.config(), 0, 1);

  int         dimensions = interface.getDimensions();
  std::string meshName;
  std::string writeDataName;
  std::string readDataName;
  double      writeValue;

  if (context.isNamed("SolverOne")) {
    meshName      = "MeshOne";
    writeDataName = "Forces";
    readDataName  = "Velocities";
    writeValue    = 1;
  } else {
    BOOST_TEST(context.isNamed("SolverTwo"));
    meshName      = "MeshTwo";
    writeDataName = "Velocities";
    readDataName  = "Forces";
    writeValue    = 2;
  }
  int                 meshID      = interface.getMeshID(meshName);
  int                 writeDataID = interface.getDataID(writeDataName, meshID);
  int                 readDataID  = interface.getDataID(readDataName, meshID);
  std::vector<double> vertex(dimensions, 0);
  int                 vertexID = interface.setMeshVertex(meshID, vertex.data());

  double dt = -1;
  BOOST_TEST(action::RecorderAction::records.empty());
  action::RecorderAction::reset();
  std::vector<double> writeData(dimensions, writeValue);
  std::vector<double> readData(dimensions, -1);

  if (interface.requiresInitialData()) {
    BOOST_TEST(context.isNamed("SolverTwo"));
    interface.writeVectorData(writeDataID, vertexID, writeData.data());
  }

  dt = interface.initialize();
  BOOST_TEST(dt == 1.0);

  if (context.isNamed("SolverOne")) {
    BOOST_TEST(action::RecorderAction::records.size() == 2);
    BOOST_TEST(action::RecorderAction::records.at(0).timing == action::Action::READ_MAPPING_PRIOR);
    BOOST_TEST(action::RecorderAction::records.at(1).timing == action::Action::READ_MAPPING_POST);
  } else {
    BOOST_TEST(context.isNamed("SolverTwo"));
    BOOST_TEST(action::RecorderAction::records.size() == 4);
    BOOST_TEST(action::RecorderAction::records.at(0).timing == action::Action::WRITE_MAPPING_PRIOR);
    BOOST_TEST(action::RecorderAction::records.at(1).timing == action::Action::WRITE_MAPPING_POST);
    BOOST_TEST(action::RecorderAction::records.at(2).timing == action::Action::READ_MAPPING_PRIOR);
    BOOST_TEST(action::RecorderAction::records.at(3).timing == action::Action::READ_MAPPING_POST);
  }
  action::RecorderAction::reset();

  int iteration = 0;

  while (interface.isCouplingOngoing()) {
    interface.readVectorData(readDataID, vertexID, readData.data());
    interface.writeVectorData(writeDataID, vertexID, writeData.data());
    dt = interface.advance(dt);
    BOOST_TEST(interface.isTimeWindowComplete());
    iteration++;
    if (context.isNamed("SolverOne") || iteration < 10) {
      BOOST_TEST(action::RecorderAction::records.size() == 5);
      BOOST_TEST(action::RecorderAction::records.at(0).timing == action::Action::WRITE_MAPPING_PRIOR);
      BOOST_TEST(action::RecorderAction::records.at(1).timing == action::Action::WRITE_MAPPING_POST);
      BOOST_TEST(action::RecorderAction::records.at(2).timing == action::Action::READ_MAPPING_PRIOR);
      BOOST_TEST(action::RecorderAction::records.at(3).timing == action::Action::READ_MAPPING_POST);
      BOOST_TEST(action::RecorderAction::records.at(4).timing == action::Action::ON_TIME_WINDOW_COMPLETE_POST);
    } else { // SolverTwo only writes in very last iteration, does not read.
      BOOST_TEST(action::RecorderAction::records.size() == 3);
      BOOST_TEST(action::RecorderAction::records.at(0).timing == action::Action::WRITE_MAPPING_PRIOR);
      BOOST_TEST(action::RecorderAction::records.at(1).timing == action::Action::WRITE_MAPPING_POST);
      BOOST_TEST(action::RecorderAction::records.at(2).timing == action::Action::ON_TIME_WINDOW_COMPLETE_POST);
    }
    action::RecorderAction::reset();
  }
  interface.finalize();
}

BOOST_AUTO_TEST_SUITE_END() // Integration
BOOST_AUTO_TEST_SUITE_END() // Serial
BOOST_AUTO_TEST_SUITE_END() // ActionTimings

#endif // PRECICE_NO_MPI
