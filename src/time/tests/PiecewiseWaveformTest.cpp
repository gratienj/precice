#include <Eigen/Core>
#include "testing/TestContext.hpp"
#include "testing/Testing.hpp"
#include "testing/WaveformFixture.hpp"
#include "time/Time.hpp"
#include "time/Waveform.hpp"

using namespace precice;
using namespace precice::time;

BOOST_AUTO_TEST_SUITE(TimeTests)
// @TODO rename to SubcyclingWaveformTests?
BOOST_AUTO_TEST_SUITE(PiecewiseWaveformTests)
BOOST_AUTO_TEST_SUITE(InterpolationTests)

BOOST_AUTO_TEST_CASE(testPiecewiseInterpolateDataZerothOrder)
{
  PRECICE_TEST(1_rank);

  testing::WaveformFixture fixture;

  // Test zeroth order interpolation
  const int interpolationOrder = 0;
  Waveform  waveform(interpolationOrder);
  const int valuesSize = 1;
  waveform.initialize(Eigen::VectorXd::Zero(valuesSize));

  BOOST_TEST(fixture.valuesSize(waveform) == valuesSize);
  BOOST_TEST(fixture.numberOfStoredSamples(waveform) == 2);
  BOOST_TEST(testing::equals(waveform.sample(0.00)(0), 0.0));
  BOOST_TEST(testing::equals(waveform.sample(0.25)(0), 0.0));
  BOOST_TEST(testing::equals(waveform.sample(0.50)(0), 0.0));
  BOOST_TEST(testing::equals(waveform.sample(0.75)(0), 0.0));
  BOOST_TEST(testing::equals(waveform.sample(1.00)(0), 0.0));

  Eigen::VectorXd value(1);
  value(0) = 0.5;
  waveform.store(value, 0.5);

  value(0) = 1.0;
  waveform.store(value, 1.0);

  BOOST_TEST(testing::equals(waveform.sample(0.00)(0), 0.0));
  BOOST_TEST(testing::equals(waveform.sample(0.25)(0), 0.5));
  BOOST_TEST(testing::equals(waveform.sample(0.50)(0), 0.5));
  BOOST_TEST(testing::equals(waveform.sample(0.75)(0), 1.0));
  BOOST_TEST(testing::equals(waveform.sample(1.00)(0), 1.0));

  value(0) = 1.5;
  waveform.store(value, 0.5);

  value(0) = 2.0;
  waveform.store(value, 1.0);

  BOOST_TEST(testing::equals(waveform.sample(0.00)(0), 0.0));
  BOOST_TEST(testing::equals(waveform.sample(0.25)(0), 1.5));
  BOOST_TEST(testing::equals(waveform.sample(0.50)(0), 1.5));
  BOOST_TEST(testing::equals(waveform.sample(0.75)(0), 2.0));
  BOOST_TEST(testing::equals(waveform.sample(1.00)(0), 2.0));

  waveform.moveToNextWindow();
  BOOST_TEST(fixture.numberOfStoredSamples(waveform) == 2);

  BOOST_TEST(testing::equals(waveform.sample(0.00)(0), 2.0));
  BOOST_TEST(testing::equals(waveform.sample(0.25)(0), 2.0));
  BOOST_TEST(testing::equals(waveform.sample(0.50)(0), 2.0));
  BOOST_TEST(testing::equals(waveform.sample(0.75)(0), 2.0));
  BOOST_TEST(testing::equals(waveform.sample(1.00)(0), 2.0));

  value(0) = 3.0;
  waveform.store(value, 1.0);

  BOOST_TEST(testing::equals(waveform.sample(0.00)(0), 2.0));
  BOOST_TEST(testing::equals(waveform.sample(0.25)(0), 3.0));
  BOOST_TEST(testing::equals(waveform.sample(0.50)(0), 3.0));
  BOOST_TEST(testing::equals(waveform.sample(0.75)(0), 3.0));
  BOOST_TEST(testing::equals(waveform.sample(1.00)(0), 3.0));

  value(0) = 1.5;
  waveform.store(value, 0.5);

  value(0) = 4.0;
  waveform.store(value, 1.0);

  BOOST_TEST(testing::equals(waveform.sample(0.00)(0), 2.0));
  BOOST_TEST(testing::equals(waveform.sample(0.25)(0), 1.5));
  BOOST_TEST(testing::equals(waveform.sample(0.50)(0), 1.5));
  BOOST_TEST(testing::equals(waveform.sample(0.75)(0), 4.0));
  BOOST_TEST(testing::equals(waveform.sample(1.00)(0), 4.0));
}

