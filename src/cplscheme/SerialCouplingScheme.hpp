#pragma once

#include <string>
#include "BaseCouplingScheme.hpp"
#include "BiCouplingScheme.hpp"
#include "cplscheme/Constants.hpp"
#include "logging/Logger.hpp"
#include "m2n/SharedPointer.hpp"

namespace precice {

namespace testing {
// Forward declaration to friend the boost test struct
struct SerialCouplingSchemeFixture;
} // namespace testing

namespace cplscheme {
/**
 * @brief Coupling scheme for serial coupling, i.e. staggered execution of two coupled participants
 *
 * For more information, look into Benjamin's thesis, Section 3.5.
 * https://mediatum.ub.tum.de/doc/1320661/document.pdf
 */
class SerialCouplingScheme : public BiCouplingScheme {
  friend struct testing::SerialCouplingSchemeFixture; // Make the fixture friend of this class
public:
  /**
 * @brief Constructor.
 *
 * @param[in] maxTime Simulation time limit, or UNDEFINED_MAX_TIME.
 * @param[in] maxTimeWindows Simulation time windows limit, or UNDEFINED_TIME_WINDOWS.
 * @param[in] timeWindowSize Simulation time window size.
 * @param[in] validDigits valid digits for computation of the remainder of a time window
 * @param[in] firstParticipant Name of participant starting simulation.
 * @param[in] secondParticipant Name of second participant in coupling.
 * @param[in] localParticipant Name of participant using this coupling scheme.
 * @param[in] m2n Communication object for com. between participants.
 * @param[in] dtMethod Method used for determining the time window size, see https://www.precice.org/couple-your-code-timestep-sizes.html
 * @param[in] cplMode Set implicit or explicit coupling
 * @param[in] maxIterations maximum number of coupling iterations allowed for implicit coupling per time window
 * @param[in] extrapolationOrder order used for extrapolation
 */
  SerialCouplingScheme(
      double                        maxTime,
      int                           maxTimeWindows,
      double                        timeWindowSize,
      int                           validDigits,
      const std::string &           firstParticipant,
      const std::string &           secondParticipant,
      const std::string &           localParticipant,
      m2n::PtrM2N                   m2n,
      constants::TimesteppingMethod dtMethod,
      CouplingMode                  cplMode,
      int                           maxIterations      = UNDEFINED_MAX_ITERATIONS,
      int                           extrapolationOrder = UNDEFINED_EXTRAPOLATION_ORDER);

  CouplingScheme::ChangedMeshes firstSynchronization(const CouplingScheme::ChangedMeshes &changes) final;

  CouplingScheme::ChangedMeshes secondSynchronization() final;

  /// @copydoc CouplingScheme::getNormalizedWindowTime
  double getNormalizedWindowTime() const override; // @todo try to make private?

protected:
  /**
   * @brief Setter for _timeWindowSize
   * @param timeWindowSize
   */
  void setTimeWindowSize(double timeWindowSize);

private:
  logging::Logger _log{"cplschemes::SerialCouplingSchemes"};

  /// Determines, if the time window size is set by the participant.
  bool _participantSetsTimeWindowSize = false;

  /// Determines, if the time window size is received by the participant.
  bool _participantReceivesTimeWindowSize = false;

  /// Sends time window size, if this participant is the one to send
  void sendTimeWindowSize();

  /// Receives and sets the time window size, if this participant is the one to receive
  void receiveAndSetTimeWindowSize();

  /// @copydoc cplscheme::BaseCouplingScheme::exchangeInitialData()
  void exchangeInitialData() override final;

  /// Exchanges the first set of data between the participants of the SerialCouplingSchemes
  void exchangeFirstData() override final;

  /**
   * @brief Exchanges the second set of data between the participants of the SerialCouplingSchemes
   */
  void exchangeSecondData() override;

  const DataMap &getAccelerationData() override final;
};

} // namespace cplscheme
} // namespace precice
