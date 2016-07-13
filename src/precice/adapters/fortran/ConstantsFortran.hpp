#ifndef PRECICE_ADAPTERS_FORTRAN_CONSTANTS_HPP_
#define PRECICE_ADAPTERS_FORTRAN_CONSTANTS_HPP_

#ifdef __cplusplus
extern"C" {
#endif

/**
 * @brief Standard name for preCICE configuration file.
 *
 * Fortran syntax:
 * precicef_name_config( CHARACTER nameConfig(*) )
 */
void precicef_name_config_(
  char* nameConfig,
  int   lengthNameConfig );

/**
 * @brief Name of action for writing iteration checkpoint.
 *
 * Fortran syntax:
 * precicef_action_write_iter_checkpoint( CHARACTER nameAction(*) )
 */
void precicef_action_write_iter_checkp_(
  char* nameAction,
  int   lengthNameAction );

/**
 * @brief Name of action for writing initial data.
 *
 * FortranSyntax:
 * precicef_action_write_initial_data( CHARACTER nameAction(*) )
 */
void precicef_action_write_initial_data_(
  char* nameAction,
  int   lengthNameAction );

/**
 * @brief Name of action for reading iteration checkpoint.
 *
 * Fortran syntax:
 * precicef_action_read_iter_checkpoint( CHARACTER nameAction(*) )
 */
void precicef_action_read_iter_checkp_(
  char* nameAction,
  int   lengthNameAction );

/**
 * @brief Name of action for writing simulation checkpoint.
 *
 * Fortran syntax:
 * precicef_action_write_sim_checkpoint( CHARACTER nameAction(*) )
 */
void precicef_action_write_sim_checkp_(
  char* nameAction,
  int   lengthNameAction );

/**
 * @brief Name of action for reading simulation checkpoint.
 *
 * Fortran syntax:
 * precicef_action_read_sim_checkpoint( CHARACTER nameAction(*) )
 */
void precicef_action_read_sim_checkp_(
  char* nameAction,
  int   lengthNameAction );

#ifdef __cplusplus
}
#endif

#endif /* PRECICE_ADAPTERS_FORTRAN_CONSTANTS_HPP_ */
