#pragma once

#include <Eigen/Core>

namespace precice {
class IUserAppConvergenceOp
{
public:

    IUserAppConvergenceOp &operator=(IUserAppConvergenceOp &&) = delete;

    virtual ~IUserAppConvergenceOp() = default;

    virtual void newMeasurementSeries() = 0;

    virtual double measure(
         const Eigen::VectorXd &oldValues,
         const Eigen::VectorXd &newValues) = 0;

     /// Returns true, if the last measurement indicates convergence.
     virtual bool isConvergence() const = 0;
};
} // namespace precice
