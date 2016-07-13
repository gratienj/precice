#ifndef PRECICE_IO_TXTWRITER_HPP_
#define PRECICE_IO_TXTWRITER_HPP_

#include "tarch/logging/Log.h"
#include "tarch/la/traits/IsMatrix.h"
#include "tarch/la/traits/IsVector.h"
#include "tarch/la/traits/MatrixTraits.h"
#include "tarch/la/traits/VectorTraits.h"
#include "Eigen/Core"
#include <string>
#include <fstream>
#include <type_traits>

namespace tarch {
  namespace la {
    template<typename Scalar> class DynamicMatrix;
    template<typename Scalar> class DynamicVector;
  }
}

// ----------------------------------------------------------- CLASS DEFINITION

namespace precice {
namespace io {

/**
 * @brief File writer for matrix in Matlab V7 ASCII format.
 */
class TXTWriter
{
public:

  /**
   * @brief Constructor, opens file and sets format.
   */
  TXTWriter(const std::string& filename);

  /**
   * @brief Destructor, closes file.
   */
  ~TXTWriter();

  /**
   * @brief Writes (appends) the matrix to the file.
   */
  template<typename MATRIX>
    typename std::enable_if<tarch::la::IsMatrix<MATRIX>::value
  >::type write(const MATRIX& matrix)
  {
    typedef tarch::la::MatrixTraits<MATRIX> T;
    for (int i=0; i < T::rows(matrix); i++){
      for (int j=0; j < T::cols(matrix); j++){
        _file << T::celem(i,j,matrix) << " ";
      }
      _file << std::endl;
    }
  }

  /**
   * @brief Writes (appends) the vector to the file.
   */
  template<typename VECTOR>
    typename std::enable_if<tarch::la::IsVector<VECTOR>::value
  >::type write(const VECTOR& vector)
  {
    typedef tarch::la::VectorTraits<VECTOR> T;
    for (int i=0; i < T::size(vector); i++){
      _file << T::celem(i,vector) << " ";
    }
    _file << std::endl;
  }

  /**
   * @brief Writes (appends) the matrix to the file.
   */
 //void write(const Eigen::MatrixXd& matrix);

  /**
   * @brief Writes (appends) the matrix to the file.
   */
 //void write(const Eigen::VectorXd& vec);


private:

  // @brief Logging device.
  static tarch::logging::Log _log;

  // @brief Filestream.
  std::ofstream _file;
};

}} // namespace precice, io

#endif /* PRECICE_IO_TXTWRITER_HPP_ */
