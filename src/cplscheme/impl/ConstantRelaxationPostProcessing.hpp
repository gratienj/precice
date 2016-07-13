#ifndef PRECICE_CPLSCHEME_CONSTANTRELAXATIONPOSTPROCESSING_HPP_
#define PRECICE_CPLSCHEME_CONSTANTRELAXATIONPOSTPROCESSING_HPP_

#include "PostProcessing.hpp"
#include "tarch/logging/Log.h"
#include <map>

namespace precice {
namespace cplscheme {
namespace impl {

class ConstantRelaxationPostProcessing : public PostProcessing
{
public:

   ConstantRelaxationPostProcessing (
      double relaxation,
      std::vector<int>    dataIDs );

   virtual ~ConstantRelaxationPostProcessing() {}

   virtual std::vector<int> getDataIDs () const
   {
      return _dataIDs;
   }

    virtual void setDesignSpecification(
      Eigen::VectorXd& q);


    // TODO: change to call by ref when Eigen is used.
    virtual std::map<int, Eigen::VectorXd> getDesignSpecification(DataMap& cplData);


   virtual void initialize ( DataMap & cplData );

   virtual void performPostProcessing ( DataMap & cplData );

   virtual void iterationsConverged ( DataMap & cplData )
   {}

private:

   static tarch::logging::Log _log;

   double _relaxation;

   std::vector<int> _dataIDs;

   Eigen::VectorXd _designSpecification;
};

}}} // namespace precice, cplscheme, impl

#endif /* PRECICE_CPLSCHEME_CONSTANTRELAXATIONPOSTPROCESSING_HPP_ */
