#pragma once

#include "com/SharedPointer.hpp"
#include <string>
#include <vector>
#include <map>

namespace precice {
namespace cplscheme {

/**
 * @brief Interface for all coupling schemes.
 *
 * ! General description
 * A coupling scheme computes the actions to be done by the coupled participants
 * (solvers) in time. It provides interface functions to setup, advance and
 * shutdown the coupling scheme and interface functions to query the state of
 * the coupling scheme and required actions of the participants.
 *
 * ! Usage
 * -# create an object of a concrete coupling scheme class
 *    (ExplicitCouplingScheme, e.g.)
 * -# add all meshes holding data to the coupling scheme by addMesh()
 * -# configure the object by adding subclass specific information
 * -# start the coupling scheme with initialize(), where the name of the local
 *    participant, i.e. the participant using the coupling scheme object, is
 *    needed
 * -# retrieve necessary information about sent/received data and the state of
 *    the coupled simulation
 * -# query and fulfill required actions
 * -# compute data to be sent (possibly taking into account received data from
 *    initialize())
 * -# advance the coupling scheme with advance(); where the maximum timestep
 *    length needs to be obeyed
 * -# ....
 * -# when the method isCouplingOngoing() returns false, call finalize() to
 *    stop the coupling scheme
 */
class CouplingScheme
{
public:

  /// Does not define a time limit for the coupled simulation.
  static const double UNDEFINED_TIME;

  /// Does not define a timestep limit for the coupled simulation.
  static const int UNDEFINED_TIMESTEPS;

  /// To be used, when the coupling timestep length is determined dynamically during the coupling.
  static const double UNDEFINED_TIMESTEP_LENGTH;

  CouplingScheme& operator=(CouplingScheme &&) = delete;

  virtual ~CouplingScheme() {}

  /**
   * @brief Initializes the coupling scheme and establishes a communiation
   *        connection to the coupling partner.
   */
  virtual void initialize (
    double startTime,
    int    startTimesteps ) =0;

  /// Returns true, if initialize has been called.
  virtual bool isInitialized() const =0;

  /**
   * @brief Initializes the data for first implicit coupling scheme iteration.
   *
   * Has to be called after initialize() and before advance().
   * If this method is not used, the first participant has zero initial values
   * for its read data, before receiving data in advance(). If non-zero values
   * are needed, this has to be configured in the coupling-scheme XML
   * exchange-data tags. This method can nevertheless also be called if no
   * initialization is necessary. Then it is simply skipped.
   * It has to be called after initialize() and before
   * advance(). The second participant has to write the initial data values
   * to preCICE after initialize() and before initializeData().
   */
  virtual void initializeData() =0;

  /// @brief Adds newly computed time. Has to be called before every advance.
  virtual void addComputedTime(double timeToAdd) =0;

  /**
   * @brief Exchanges data and updates the state of the coupling scheme.
   *
   * @pre initialize() has been called.
   *
   * Does not necessarily advances in time.
   */
  virtual void advance() =0;

  /// Finalizes the coupling and disconnects communication.
  virtual void finalize() =0;

  /// Returns list of all coupling partners.
  virtual std::vector<std::string> getCouplingPartners() const =0;

  /*
   * @brief Returns true, if data will be exchanged when calling advance().
   *
   * Also returns true after the last call of advance() at the end of the
   * simulation.
   *
   * @param lastSolverTimestepLength [IN] The length of the last timestep
   *        computed by the solver calling willDataBeExchanged().
   */
  virtual bool willDataBeExchanged(double lastSolverTimestepLength) const =0;

  /// @brief Returns true, if data has been exchanged in last call of advance().
  /// actually, this only means that data has been received, data is always sent
  virtual bool hasDataBeenExchanged() const =0;

  /// Returns the currently computed time of the coupling scheme.
  virtual double getTime() const =0;

  /// Returns the currently computed timesteps of the coupling scheme.
  virtual int getTimesteps() const =0;

  /// Returns the maximal time to be computed.
  virtual double getMaxTime() const =0;

  /// Returns the maximal timesteps to be computed.
  virtual int getMaxTimesteps() const =0;

  //
  // @brief Returns current subiteration number in timestep.
  //
  //virtual int getSubIteration() const =0;

  /// Returns true, if timestep length is prescribed by the cpl scheme.
  virtual bool hasTimestepLength() const =0;

  /**
   * @brief Returns the timestep length, if one is given by the coupling scheme.
   *
   * An assertion is thrown, if no valid timestep is given. Check with
   * hasTimestepLength().
   */
  virtual double getTimestepLength() const =0;


  /// Defaults to false, i.e., no multilevel PP
  virtual bool isCoarseModelOptimizationActive()
  {
    return false;
  }

  /**
   * @brief Returns the remaining timestep length of the current time step.
   *
   * This is not necessarily the timestep length limit the solver has to obeye
   * which is returned by getNextTimestepMaxLength().
   *
   * If no timestep length is precribed by the coupling scheme, always 0.0 is
   * returned.
   */
  virtual double getThisTimestepRemainder() const =0;

  /// Returns part of the current timestep that has been computed already.
  virtual double getComputedTimestepPart() const =0;

  /**
   * @brief Returns the maximal length of the next timestep to be computed.
   *
   * If no timestep length is prescribed by the coupling scheme, always the
   * maximal double accuracy floating point number value is returned.
   */
  virtual double getNextTimestepMaxLength() const =0;

  //
  // @brief Returns the number of valid digits when compare times.
  //
  //virtual int getValidDigits() const =0;

  /// Returns true, when the coupled simulation is still ongoing.
  virtual bool isCouplingOngoing() const =0;

  /// Returns true, when the accessor can advance to the next timestep.
  virtual bool isCouplingTimestepComplete() const =0;

  /// Returns true, if the given action has to be performed by the accessor.
  virtual bool isActionRequired(const std::string& actionName) const =0;

  /// Tells the coupling scheme that the accessor has performed the given action.
  virtual void performedAction(const std::string& actionName) =0;

  /// Sets an action required to be performed by the accessor.
  virtual void requireAction(const std::string& actionName) =0;

  /// Returns a string representation of the current coupling state.
  virtual std::string printCouplingState() const =0;

  /**
   * @brief Send the state of the coupling scheme to another remote scheme.
   *
   * Used in client-server approach for parallel solvers. There, the solver
   * interface does hold a coupling scheme with no data but state. The state
   * is transferred between the solver coupling scheme and the server coupling
   * scheme via sendState and receiveState.
   */
  virtual void sendState (
    com::PtrCommunication communication,
    int                   rankReceiver ) =0;

  /**
   * @brief Receive the state of the coupling scheme from another remote scheme.
   *
   * Used in client-server approach for parallel solvers. There, the solver
   * interface does hold a coupling scheme with no data but state. The state
   * is transferred between the solver coupling scheme and the server coupling
   * scheme via sendState and receiveState.
   */
  virtual void receiveState (
    com::PtrCommunication communication,
    int                   rankSender ) =0;

};

}} // namespace precice, cplscheme

