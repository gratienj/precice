#ifndef PRECICE_NO_MPI

#include "testing/Testing.hpp"

#include <precice/precice.hpp>
#include <vector>

using namespace precice;

BOOST_AUTO_TEST_SUITE(Integration)
BOOST_AUTO_TEST_SUITE(Serial)
BOOST_AUTO_TEST_SUITE(Time)
BOOST_AUTO_TEST_SUITE(Implicit)
BOOST_AUTO_TEST_SUITE(SerialCoupling)

/**
 * @brief Test to run a simple coupling with sampling from a zeroth order waveform; no subcycling.
 *
 * Provides a dt argument to the read function.
 */

BOOST_AUTO_TEST_CASE(ReadWriteScalarDataWithWaveformSamplingZero)
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

  double   writeData = 0;
  double   readData  = 0;
  double   v0[]      = {0, 0, 0};
  VertexID vertexID  = precice.setMeshVertex(meshName, v0);

  int    nWindows             = 5; // perform 5 windows.
  int    timewindow           = 0;
  int    nSamples             = 4;
  int    iterations           = 0;
  double time                 = 0;
  int    timewindowCheckpoint = timewindow;
  double timeCheckpoint       = time;
  double sampleDt; // dt relative to timestep start, where we are sampling
  double readDt;   // dt relative to timestep start for readTime
  double readTime; // time where we are reading from the reference solution
  if (precice.requiresInitialData()) {
    writeData = writeFunction(time);
    precice.writeData(meshName, writeDataName, {&vertexID, 1}, {&writeData, 1});
  }

  precice.initialize();
  double maxDt        = precice.getMaxTimeStepSize();
  double dt           = maxDt; // time step size desired by solver
  double currentDt    = dt;    // time step size used by solver
  double sampleDts[4] = {0.0, dt / 4.0, dt / 2.0, 3.0 * dt / 4.0};
  double readDts[4]   = {0.0, currentDt, currentDt, currentDt};

  while (precice.isCouplingOngoing()) {
    if (precice.requiresWritingCheckpoint()) {
      timeCheckpoint       = time;
      timewindowCheckpoint = timewindow;
      iterations           = 0;
    }
    for (int j = 0; j < nSamples; j++) {
      sampleDt = sampleDts[j];
      readDt   = readDts[j];
      readTime = time + readDt;
      precice.readData(meshName, readDataName, {&vertexID, 1}, sampleDt, {&readData, 1});
      if (context.isNamed("SolverOne") && iterations == 0) { // Solver One receives initial data / old data in first iteration
        BOOST_TEST(readData == readFunction(time));
      } else if (context.isNamed("SolverTwo") || (context.isNamed("SolverOne") && iterations > 0)) {
        BOOST_TEST(readData == readFunction(readTime));
      } else {
        BOOST_TEST(false); // unreachable!
      }
    }
    // solve usually goes here. Dummy solve: Just sampling the writeFunction.
    time += currentDt;
    timewindow++;
    writeData = writeFunction(time);
    precice.writeData(meshName, writeDataName, {&vertexID, 1}, {&writeData, 1});
    precice.advance(currentDt);
    maxDt = precice.getMaxTimeStepSize();
    if (precice.requiresReadingCheckpoint()) {
      time       = timeCheckpoint;
      timewindow = timewindowCheckpoint;
      iterations++;
    }
    currentDt = dt > maxDt ? maxDt : dt;
  }

  precice.finalize();
  BOOST_TEST(timewindow == nWindows);
}

BOOST_AUTO_TEST_SUITE_END() // Integration
BOOST_AUTO_TEST_SUITE_END() // Serial
BOOST_AUTO_TEST_SUITE_END() // Time
BOOST_AUTO_TEST_SUITE_END() // Implicit
BOOST_AUTO_TEST_SUITE_END() // SerialCoupling

#endif // PRECICE_NO_MPI