BOOST_AUTO_TEST_CASE(testPiecewiseInterpolateDataFirstOrder)
{
  PRECICE_TEST(1_rank);

  testing::WaveformFixture fixture;

  // Test zeroth order interpolation
  const int interpolationOrder = 1;
  Waveform  waveform(interpolationOrder);
  const int valuesSize = 1;
  waveform.initialize(Eigen::VectorXd::Zero(valuesSize));

  BOOST_TEST(fixture.valuesSize(waveform) == valuesSize);
  BOOST_TEST(fixture.numberOfStoredSamples(waveform) == 2);
  BOOST_TEST(testing::equals(waveform.sample(0.00)(0), 0.00));
  BOOST_TEST(testing::equals(waveform.sample(0.25)(0), 0.00));
  BOOST_TEST(testing::equals(waveform.sample(0.50)(0), 0.00));
  BOOST_TEST(testing::equals(waveform.sample(0.75)(0), 0.00));
  BOOST_TEST(testing::equals(waveform.sample(1.00)(0), 0.00));

  Eigen::VectorXd value(1);
  value(0) = 0.5;
  waveform.store(value, 0.5);

  value(0) = 1.0;
  waveform.store(value, 1.0);

  BOOST_TEST(testing::equals(waveform.sample(0.00)(0), 0.00));
  BOOST_TEST(testing::equals(waveform.sample(0.25)(0), 0.25));
  BOOST_TEST(testing::equals(waveform.sample(0.50)(0), 0.50));
  BOOST_TEST(testing::equals(waveform.sample(0.75)(0), 0.75));
  BOOST_TEST(testing::equals(waveform.sample(1.00)(0), 1.00));

  value(0) = 1.5;
  waveform.store(value, 0.5);

  value(0) = 2.0;
  waveform.store(value, 1.0);

  BOOST_TEST(testing::equals(waveform.sample(0.00)(0), 0.00));
  BOOST_TEST(testing::equals(waveform.sample(0.25)(0), 0.75));
  BOOST_TEST(testing::equals(waveform.sample(0.50)(0), 1.50));
  BOOST_TEST(testing::equals(waveform.sample(0.75)(0), 1.75));
  BOOST_TEST(testing::equals(waveform.sample(1.00)(0), 2.00));

  waveform.moveToNextWindow();
  BOOST_TEST(fixture.numberOfStoredSamples(waveform) == 2);

  BOOST_TEST(testing::equals(waveform.sample(0.00)(0), 2.00));
  BOOST_TEST(testing::equals(waveform.sample(0.25)(0), 2.00));
  BOOST_TEST(testing::equals(waveform.sample(0.50)(0), 2.00));
  BOOST_TEST(testing::equals(waveform.sample(0.75)(0), 2.00));
  BOOST_TEST(testing::equals(waveform.sample(1.00)(0), 2.00));

  value(0) = 3.0;
  waveform.store(value, 1.0);

  BOOST_TEST(testing::equals(waveform.sample(0.00)(0), 2.00));
  BOOST_TEST(testing::equals(waveform.sample(0.25)(0), 2.25));
  BOOST_TEST(testing::equals(waveform.sample(0.50)(0), 2.50));
  BOOST_TEST(testing::equals(waveform.sample(0.75)(0), 2.75));
  BOOST_TEST(testing::equals(waveform.sample(1.00)(0), 3.00));

  value(0) = 1.5;
  waveform.store(value, 0.5);

  value(0) = 4.0;
  waveform.store(value, 1.0);

  BOOST_TEST(testing::equals(waveform.sample(0.00)(0), 2.00));
  BOOST_TEST(testing::equals(waveform.sample(0.25)(0), 1.75));
  BOOST_TEST(testing::equals(waveform.sample(0.50)(0), 1.50));
  BOOST_TEST(testing::equals(waveform.sample(0.75)(0), 2.75));
  BOOST_TEST(testing::equals(waveform.sample(1.00)(0), 4.00));
}

