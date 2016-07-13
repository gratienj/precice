#include "ResidualPreconditioner.hpp"
#include "utils/MasterSlave.hpp"

namespace precice {
namespace cplscheme {
namespace impl {

tarch::logging::Log ResidualPreconditioner::
   _log ( "precice::cplscheme::ResidualPreconditioner" );

ResidualPreconditioner:: ResidualPreconditioner
(
    std::vector<int> dimensions,
    int maxNonConstTimesteps)
:
   Preconditioner (dimensions,
        maxNonConstTimesteps)
{}


void ResidualPreconditioner::_update_(bool timestepComplete, const Eigen::VectorXd& oldValues, const Eigen::VectorXd& res)
{
  if(not timestepComplete){
    std::vector<double> norms(_dimensions.size(),0.0);

    int offset = 0;
    for(size_t k=0; k<_dimensions.size(); k++){
      Eigen::VectorXd part = Eigen::VectorXd::Zero(_dimensions[k]*_sizeOfSubVector);
      for(int i=0; i<_dimensions[k]*_sizeOfSubVector; i++){
        part(i) = res(i+offset);
      }
      norms[k] = utils::MasterSlave::l2norm(part);
      offset += _dimensions[k]*_sizeOfSubVector;
      assertion(norms[k]>0.0);
    }

    offset = 0;
    for(size_t k=0; k<_dimensions.size(); k++){
      for(int i=0; i<_dimensions[k]*_sizeOfSubVector; i++){
        _weights[i+offset] = 1.0 / norms[k];
        _invWeights[i+offset] = norms[k];
      }
      offset += _dimensions[k]*_sizeOfSubVector;
    }

    _requireNewQR = true;
  }
}

}}} // namespace precice, cplscheme
