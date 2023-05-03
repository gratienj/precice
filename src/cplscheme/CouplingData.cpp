#include "cplscheme/CouplingData.hpp"

#include <utility>

#include "mesh/Data.hpp"
#include "mesh/Mesh.hpp"
#include "utils/EigenHelperFunctions.hpp"

namespace precice::cplscheme {

CouplingData::CouplingData(
    mesh::PtrData data,
    mesh::PtrMesh mesh,
    bool          requiresInitialization,
    int           extrapolationOrder)
    : requiresInitialization(requiresInitialization),
      _data(std::move(data)),
      _mesh(std::move(mesh)),
      _extrapolation(extrapolationOrder)
{
  PRECICE_ASSERT(_data != nullptr);
  /// Lazy allocation of _previousIteration.gradient: only used in case the corresponding data has gradients
  _previousIteration = time::Sample{Eigen::VectorXd::Zero(getSize())};

  PRECICE_ASSERT(_mesh != nullptr);
  PRECICE_ASSERT(_mesh.use_count() > 0);
}

int CouplingData::getDimensions() const
{
  PRECICE_ASSERT(_data != nullptr);
  return _data->getDimensions();
}

int CouplingData::getSize() const
{
  return sample().values.size();
}

Eigen::VectorXd &CouplingData::values()
{
  return sample().values;
}

const Eigen::VectorXd &CouplingData::values() const
{
  return sample().values;
}

Eigen::MatrixXd &CouplingData::gradientValues()
{
  return sample().gradient;
}

const Eigen::MatrixXd &CouplingData::gradientValues() const
{
  return sample().gradient;
}

time::Storage &CouplingData::timeStepsStorage()
{
  PRECICE_ASSERT(_data != nullptr);
  return _data->timeStepsStorage();
}

const time::Storage &CouplingData::timeStepsStorage() const
{
  PRECICE_ASSERT(_data != nullptr);
  return _data->timeStepsStorage();
}

const std::vector<time::Stample> &CouplingData::getStamples() const
{
  return timeStepsStorage().getStamples();
}

void CouplingData::setSampleAtTime(double time, time::Sample sample)
{
  this->sample() = sample; // @todo at some point we should not need this anymore, when mapping, acceleration ... directly work on _timeStepsStorage
  timeStepsStorage().setSampleAtTime(time, sample);
}

bool CouplingData::hasGradient() const
{
  PRECICE_ASSERT(_data != nullptr);
  return _data->hasGradient();
}

int CouplingData::meshDimensions() const
{
  return _mesh->getDimensions();
}

void CouplingData::storeIteration()
{
  const auto stamples = this->getStamples();
  // PRECICE_ASSERT(stamples.size() > 0);  //@todo preferable, but cannot use this, because of some invalid configs in tests (e.g. tests/serial/AitkenAcceleration.xml)
  if (stamples.size() > 0) {
    this->sample() = stamples.back().sample;
  }

  _previousIteration = this->sample();
}

const Eigen::VectorXd CouplingData::previousIteration() const
{
  return _previousIteration.values;
}

const Eigen::MatrixXd &CouplingData::previousIterationGradients() const
{
  return _previousIteration.gradient;
}

int CouplingData::getPreviousIterationSize() const
{
  return _previousIteration.values.size();
}

int CouplingData::getMeshID()
{
  return _mesh->getID();
}

int CouplingData::getDataID()
{
  return _data->getID();
}

std::string CouplingData::getDataName()
{
  return _data->getName();
}

std::vector<int> CouplingData::getVertexOffsets()
{
  return _mesh->getVertexOffsets();
}

void CouplingData::initializeExtrapolation()
{
  _extrapolation.initialize(getSize());
  storeIteration();
}

void CouplingData::moveToNextWindow(bool doExtrapolation)
{
  if (this->timeStepsStorage().getStamples().size() > 0) {
    auto atEndOfWindow = this->timeStepsStorage().getStamples().back();
    this->timeStepsStorage().clearAll();

    if (doExtrapolation) {
      _extrapolation.moveToNextWindow();
      this->values() = _extrapolation.getInitialGuess();
    }

    this->setSampleAtTime(time::Storage::WINDOW_START, atEndOfWindow.sample);
    this->setSampleAtTime(time::Storage::WINDOW_END, this->sample());
  }
}

void CouplingData::storeExtrapolationData()
{
  const auto stamples = this->getStamples();
  // PRECICE_ASSERT(stamples.size() > 0);  //@todo preferable, but cannot use this, because of some invalid configs in tests (e.g. tests/serial/AitkenAcceleration.xml)
  if (stamples.size() > 0) {
    this->values() = stamples.back().sample.values;
  }

  _extrapolation.store(values());
}

time::Sample &CouplingData::sample()
{
  PRECICE_ASSERT(_data != nullptr);
  return _data->sample();
}

const time::Sample &CouplingData::sample() const
{
  PRECICE_ASSERT(_data != nullptr);
  return _data->sample();
}

} // namespace precice::cplscheme
