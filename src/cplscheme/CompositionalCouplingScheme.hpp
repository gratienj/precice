#pragma once

#include <list>
#include <string>
#include <vector>
#include "Constants.hpp"
#include "CouplingScheme.hpp"
#include "SharedPointer.hpp"
#include "com/SharedPointer.hpp"
#include "logging/Logger.hpp"

namespace precice {
namespace cplscheme {

/**
 * @brief Acts as one coupling scheme, but consists of several composed ones.
 *
 * The sequence of how the schemes are added by addCouplingScheme() determines
 * their execution sequence. The schemes are executed from first added to last
 * added, following the rule that implicit schemes need to be converged before
 * executing the next explicit scheme/s.
 * An example with implicit schemes I1, ..., I5 and explicit
 * schemes E1, ..., E4 is given here. The schemes are added in the sequence
 *
 *   I1, I2, E1, E2, I3, E3, I4, I5, E4
 *
 * The execution is as follows:
 *
 *   1. I1 and I2 iterate until convergence
 *   2. E1 and E2 are computed
 *   3. I3 iterates until convergence
 *   4. E3 is computed
 *   5. I4 and I5 iterate until convergence
 *   6. E4 is computed
 *
 * Steps 2 and 3 are computed directly when 1 is converged, i.e., no additional
 * solver iteration/computation is necessary. Step 3, however, will require
 * additional solver computations for every additional iteration. Similarly,
 * 4 and 5 are computed directly when 3 is converged and 6 is computed directly
 * when 5 is converged.
 *
 * The grouping into such "active" sets is repeatedly done, in order to know
 * which coupling schemes should be executed. The implicit coupling scheme
 * actions constants::actionWrite/ReadIterationCheckpoint() is used to find out
 * about whether a coupling scheme does perform iterations.
 *
 * When, in an active set of implicit coupling schemes, a scheme is converged but
 * others not, the converged scheme is put on hold until all schemes in the
 * active set are converged. This assumes that the converged state of a coupling
 * scheme is not deteriorated by the further iteration of the
 * remaining non-converged schemes.
 *
 * The execution of each scheme does not only depend on this sequence, but also
 * on how the participants are configured to be first and second in the schemes.
 * If not configured properly, a deadlock might be created.
 */
class CompositionalCouplingScheme final : public CouplingScheme {
public:
  /// Destructor, empty.
  virtual ~CompositionalCouplingScheme() {}

  /**
   * @brief Adds another coupling scheme in parallel to this scheme.
   *
   * If this coupling scheme is a normal coupling scheme, an object of
   * CompositionalCouplingScheme will be created that contains this and the
   * new scheme in parallel. If this coupling scheme is already a composed
   * scheme, the new scheme will be added as another parallel scheme.
   *
   * @return Pointer to composition of coupling schemes.
   */
  void addCouplingScheme(const PtrCouplingScheme &scheme);

  /**
   * @brief Initializes the coupling scheme and establishes a communication
   *        connection to the coupling partner.
   * @param[in] startTime TODO
   * @param[in] startTimeWindow TODO
   */
  void initialize(
      double startTime,
      int    startTimeWindow) final override;

  void receiveResultOfFirstAdvance() override final;

  /// Returns true, if any of the composed coupling schemes sendsInitializedData for this participant
  bool sendsInitializedData() const override final;

  /// Returns true, if initialize has been called.
  bool isInitialized() const final override;

  /// Adds newly computed time. Has to be called before every advance.
  void addComputedTime(double timeToAdd) final override;

  /// Exchanges data and updates the state of the coupling scheme.
  //void advance() final override;

  ChangedMeshes firstSynchronization(const ChangedMeshes &changes) override;

  void firstExchange() override;

  ChangedMeshes secondSynchronization() override;

  void secondExchange() override;

  /// Finalizes the coupling and disconnects communication.
  void finalize() final override;

  std::string getLocalParticipant() const final override;

  /// Returns list of all coupling partners
  std::vector<std::string> getCouplingPartners() const final override;

