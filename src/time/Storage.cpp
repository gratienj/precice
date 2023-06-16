#include <boost/range.hpp>

#include "cplscheme/CouplingScheme.hpp"
#include "math/differences.hpp"
#include "time/Storage.hpp"
#include "utils/assertion.hpp"

namespace precice::time {

const double Storage::WINDOW_START = 0.0;

const double Storage::WINDOW_END = 1.0;

Storage::Storage()
    : _stampleStorage{}, _extrapolationOrder{0}
{
}

void Storage::initialize(time::Sample sample)
{
  _stampleStorage.emplace_back(Stample{WINDOW_START, sample});
  _stampleStorage.emplace_back(Stample{WINDOW_END, sample});
}

void Storage::setSampleAtTime(double time, Sample sample)
{
  PRECICE_ASSERT(math::smallerEquals(WINDOW_START, time), "Setting sample outside of valid range!");
  PRECICE_ASSERT(math::smallerEquals(time, WINDOW_END), "Setting sample outside of valid range!");
  // check if key "time" exists.
  auto existingSample = std::find_if(_stampleStorage.begin(), _stampleStorage.end(), [&time](const auto &s) { return math::equals(s.timestamp, time); });
  if (existingSample == _stampleStorage.end()) { // key does not exist yet
    PRECICE_ASSERT(math::smaller(maxStoredNormalizedDt(), time), maxStoredNormalizedDt(), time, "Trying to write sample with a time that is too small. Please use clear(), if you want to write new samples to the storage.");
    _stampleStorage.emplace_back(Stample{time, sample});
  } else { // overwrite sample at "time"
    for (auto &stample : _stampleStorage) {
      if (math::equals(stample.timestamp, time)) {
        stample.sample = sample;
        return;
      }
    }
    PRECICE_ASSERT(false, "unreachable!");
  }
}

void Storage::setExtrapolationOrder(int extrapolationOrder)
{
  _extrapolationOrder = extrapolationOrder;
}

double Storage::maxStoredNormalizedDt() const
{
  if (_stampleStorage.size() == 0) {
    return -1; // invalid return
  } else {
    return _stampleStorage.back().timestamp;
  }
}

int Storage::nTimes() const
{
  return _stampleStorage.size();
}

int Storage::nDofs() const
{
  PRECICE_ASSERT(_stampleStorage.size() > 0);
  return _stampleStorage[0].sample.values.size();
}

void Storage::move()
{
  PRECICE_ASSERT(nTimes() > 0);
  auto sampleAtBeginning = getSampleAtEnd();
  auto sampleAtEnd       = computeExtrapolation();
  _stampleStorage.clear();
  _stampleStorage.emplace_back(time::Stample{WINDOW_START, sampleAtBeginning});
  _stampleStorage.emplace_back(time::Stample{WINDOW_END, sampleAtEnd});
}

void Storage::trim()
{
  PRECICE_ASSERT(!_stampleStorage.empty(), "Storage does not contain any data!");
  PRECICE_ASSERT(_stampleStorage.front().timestamp == time::Storage::WINDOW_START);
  _stampleStorage.erase(++_stampleStorage.begin(), _stampleStorage.end());
}

Eigen::VectorXd Storage::getValuesAtOrAfter(double before) const
{
  auto stample = std::find_if(_stampleStorage.begin(), _stampleStorage.end(), [&before](const auto &s) { return math::greaterEquals(s.timestamp, before); });
  PRECICE_ASSERT(stample != _stampleStorage.end(), "no values found!");

  return stample->sample.values;
}

Eigen::VectorXd Storage::getTimes() const
{
  auto times = Eigen::VectorXd(nTimes());
  for (int i = 0; i < times.size(); i++) {
    times[i] = _stampleStorage[i].timestamp;
  }
  return times;
}

std::pair<Eigen::VectorXd, Eigen::MatrixXd> Storage::getTimesAndValues() const
{
  auto times  = Eigen::VectorXd(nTimes());
  auto values = Eigen::MatrixXd(nDofs(), nTimes());
  for (int i = 0; i < times.size(); i++) {
    times[i]      = _stampleStorage[i].timestamp;
    values.col(i) = _stampleStorage[i].sample.values;
  }
  return std::make_pair(times, values);
}

time::Sample Storage::computeExtrapolation()
{
  if (_extrapolationOrder == 0) {
    return getSampleAtEnd(); // use values at end of window as initial guess for next
  } else if (_extrapolationOrder == 1) {
    auto s0 = getSampleAtBeginning();
    auto s1 = getSampleAtEnd();
    return time::Sample{2 * s1.values - s0.values, 2 * s1.gradients - s0.gradients}; // use linear extrapolation from window at beginning and end of window.
  }
  PRECICE_UNREACHABLE("Invalid _extrapolationOrder")
}

time::Sample Storage::getSampleAtBeginning()
{
  return _stampleStorage.front().sample;
}

time::Sample Storage::getSampleAtEnd()
{
  return _stampleStorage.back().sample;
}

} // namespace precice::time
