// Copyright (C) 2011 Technische Universitaet Muenchen
// This file is part of the preCICE project. For conditions of distribution and
// use, please see the license notice at http://www5.in.tum.de/wiki/index.php/PreCICE_License
#ifndef RELATIVECONVERGENCEMEASURE_HPP_
#define RELATIVECONVERGENCEMEASURE_HPP_

#include "ConvergenceMeasure.hpp"
#include "../CouplingData.hpp"
#include "utils/Helpers.hpp"
#include "utils/Dimensions.hpp"
//#include "utils/NumericalCompare.hpp"
#include "tarch/logging/Log.h"
#include "utils/MasterSlave.hpp"

namespace precice {
   namespace cplscheme {
      namespace tests {
         class RelativeConvergenceMeasureTest;
      }
   }
}

// ----------------------------------------------------------- CLASS DEFINITION

namespace precice {
namespace cplscheme {
namespace impl {

/**
 * @brief Measures the convergence from an old data set to a new one.
 *
 * The convergence is evaluated by looking at the two norm of the differences
 * between each data value from the new and old data set. If the two norm is
 * equal or below a given percentage of the norm of the old data set,
 * convergence is achieved.
 *
 * For a description of how to perform the measurement, see class
 * ConvergenceMeasure.
 */
class RelativeConvergenceMeasure : public ConvergenceMeasure
{
public:

   /**
    * @brief Constructor.
    *
    * @param convergenceLimitPercent [IN]
    *        Limit to define convergence relative to the norm of the current
    *        new dataset. Has to be in $] 0 ; 1 ]$.
    */
   RelativeConvergenceMeasure ( double convergenceLimitPercent );

   virtual ~RelativeConvergenceMeasure () {};

   virtual void newMeasurementSeries ()
   {
      _isConvergence = false;
   }

   virtual void measure (
      const utils::DynVector & oldValues,
      const utils::DynVector & newValues )
   {
     //_normDiff = tarch::la::norm2(newValues - oldValues);
     //_norm = tarch::la::norm2(newValues);
     _normDiff = utils::MasterSlave::l2norm(newValues - oldValues);
     _norm = utils::MasterSlave::l2norm(newValues);
     _isConvergence = _normDiff <= _norm * _convergenceLimitPercent;
//      preciceInfo ( "measure()", "Relative convergence measure: "
//                    << "two-norm differences = " << normDiff
//                    << ", convergence limit = "
//                    << normNew * _convergenceLimitPercent
//                    << ", convergence = " << _isConvergence );
   }

   virtual bool isConvergence () const
   {
      return _isConvergence;
   }

   /**
    * @brief Adds current convergence information to output stream.
    */
   virtual std::string printState()
   {
     std::ostringstream os;
     os << "relative convergence measure: ";
     os << "two-norm diff = " << _normDiff;
     os << ", limit = " << _norm * _convergenceLimitPercent;
     os << ", conv = ";
     if (_isConvergence) os << "true";
     else os << "false";
     return os.str();
   }

   virtual double getNormResidual()
   {
     return _normDiff / ( _norm + std::numeric_limits<double>::epsilon() );
   }

private:

   static tarch::logging::Log _log;

   double _convergenceLimitPercent;

   double _normDiff;

   double _norm;

   bool _isConvergence;
};

}}} // namespace precice, cplscheme, impl

#endif /* RELATIVECONVERGENCEMEASURE_HPP_ */
