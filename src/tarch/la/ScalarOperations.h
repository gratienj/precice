#ifndef _TARCH_LA_SCALAROPERATIONS_H_
#define _TARCH_LA_SCALAROPERATIONS_H_

#include "tarch/la/traits/IsVector.h"
#include "tarch/la/traits/IsMatrix.h"
#include "tarch/la/Scalar.h"
#include <cmath>

namespace tarch {
  namespace la {

    /**
     * Returns the absolute value of a type by redirecting to std::abs.
     *
     * Why not using std::abs directily then? Since for this abs an overload
     * for int types exist, which does not for std::abs. This is crucial if
     * an operation has to compute abs for generic types.
     */
    template<typename Type>
      typename std::enable_if< not IsVector<Type>::value,
      Type
    >::type abs (Type value);

    /**
     * Returns the absolute value of the given int.
     */
    int abs (int value);

    /**
     * Computes the i-th power of a in integer arithmetic.
     */
    inline int aPowI(int i,int a);

    /**
     * Returns true, if lhs is greater than rhs by more than tolerance.
     */
    template<typename Type>
      typename std::enable_if<(not IsVector<Type>::value),
      bool
    >::type greater (
      Type lhs,
      Type rhs,
      Type tolerance = NUMERICAL_ZERO_DIFFERENCE);

    /**
     * Returns true, if lhs is greater or equal within a tolerance.
     */
    template<typename Type>
      typename std::enable_if<not IsVector<Type>::value,
      bool
    >::type greaterEquals (
      Type lhs,
      Type rhs,
      Type tolerance = NUMERICAL_ZERO_DIFFERENCE);

    /**
     * Returns true, if lhs is smaller than rhs by more than tolerance.
     */
    template<typename Type>
      typename std::enable_if<(not IsVector<Type>::value),
      bool
    >::type smaller (
      Type lhs,
      Type rhs,
      Type tolerance = NUMERICAL_ZERO_DIFFERENCE);

    /**
     * Returns true, if lhs is equals or smaller than rhs by more than tolerance.
     */
    template<typename Type>
      typename std::enable_if<(not IsVector<Type>::value),
      bool
    >::type smallerEquals (
      Type lhs,
      Type rhs,
      Type tolerance = NUMERICAL_ZERO_DIFFERENCE);

    /**
     * Returns true, if lhs is equals to rhs within a +/- tolerance range.
     */
    template<typename Type>
      typename std::enable_if<(not IsVector<Type>::value && not IsMatrix<Type>::value),
      bool
    >::type equals (
      Type lhs,
      Type rhs,
      Type tolerance = NUMERICAL_ZERO_DIFFERENCE);

    int sign (double number);

  } // namespace la
} // namespace tarch

#include "tarch/la/ScalarOperations.cpph"

#endif /* _TARCH_LA_SCALAROPERATIONS_H_ */
