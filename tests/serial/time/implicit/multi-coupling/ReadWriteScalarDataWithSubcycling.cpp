#ifndef PRECICE_NO_MPI

#include "testing/Testing.hpp"

#include <precice/SolverInterface.hpp>
#include <vector>

using namespace precice;

BOOST_AUTO_TEST_SUITE(Integration)
BOOST_AUTO_TEST_SUITE(Serial)
BOOST_AUTO_TEST_SUITE(Time)
BOOST_AUTO_TEST_SUITE(Implicit)
BOOST_AUTO_TEST_SUITE(MultiCoupling)
/**
 * @brief Test to run a multi coupling with subcycling. The three solvers use each a different time step size.
 */
BOOST_AUTO_TEST_CASE(ReadWriteScalarDataWithSubcycling)
{
  PRECICE_TEST("SolverOne"_on(1_rank), "SolverTwo"_on(1_rank), "SolverThree"_on(1_rank));

  SolverInterface precice(context.name, context.config(), 0, 1);

  MeshID meshID;
  DataID writeDataID;

  typedef double (*DataFunction)(double);
  std::vector<std::pair<DataID, DataFunction>> readDataPairs;

  DataFunction dataOneFunction = [](double t) -> double {
    return (double) (2 + t);
  };
  DataFunction dataTwoFunction = [](double t) -> double {
    return (double) (10 + t);
  };
  DataFunction dataThreeFunction = [](double t) -> double {
    return (double) (300 + t);
  };
  DataFunction writeFunction;

  int nSubsteps; // let three solvers use different time step sizes

  if (context.isNamed("SolverOne")) {
    meshID         = precice.getMeshID("MeshOne");
    writeDataID    = precice.getDataID("DataOne", meshID);
    writeFunction  = dataOneFunction;
    auto dataTwoId = precice.getDataID("DataTwo", meshID);
    readDataPairs.push_back(std::make_pair(dataTwoId, dataTwoFunction));
    auto dataThreeId = precice.getDataID("DataThree", meshID);
    readDataPairs.push_back(std::make_pair(dataThreeId, dataThreeFunction));
    nSubsteps = 1;
  } else if (context.isNamed("SolverTwo")) {
    meshID         = precice.getMeshID("MeshTwo");
    writeDataID    = precice.getDataID("DataTwo", meshID);
    writeFunction  = dataTwoFunction;
    auto dataOneId = precice.getDataID("DataOne", meshID);
    readDataPairs.push_back(std::make_pair(dataOneId, dataOneFunction));
    nSubsteps = 2;
  } else {
    BOOST_TEST(context.isNamed("SolverThree"));
    meshID         = precice.getMeshID("MeshThree");
    writeDataID    = precice.getDataID("DataThree", meshID);
    writeFunction  = dataThreeFunction;
    auto dataOneId = precice.getDataID("DataOne", meshID);
    readDataPairs.push_back(std::make_pair(dataOneId, dataOneFunction));
    nSubsteps = 3;
  }

  double   writeData, readData;
  VertexID vertexID = precice.setMeshVertex(meshID, Eigen::Vector3d(0.0, 0.0, 0.0).data());

  int    nWindows        = 5; // perform 5 windows.
  int    timestep        = 0;
  int    timewindow      = 0;
  double windowStartTime = 0;
  int    windowStartStep = 0;
  int    nSamples        = 4;
  int    iterations      = 0;
  double time            = 0;

  if (precice.isActionRequired(precice::constants::actionWriteInitialData())) {
    writeData = writeFunction(time);
    precice.writeScalarData(writeDataID, vertexID, writeData);
    precice.markActionFulfilled(precice::constants::actionWriteInitialData());
  }

  double maxDt     = precice.initialize();
  double windowDt  = maxDt;
  double dt        = windowDt / nSubsteps; // Timestep length desired by solver. E.g. 2 steps with size 1/2
  double currentDt = dt;                   // Timestep length used by solver

  while (precice.isCouplingOngoing()) {
    if (precice.isActionRequired(precice::constants::actionWriteIterationCheckpoint())) {
      windowStartTime = time;
      windowStartStep = timestep;
      precice.markActionFulfilled(precice::constants::actionWriteIterationCheckpoint());
    }

    for (auto readDataPair : readDataPairs) {
      auto readDataID   = readDataPair.first;
      auto readFunction = readDataPair.second;

      precice.readScalarData(readDataID, vertexID, readData);
      if (iterations == 0 && timestep == 0) {                        // special situation: Both solvers are in their very first time windows, first iteration, first time step
        BOOST_TEST(readData == readFunction(0));                     // use initial data only.
      } else if (iterations == 0) {                                  // special situation: Both solvers get the old data for all time windows.
        BOOST_TEST(readData == readFunction(timewindow * windowDt)); // data at end of window was written by other solver.
      } else if (iterations > 0) {
        BOOST_TEST(readData == readFunction((timewindow + 1) * windowDt));
      } else { // we should not enter this branch, because this would skip all tests.
        BOOST_TEST(false);
      }
    }

    // solve usually goes here. Dummy solve: Just sampling the writeFunction.
    time += currentDt;
    writeData = writeFunction(time);
    precice.writeScalarData(writeDataID, vertexID, writeData);
    maxDt     = precice.advance(currentDt);
    currentDt = dt > maxDt ? maxDt : dt;
    BOOST_CHECK(currentDt == windowDt / nSubsteps); // no subcycling.
    timestep++;
    if (precice.isActionRequired(precice::constants::actionReadIterationCheckpoint())) { // at end of window and we have to repeat it.
      iterations++;
      timestep = windowStartStep;
      time     = windowStartTime;
      precice.markActionFulfilled(precice::constants::actionReadIterationCheckpoint()); // this test does not care about checkpointing, but we have to make the action
    }
    if (precice.isTimeWindowComplete()) {
      timewindow++;
      iterations = 0;
    }
  }

  precice.finalize();
  BOOST_TEST(timestep == nWindows * nSubsteps);
}

BOOST_AUTO_TEST_SUITE_END() // Integration
BOOST_AUTO_TEST_SUITE_END() // Serial
BOOST_AUTO_TEST_SUITE_END() // Time
BOOST_AUTO_TEST_SUITE_END() // Implicit
BOOST_AUTO_TEST_SUITE_END() // MultiCoupling

#endif // PRECICE_NO_MPI