BOOST_AUTO_TEST_CASE(testPiecewiseInterpolateDataThirdOrder)
{
  PRECICE_TEST(1_rank);

  testing::WaveformFixture fixture;

  // Test zeroth order interpolation
  const int interpolationOrder = 3;
  Waveform  waveform(interpolationOrder);
  const int valuesSize = 1;

  waveform.initialize(Eigen::VectorXd::Zero(valuesSize));

  BOOST_TEST(fixture.valuesSize(waveform) == valuesSize);
  BOOST_TEST(fixture.numberOfStoredSamples(waveform) == 2);
  BOOST_TEST(testing::equals(waveform.sample(0.00)(0), 0.00));
  BOOST_TEST(testing::equals(waveform.sample(0.25)(0), 0.00));
  BOOST_TEST(testing::equals(waveform.sample(0.50)(0), 0.00));
  BOOST_TEST(testing::equals(waveform.sample(0.75)(0), 0.00));
  BOOST_TEST(testing::equals(waveform.sample(1.00)(0), 0.00));

  // linearly increasing values
  Eigen::VectorXd value(1);
  for(double t : std::vector<double>{0.25, 0.5, 0.75, 1}) {
    value(0) = t;
    waveform.store(value, t);
  }

  for(double t: std::vector<double>{0.1,0.2,0.3,0.5,0.6,0.7,0.8,0.9,1.0}) {
    BOOST_TEST(testing::equals(waveform.sample(t)(0), t));
  }

  // quadratically increasing values
  for(double t : std::vector<double>{0.25, 0.5, 0.75, 1}) {
    value(0) = t*t;
    waveform.store(value, t);
  }

  // interpolates given values
  for(double t : std::vector<double>{0.25, 0.5, 0.75, 1}) {
    BOOST_TEST(testing::equals(waveform.sample(t)(0), t*t));
  }

  // introduces approximation error w.r.t function
  // @TODO: Potential optimization. Use a different parametrization. E.g. chord length? Should try to minimize error.
  for(double t: std::vector<double>{0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,1.0}) {
    BOOST_TEST(testing::equals(waveform.sample(t)(0), t*t));
  }

  // cubically increasing values
  for(double t : std::vector<double>{0.25, 0.5, 0.75, 1}) {
    value(0) = t*t*t;
    waveform.store(value, t);
  }

  // interpolates given values
  for(double t : std::vector<double>{0.25, 0.5, 0.75, 1}) {
    BOOST_TEST(testing::equals(waveform.sample(t)(0), t*t*t));
  }

  // introduces approximation error w.r.t function
  for(double t: std::vector<double>{0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,1.0}) {
    BOOST_TEST(testing::equals(waveform.sample(t)(0), t*t*t));
  }

  // cubically increasing values, but with non-uniform spacing
  for(double t : std::vector<double>{0.01, 0.1, 0.2, 1}) {
    value(0) = t*t*t;
    waveform.store(value, t);
  }

  // interpolates given values
  for(double t : std::vector<double>{0.01, 0.1, 0.2, 1}) {
    BOOST_TEST(testing::equals(waveform.sample(t)(0), t*t*t));
  }

  // introduces approximation error w.r.t function
  for(double t: std::vector<double>{0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,1.0}) {
    // 0.885212!=0.729 @ t = 0.9 without given knots (and sometimes not even interpolating above); looks like this is equal to method (3) with chord length parametrization.
    // 0.8760!=0.729 @ t = 0.9 with knots (w.r.t ts; method (1))
    // 0.6032!=0.729 @ t = 0.9 with knots (w.r.t uniform; method (2))
    BOOST_TEST(testing::equals(waveform.sample(t)(0), t*t*t));
  }
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()