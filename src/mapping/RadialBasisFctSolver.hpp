#pragma once

#include <Eigen/QR>
#include <boost/range/irange.hpp>
#include <numeric>
#include "mapping/config/MappingConfiguration.hpp"
#include "mesh/Mesh.hpp"
#include "precice/types.hpp"
#include "utils/Event.hpp"

namespace precice {
namespace mapping {

/**
 * This class assembles and solves an RBF system, given an input mesh and an output mesh with relevant vertex IDs.
 * The class uses a dense matrix decomposition in order to decompose the resulting system(s) and a backward substitution
 * in order to solve the system at runtime. The functionality uses Eigen and supports only serial execution. In case
 * the polynomial="separate" option is used, the polynomial system is solved using a QR decomposition.
 */
class RadialBasisFctSolver {
public:
  /// Default constructor
  RadialBasisFctSolver() = default;

  /// Assembles the system matrices and computes the decomposition of the interpolation matrix
  template <typename RADIAL_BASIS_FUNCTION_T>
  RadialBasisFctSolver(RADIAL_BASIS_FUNCTION_T basisFunction, const mesh::Mesh &inputMesh, const mesh::Mesh &outputMesh, std::vector<bool> deadAxis, Polynomial polynomial);

  /// Maps the given input data
  Eigen::VectorXd solveConsistent(Eigen::VectorXd &inputData, Polynomial polynomial) const;

  /// Maps the given input data
  Eigen::VectorXd solveConservative(const Eigen::VectorXd &inputData, Polynomial polynomial) const;

  // Clear all stored matrices
  void clear();

  // Access to the evaluation matrix (output x input)
  const Eigen::MatrixXd &getEvaluationMatrix() const;

private:
  precice::logging::Logger _log{"mapping::RadialBasisFctSolver"};

  /// Decomposition of the interpolation matrix
  Eigen::ColPivHouseholderQR<Eigen::MatrixXd> _qrMatrixC;

  /// Decomposition of the polynomial (for separate polynomial)
  Eigen::ColPivHouseholderQR<Eigen::MatrixXd> _qrMatrixQ;

  /// Polynomial matrix of the input mesh (for separate polynomial)
  Eigen::MatrixXd _matrixQ;

  /// Polynomial matrix of the output mesh (for separate polynomial)
  Eigen::MatrixXd _matrixV;

