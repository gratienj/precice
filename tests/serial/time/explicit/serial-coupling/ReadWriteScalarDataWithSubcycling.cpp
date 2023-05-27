#include <string>
#ifndef PRECICE_NO_MPI

#include "testing/Testing.hpp"

#include <precice/Participant.hpp>
#include <vector>

using namespace precice;

BOOST_AUTO_TEST_SUITE(Integration)
BOOST_AUTO_TEST_SUITE(Serial)
BOOST_AUTO_TEST_SUITE(Time)
BOOST_AUTO_TEST_SUITE(Explicit)
BOOST_AUTO_TEST_SUITE(SerialCoupling)

/**
 * @brief Test to run a simple coupling with subcycling.
 *
 * Ensures that each time step provides its own data, but preCICE only exchanges data at the end of the window.
 */
BOOST_AUTO_TEST_CASE(ReadWriteScalarDataWithSubcycling)
{
  PRECICE_TEST("SolverOne"_on(1_rank), "SolverTwo"_on(1_rank));

  Participant precice(context.name, context.config(), 0, 1);

  typedef double (*DataFunction)(double);

  DataFunction dataOneFunction = [](double t) -> double {
    return (double) (2 + t);
  };
  DataFunction dataTwoFunction = [](double t) -> double {
    return (double) (10 + t);
  };
  DataFunction writeFunction;
  DataFunction readFunction;

  std::string meshName, writeDataName, readDataName;
  if (context.isNamed("SolverOne")) {
    meshName      = "MeshOne";
    writeDataName = "DataOne";
    writeFunction = dataOneFunction;
    readDataName  = "DataTwo";
    readFunction  = dataTwoFunction;
  } else {
    BOOST_TEST(context.isNamed("SolverTwo"));
    meshName      = "MeshTwo";
    writeDataName = "DataTwo";
    writeFunction = dataTwoFunction;
    readDataName  = "DataOne";
    readFunction  = dataOneFunction;
  }

  double   writeData, readData;
  double   v0[]     = {0, 0, 0};
  VertexID vertexID = precice.setMeshVertex(meshName, v0);

  int    nSubsteps  = 4; // perform subcycling on solvers. 4 steps happen in each window.
  int    nWindows   = 5; // perform 5 windows.
  int    timestep   = 0;
  int    timewindow = 0;
  double time       = 0;

  if (precice.requiresInitialData()) {
    writeData = writeFunction(time);
    precice.writeData(meshName, writeDataName, {&vertexID, 1}, {&writeData, 1});
  }

  precice.initialize();
  BOOST_TEST(precice.getMaxTimeStepSize() == 2.0); // use window size != 1.0 to be able to detect more possible bugs
  double windowDt      = precice.getMaxTimeStepSize();
  double solverDt      = windowDt / (nSubsteps - 0.5);                 // Solver always tries to do a timestep of fixed size.
  double expectedDts[] = {4.0 / 7.0, 4.0 / 7.0, 4.0 / 7.0, 2.0 / 7.0}; // If solver uses timestep size of 4/7, fourth step will be restricted to 2/7 via preCICE steering to fit into the window.

  while (precice.isCouplingOngoing()) {
    double readTime;
    double preciceDt = precice.getMaxTimeStepSize();
    double currentDt = solverDt > preciceDt ? preciceDt : solverDt; // determine actual time step size; must fit into remaining time in window
    if (context.isNamed("SolverOne")) {
      readTime = timewindow * windowDt; // SolverOne lags one window behind SolverTwo for serial-explicit coupling.
    } else {
      readTime = (timewindow + 1) * windowDt; // SolverTwo gets result at end of window from SolverOne
    }

    // do read at end of window. preCICE v2 style subcycling.
    precice.readData(meshName, readDataName, {&vertexID, 1}, preciceDt, {&readData, 1});
    BOOST_TEST(readData == readFunction(readTime));

    // solve usually goes here. Dummy solve: Just sampling the writeFunction.
    BOOST_TEST(currentDt == expectedDts[timestep % nSubsteps]);
    time += currentDt;

    writeData = writeFunction(time);
    precice.writeData(meshName, writeDataName, {&vertexID, 1}, {&writeData, 1});

    precice.advance(currentDt);
    timestep++;
    if (precice.isTimeWindowComplete()) {
      timewindow++;
    }
  }

  precice.finalize();
  BOOST_TEST(timestep == nWindows * nSubsteps);
}

BOOST_AUTO_TEST_SUITE_END() // Integration
BOOST_AUTO_TEST_SUITE_END() // Serial
BOOST_AUTO_TEST_SUITE_END() // Time
BOOST_AUTO_TEST_SUITE_END() // Explicit
BOOST_AUTO_TEST_SUITE_END() // SerialCoupling

#endif // PRECICE_NO_MPI
