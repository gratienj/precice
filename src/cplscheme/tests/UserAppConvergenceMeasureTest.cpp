#include <Eigen/Core>
#include "cplscheme/impl/UserAppConvergenceMeasure.hpp"
#include "cplscheme/impl/ConvergenceMeasure.hpp"
#include "cplscheme/CouplingScheme.hpp"
#include "testing/TestContext.hpp"
#include "testing/Testing.hpp"

using namespace precice;
using namespace cplscheme;

class UserOp
: public precice::IUserAppConvergenceOp
{
public :
  UserOp(int max_iter)
  : _max_iter(max_iter)
  {

  }

  void newMeasurementSeries()
  {
      _iter = 0 ;
      _convergence = false ;
  }

  double measure(
       const Eigen::VectorXd &oldValues,
       const Eigen::VectorXd &newValues)
  {
      ++_iter ;
      _convergence = (_iter >= _max_iter) ;
      return 0. ;
  }

   /// Returns true, if the last measurement indicates convergence.
   bool isConvergence() const {
       return _convergence ;
   }
private :
  int _iter = 0 ;
  int _max_iter = -1 ;
  bool _convergence = false ;
};

BOOST_AUTO_TEST_SUITE(CplSchemeTests)

PRECICE_TEST_SETUP(1_rank)
BOOST_AUTO_TEST_CASE(UserAppConvergenceMeasureTest)
{
  PRECICE_TEST();
  using Eigen::Vector3d;
  // Create convergence measure for Vector data

  UserOp user_op(2) ;
  precice::IUserAppConvergenceOp* user_op_ptr = &user_op ;
  cplscheme::impl::UserAppConvergenceMeasure measure(user_op_ptr);

  // Create data sets for old state of data and new state of data
  Vector3d oldValues0(-2, -1, 0);
  Vector3d oldValues1(2, 3, 4);
  Vector3d oldValues2(3, 4, 5);
  Vector3d newValues(5, 6, 7);

  measure.measure(oldValues0, newValues);    //iter = 1
  BOOST_TEST(not measure.isConvergence());

  measure.measure(oldValues1, newValues);    //iter = 2
  BOOST_TEST(measure.isConvergence());

  measure.newMeasurementSeries();            //iter = 0
  measure.measure(oldValues2, newValues);
  BOOST_TEST(not measure.isConvergence()); // iter = 1
}

BOOST_AUTO_TEST_SUITE_END()