  /**
   * @brief Returns true, if data will be exchanged when calling advance().
   *
   * Also returns true after the last call of advance() at the end of the
   * simulation.
   *
   * @param lastSolverTimestepLength [IN] The length of the last timestep
   *        computed by the solver calling willDataBeExchanged().
   */
  bool willDataBeExchanged(double lastSolverTimestepLength) const final override;

  /**
   * @brief checks all coupling schemes this coupling scheme is composed of.
   * @returns true, if data has been exchanged in last call of advance().
   */
  bool hasDataBeenReceived() const final override;

  /**
   * @brief Returns the currently computed time of the coupling scheme.
   *
   * This time is the minimum time of any coupling scheme in the composition.
   */
  double getTime() const final override;

  /**
   * @brief Returns the currently computed time windows of the coupling scheme.
   *
   * The time window is the minimum time window in any coupling scheme in the composition.
   */
  int getTimeWindows() const final override;

  /**
   * @brief Returns true, if timestep length by any of the coupling schemes in this compositional coupling scheme.
   *
   * If any of the solvers in the composition has a timestep length limit, this
   * counts as limit.
   */
  bool hasTimeWindowSize() const final override;

  /**
   * @brief Returns the timestep length, if one is given by the coupling scheme.
   *
   * An assertion is thrown, if no valid timestep is given. Check with
   * hasTimeWindowSize().
   *
   * The smallest timestep length limit in the coupling scheme composition has
   * to be obeyed.
   */
  double getTimeWindowSize() const final override;

  /**
   * @brief Returns the remaining timestep length inside the current time window.
   *
   * This is not necessarily the timestep length limit the solver has to obey
   * which is returned by getNextTimestepMaxLength().
   *
   * If no timestep length is prescribed by the coupling scheme, always 0.0 is
   * returned.
   *
   * The maximum remainder of all composed coupling schemes is returned.
   */
  double getThisTimeWindowRemainder() const final override;

  /**
   * @brief Returns the maximal length of the next timestep to be computed.
   *
   * If no timestep length is prescribed by the coupling scheme, always the
   * maximal double accuracy floating point number value is returned.
   *
   * This is the minimum of all max lengths of the composed coupling schemes.
   */
  double getNextTimestepMaxLength() const final override;

  /**
   * @brief Returns true, when the coupled simulation is still ongoing.
   *
   * As long as one composed coupling scheme is still ongoing, returns true.
   */
  bool isCouplingOngoing() const final override;

  /**
   * @brief Returns true, when the accessor can advance to the next time window.
   *
   * Only true, if all composed coupling schemes have completed the time window.
   */
  bool isTimeWindowComplete() const final override;

  /**
   * @brief Returns true, if the given action has to be performed by the accessor.
   *
   * True, if any of the composed coupling schemes requires the action.
   */
  bool isActionRequired(Action action) const final override;

  /// Returns true, if the given action has been performed by the accessor.
  bool isActionFulfilled(Action action) const final override;

  /// Tells the coupling scheme that the accessor has performed the given action.
  void markActionFulfilled(Action action) final override;

  /// Sets an action required to be performed by the accessor.
  void requireAction(Action action) final override;

  /// Returns a string representation of the current coupling state.
  std::string printCouplingState() const final override;

  /// True if one cplscheme is an implicit scheme
  bool isImplicitCouplingScheme() const final;

  /// True if the implicit scheme has converged or no implicit scheme is defined
  bool hasConverged() const final;

  bool isSynchronizationRequired() const final;

private:
  mutable logging::Logger _log{"cplscheme::CompositionalCouplingScheme"};

  using Schemes = std::vector<PtrCouplingScheme>;

  /// Explicit coupling schemes to be executed
  Schemes _explicitSchemes;

  /// The optional implicit scheme to be handled last
  PtrCouplingScheme _implicitScheme;

  /// Is the implicit scheme still iterating?
  bool _iterating = false;

  /// Returns all schemes at the beginning of a timestep or the implicit one until convergence
  std::vector<CouplingScheme *> schemesToRun() const;

  /// Returns all schemes
  std::vector<CouplingScheme *> allSchemes() const;
};

} // namespace cplscheme
} // namespace precice
