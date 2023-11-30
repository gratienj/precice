#pragma once

#include <Eigen/Core>
#include <map>
#include <vector>

#include "cplscheme/BaseCouplingScheme.hpp"
#include "cplscheme/SharedPointer.hpp"

namespace precice {
namespace io {
class TXTWriter;
class TXTReader;
} // namespace io
} // namespace precice

namespace precice {
namespace acceleration {

class Acceleration {
public:
  static const int NOFILTER      = 0;
  static const int QR1FILTER     = 1;
  static const int QR1FILTER_ABS = 2;
  static const int QR2FILTER     = 3;
  static const int PODFILTER     = 4;

  /// Map from data ID to data values.
  using DataMap = std::map<int, cplscheme::PtrCouplingData>;

  virtual ~Acceleration() = default;

  virtual std::vector<int> getDataIDs() const = 0;

  virtual void initialize(const DataMap &cpldata) = 0;

  virtual void performAcceleration(DataMap &cpldata) = 0;

  virtual void iterationsConverged(const DataMap &cpldata) = 0;

  virtual void exportState(io::TXTWriter &writer) {}

  virtual void importState(io::TXTReader &reader) {}

  /// Gives the number of QN columns that where filtered out (i.e. deleted) in this time window
  virtual int getDeletedColumns() const
  {
    return 0;
  }

  /// Gives the number of QN columns that went out of scope in this time window
  virtual int getDroppedColumns() const
  {
    return 0;
  }

  /// Gives the number of current QN columns (LS = least squares)
  virtual int getLSSystemCols() const
  {
    return 0;
  }

protected:
  /// Checks if all dataIDs are contained in cplData
  void checkDataIDs(const DataMap &cplData) const;

  /// Concatenates all coupling data involved into a single vector
  void concatenateCouplingData(const DataMap &cplData, const std::vector<DataID> &dataIDs, Eigen::VectorXd &targetValues, Eigen::VectorXd &targetOldValues) const;

  /// performs a relaxation given a relaxation factor omega
  static void applyRelaxation(double omega, DataMap &cplData);
};
} // namespace acceleration
} // namespace precice
