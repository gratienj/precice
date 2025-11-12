#pragma once

#include <Eigen/Core>
#include <iomanip>
#include <ostream>
#include <string>
#include "ConvergenceMeasure.hpp"
#include "logging/Logger.hpp"
#include "utils/IntraComm.hpp"
#include "utils/assertion.hpp"
#include "precice/UserAppTools.hpp"

namespace precice::cplscheme::tests {
class UserAppConvergenceMeasureTest;
}
namespace precice {

class IUserAppConvergenceOp ;
}

namespace precice::cplscheme::impl {

/**
 * @brief Measures the convergence from an old data set to a new one.
 *
 * The convergence is evaluated by looking at the two norm of the differences
 * between each data value from the new and old data set. If the two norm is
 * equal or below a given limit, convergence is achieved.
 *
 * For a description of how to perform the measurement, see class
 * ConvergenceMeasure.
 */
class UserAppConvergenceMeasure : public ConvergenceMeasure {
public:
  explicit UserAppConvergenceMeasure(precice::IUserAppConvergenceOp* user_op);

  ~UserAppConvergenceMeasure() override = default;

  void newMeasurementSeries() override ;

  void measure(
      const Eigen::VectorXd &oldValues,
      const Eigen::VectorXd &newValues) override ;

  bool isConvergence() const override
  {
    return _isConvergence;
  }

  /// Adds current convergence information to output stream.
  std::string printState(const std::string &dataName) override
  {
    std::ostringstream os;
    os << "user app convergence measure: ";
    os << "two-norm diff of data \"" << dataName << "\" = ";
    os << std::scientific << std::setprecision(2) << _normDiff;
    os << ", conv = ";
    if (_isConvergence)
      os << "true";
    else
      os << "false";
    return os.str();
  }

  double getNormResidual() override
  {
    return _normDiff;
  }

  std::string getAbbreviation() const override
  {
    return "UserApp";
  }

private:
  logging::Logger _log{"cplscheme::UserAppConvergenceMeasure"};

  double _normDiff = 0;

  bool _isConvergence = false;

  precice::IUserAppConvergenceOp* _user_app_op = nullptr ;
};
} // namespace precice::cplscheme::impl