  /// Evaluation matrix (output x input)
  Eigen::MatrixXd _matrixA;
};

// ------- Non-Member Functions ---------

/// Deletes all dead directions from fullVector and returns a vector of reduced dimensionality.
inline double computeSquaredDifference(
    const std::array<double, 3> &u,
    std::array<double, 3>        v,
    const std::array<bool, 3> &  activeAxis)
{
  // Subtract the values and multiply out dead dimensions
  for (unsigned int d = 0; d < v.size(); ++d) {
    v[d] = (u[d] - v[d]) * static_cast<int>(activeAxis[d]);
  }
  // @todo: this can be replaced by std::hypot when moving to C++17
  return std::accumulate(v.begin(), v.end(), static_cast<double>(0.), [](auto &res, auto &val) { return res + val * val; });
}

// Fill in the polynomial entries
inline void fillPolynomialEntries(Eigen::MatrixXd &matrix, const mesh::Mesh &mesh, Eigen::Index startIndex, std::array<bool, 3> activeAxis)
{
  // Loop over all vertices in the mesh
  for (auto i : boost::irange<Eigen::Index>(0, mesh.vertices().size())) {

    // 1. the constant contribution
    matrix(i, startIndex) = 1.0;

    // 2. the linear contribution
    const auto & u = mesh.vertices()[i].rawCoords();
    unsigned int k = 0;
    // Loop over all three space dimension and ignore dead axis
    for (unsigned int d = 0; d < activeAxis.size(); ++d) {
      if (activeAxis[d]) {
        PRECICE_ASSERT(matrix.rows() > i, matrix.rows(), i);
        PRECICE_ASSERT(matrix.cols() > startIndex + 1 + k, matrix.cols(), startIndex + 1 + k);
        matrix(i, startIndex + 1 + k) = u[d];
        ++k;
      }
    }
  }
}

template <typename RADIAL_BASIS_FUNCTION_T>
Eigen::MatrixXd buildMatrixCLU(RADIAL_BASIS_FUNCTION_T basisFunction, const mesh::Mesh &inputMesh, std::array<bool, 3> activeAxis, Polynomial polynomial)
{
  // Treat the 2D case as 3D case with dead axis
  const unsigned int deadDimensions = std::count(activeAxis.begin(), activeAxis.end(), false);
  const unsigned int dimensions     = 3;
  const unsigned int polyparams     = polynomial == Polynomial::ON ? 1 + dimensions - deadDimensions : 0;

  // Add linear polynom degrees if polynomial requires this
  const auto inputSize = inputMesh.vertices().size();
  const auto n         = inputSize + polyparams;

  PRECICE_ASSERT((inputMesh.getDimensions() == 3) || activeAxis[2] == false);
  PRECICE_ASSERT((inputSize >= 1 + polyparams) || polynomial != Polynomial::ON, inputSize);

  Eigen::MatrixXd matrixCLU(n, n);
  matrixCLU.setZero();

  // Compute RBF matrix entries
  for (auto i : boost::irange<Eigen::Index>(0, inputSize)) {
    for (auto j : boost::irange<Eigen::Index>(i, inputSize)) {
      const auto &u                 = inputMesh.vertices()[i].rawCoords();
      const auto &v                 = inputMesh.vertices()[j].rawCoords();
      double      squaredDifference = computeSquaredDifference(u, v, activeAxis);
      matrixCLU(i, j)               = basisFunction.evaluate(std::sqrt(squaredDifference));
    }
  }

  // Add potentially the polynomial contribution in the matrix
  if (polynomial == Polynomial::ON) {
    fillPolynomialEntries(matrixCLU, inputMesh, inputSize, activeAxis);
  }
  matrixCLU.triangularView<Eigen::Lower>() = matrixCLU.transpose();
  return matrixCLU;
}

template <typename RADIAL_BASIS_FUNCTION_T>
Eigen::MatrixXd buildMatrixA(RADIAL_BASIS_FUNCTION_T basisFunction, const mesh::Mesh &inputMesh, const mesh::Mesh &outputMesh, std::array<bool, 3> activeAxis, Polynomial polynomial)
{
  // Treat the 2D case as 3D case with dead axis
  const unsigned int deadDimensions = std::count(activeAxis.begin(), activeAxis.end(), false);
  const unsigned int dimensions     = 3;
  const unsigned int polyparams     = polynomial == Polynomial::ON ? 1 + dimensions - deadDimensions : 0;

  const auto inputSize  = inputMesh.vertices().size();
  const auto outputSize = outputMesh.vertices().size();
  const auto n          = inputSize + polyparams;

  PRECICE_ASSERT((inputMesh.getDimensions() == 3) || activeAxis[2] == false);
  PRECICE_ASSERT((inputSize >= 1 + polyparams) || polynomial != Polynomial::ON, inputSize);

  Eigen::MatrixXd matrixA(outputSize, n);
  matrixA.setZero();

  // Compute RBF values for matrix A
  for (auto i : boost::irange<Eigen::Index>(0, outputSize)) {
    for (auto j : boost::irange<Eigen::Index>(0, inputSize)) {
      const auto &u                 = outputMesh.vertices()[i].rawCoords();
      const auto &v                 = inputMesh.vertices()[j].rawCoords();
      double      squaredDifference = computeSquaredDifference(u, v, activeAxis);
      matrixA(i, j)                 = basisFunction.evaluate(std::sqrt(squaredDifference));
    }
  }

  // Add potentially the polynomial contribution in the matrix
  if (polynomial == Polynomial::ON) {
    fillPolynomialEntries(matrixA, outputMesh, inputSize, activeAxis);
  }
  return matrixA;
}

template <typename RADIAL_BASIS_FUNCTION_T>
RadialBasisFctSolver::RadialBasisFctSolver(RADIAL_BASIS_FUNCTION_T basisFunction, const mesh::Mesh &inputMesh, const mesh::Mesh &outputMesh, std::vector<bool> deadAxis, Polynomial polynomial)
{
  // Convert dead axis vector into an active axis array so that we can handle the reduction more easily
  std::array<bool, 3> activeAxis({{false, false, false}});
  std::transform(deadAxis.begin(), deadAxis.end(), activeAxis.begin(), [](const auto ax) { return !ax; });
  // First, assemble the interpolation matrix and check the invertability
  _qrMatrixC = buildMatrixCLU(basisFunction, inputMesh, activeAxis, polynomial).colPivHouseholderQr();

  PRECICE_CHECK(_qrMatrixC.isInvertible(),
                "The interpolation matrix of the RBF mapping from mesh {} to mesh {} is not invertable. "
                "This means that the mapping problem is not well-posed. "
                "Please check if your coupling meshes are correct. Maybe you need to fix axis-aligned mapping setups "
                "by marking perpendicular axes as dead?",
                inputMesh.getName(), outputMesh.getName());

  // Second, assemble evaluation matrix
  _matrixA = buildMatrixA(basisFunction, inputMesh, outputMesh, activeAxis, polynomial);

  // In case we deal with separated polynomials, we need dedicated matrices for the polynomial contribution
  if (polynomial == Polynomial::SEPARATE) {

    // 1. Allocate memory for these matrices
    // 4 = 1 + dimensions(3) = maximum number of polynomial parameters
    const unsigned int polyParams = 4 - std::count(activeAxis.begin(), activeAxis.end(), false);
    _matrixQ.resize(inputMesh.vertices().size(), polyParams);
    _matrixV.resize(outputMesh.vertices().size(), polyParams);

    // 2. fill the matrices: Q for the inputMesh, V for the outputMesh
    fillPolynomialEntries(_matrixQ, inputMesh, 0, activeAxis);
    fillPolynomialEntries(_matrixV, outputMesh, 0, activeAxis);

    // 3. compute decomposition
    _qrMatrixQ = _matrixQ.colPivHouseholderQr();
  }
}
} // namespace mapping
} // namespace precice
