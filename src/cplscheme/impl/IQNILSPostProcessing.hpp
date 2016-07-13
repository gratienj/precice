#ifndef PRECICE_CPLSCHEME_IQNILSPOSTPROCESSING_HPP_
#define PRECICE_CPLSCHEME_IQNILSPOSTPROCESSING_HPP_

#include "BaseQNPostProcessing.hpp"
#include "mesh/SharedPointer.hpp"
#include "tarch/logging/Log.h"
#include <deque>

// ----------------------------------------------------------- CLASS DEFINITION

namespace precice {
namespace cplscheme {
namespace impl {

/**
 * @brief Interface quasi-Newton with interface least-squares approximation.
 *
 * Performs a quasi-Newton to accelerate the convergence of implicit coupling
 * iterations. A least-squares approximation is used to find the best linear
 * combination of old data values. After every coupling iteration, the data
 * values used are enhanced by the new coupling iterates.
 *
 * If more coupling data is present than used to compute the IQN post-processing,
 * this data is relaxed using the same linear combination as computed for the
 * IQN-related data. The data is called "secondary" henceforth and additional
 * old value and data matrices are needed for it.
 */
class IQNILSPostProcessing : public BaseQNPostProcessing
{
public:

  /**
   * @brief Constructor.
   */
   IQNILSPostProcessing (
      double initialRelaxation,
      bool forceInitialRelaxation,
      int    maxIterationsUsed,
      int    timestepsReused,
      int 	 filter,
      double singularityLimit,
      std::vector<int>    dataIDs,
      PtrPreconditioner preconditioner);

   /**
    * @brief Destructor, empty.
    */
   virtual ~IQNILSPostProcessing() {}


   /**
    * @brief Initializes the post-processing.
    */
   virtual void initialize(DataMap& cplData);

   /**
    * @brief Marks a iteration sequence as converged.
    *
    * called by the iterationsConverged() method in the BaseQNPostProcessing class
    * handles the postprocessing sepcific action after the convergence of one iteration
    */
   virtual void specializedIterationsConverged(DataMap& cplData);

private:

   // @brief Secondary data solver output from last iteration.
   std::map<int, Eigen::VectorXd> _secondaryOldXTildes;


   // @brief Secondary data x-tilde deltas.
   //
   // Stores x-tilde deltas for data not involved in least-squares computation.
   std::map<int,Eigen::MatrixXd> _secondaryMatricesW;
   std::map<int,Eigen::MatrixXd> _secondaryMatricesWBackup;
   
   // @brief updates the V, W matrices (as well as the matrices for the secondary data)
   virtual void updateDifferenceMatrices(DataMap & cplData);

   // @brief computes the IQN-ILS update using QR decomposition
   virtual void computeQNUpdate(DataMap& cplData, Eigen::VectorXd& xUpdate);
   
   // @brief computes underrelaxation for the secondary data
   virtual void computeUnderrelaxationSecondaryData(DataMap& cplData);
   
   // @brief Removes one iteration from V,W matrices and adapts _matrixCols.
   virtual void removeMatrixColumn(int columnIndex);
};

}}} // namespace precice, cplscheme, impl

#endif /* PRECICE_CPLSCHEME_IQNILSPOSTPROCESSING_HPP_ */

