#include "../impl/MinIterationConvergenceMeasure.hpp"
#include "testing/Testing.hpp"

BOOST_AUTO_TEST_SUITE(CplSchemeTests)

using namespace precice;
using namespace cplscheme;

BOOST_AUTO_TEST_CASE(MinIterationConvergenceMeasureTest)
{
  cplscheme::impl::MinIterationConvergenceMeasure measure(5);
  Eigen::VectorXd                      emptyValues; // No values needed for min-iter

  for (int iSeries = 0; iSeries < 3; iSeries++) {
    measure.newMeasurementSeries();
    for (int iMeasurement = 1; iMeasurement < 10; iMeasurement++) {
      measure.measure(emptyValues, emptyValues, emptyValues);
      if (iMeasurement < 5) {
        BOOST_TEST(not measure.isConvergence());
      } else {
        BOOST_TEST(measure.isConvergence());
      }
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()
