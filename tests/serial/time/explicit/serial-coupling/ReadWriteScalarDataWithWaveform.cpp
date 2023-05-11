#ifndef PRECICE_NO_MPI

#include <precice/precice.hpp>
#include <vector>
#include "testing/Testing.hpp"

using namespace precice;

BOOST_AUTO_TEST_SUITE(Integration)
BOOST_AUTO_TEST_SUITE(Serial)
BOOST_AUTO_TEST_SUITE(Time)
BOOST_AUTO_TEST_SUITE(Explicit)
BOOST_AUTO_TEST_SUITE(SerialCoupling)

/**
 * @brief Test to run a simple explicit coupling with first order waveform subcycling.
 */
BOOST_AUTO_TEST_CASE(ReadWriteScalarDataWithWaveform)
{
  PRECICE_TEST("SolverOne"_on(1_rank), "SolverTwo"_on(1_rank));

  Participant precice(context.name, context.config(), 0, 1);

  std::string meshName;
  std::string writeDataName;
  std::string readDataName;

  typedef double (*DataFunction)(double);

  DataFunction dataOneFunction = [](double t) -> double {
    return (double) (2 + t);
  };
  DataFunction dataTwoFunction = [](double t) -> double {
    return (double) (10 + t);
  };
  DataFunction writeFunction;
  DataFunction readFunction;

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
  double maxDt = precice.getMaxTimeStepSize();
  BOOST_TEST(maxDt == 2.0); // use window size != 1.0 to be able to detect more possible bugs
  double windowDt  = maxDt;
  double dt        = windowDt / (nSubsteps - 0.5); // Solver always tries to do a timestep of fixed size.
  double currentDt = dt > maxDt ? maxDt : dt;      // determine actual time step size; must fit into remaining time in window
  double timeCheckpoint{0};

  while (precice.isCouplingOngoing()) {

    precice.readData(meshName, readDataName, {&vertexID, 1}, currentDt, {&readData, 1});

    if (context.isNamed("SolverOne")) { // first participant receives constant value from second
      BOOST_TEST(readData == readFunction(timeCheckpoint));
    } else { // second participant samples from waveform
      BOOST_TEST(readData == readFunction(time + currentDt));
    }

    precice.readData(meshName, readDataName, {&vertexID, 1}, currentDt / 2, {&readData, 1});

    if (context.isNamed("SolverOne")) { // first participant receives constant value from second
      BOOST_TEST(readData == readFunction(timeCheckpoint));
    } else { // second participant samples from waveform
      BOOST_TEST(readData == readFunction(time + currentDt / 2));
    }

    // solve usually goes here. Dummy solve: Just sampling the writeFunction.
    time += currentDt;
    // Need to keep track of the timewindows to check correct order on the first participant!
    timestep++;
    if (timestep % nSubsteps == 0) {
      timeCheckpoint += windowDt;
    }

    writeData = writeFunction(time);
    precice.writeData(meshName, writeDataName, {&vertexID, 1}, {&writeData, 1});
    precice.advance(currentDt);
    maxDt = precice.getMaxTimeStepSize();

    currentDt = dt > maxDt ? maxDt : dt;
  }

  BOOST_TEST(timestep == nWindows * nSubsteps);
}

BOOST_AUTO_TEST_SUITE_END() // Integration
BOOST_AUTO_TEST_SUITE_END() // Serial
BOOST_AUTO_TEST_SUITE_END() // Time
BOOST_AUTO_TEST_SUITE_END() // Explicit
BOOST_AUTO_TEST_SUITE_END() // SerialCoupling

#endif // PRECICE_NO_MPI
