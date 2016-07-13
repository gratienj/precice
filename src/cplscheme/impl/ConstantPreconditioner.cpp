#include "ConstantPreconditioner.hpp"

namespace precice {
namespace cplscheme {
namespace impl {

tarch::logging::Log ConstantPreconditioner::
   _log ( "precice::cplscheme::ConstantPreconditioner" );

ConstantPreconditioner:: ConstantPreconditioner
(
   std::vector<int> dimensions,
   std::vector<double> factors)
:
   Preconditioner(dimensions, -1),
   _factors(factors)
{}

void ConstantPreconditioner::initialize(int N){
  preciceTrace("initialize()");
  Preconditioner::initialize(N);

  // is always constant by definition
  _freezed = true;
  assertion(_maxNonConstTimesteps == -1, _maxNonConstTimesteps);

  assertion(_factors.size()==_dimensions.size());

  int offset = 0;
  for(size_t k=0; k<_dimensions.size(); k++){
    for(int i=0; i<_dimensions[k]*_sizeOfSubVector; i++){
      _weights[i+offset] = 1.0 / _factors[k];
      _invWeights[i+offset] = _factors[k];
    }
    offset += _dimensions[k]*_sizeOfSubVector;
  }
}

void ConstantPreconditioner::_update_(bool timestepComplete, const Eigen::VectorXd& oldValues, const Eigen::VectorXd& res)
{

  //nothing to do here
}

}}} // namespace precice, cplscheme
