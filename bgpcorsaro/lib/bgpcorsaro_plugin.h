/*
 * This file is part of bgpstream
 *
 * CAIDA, UC San Diego
 * bgpstream-info@caida.org
 *
 * Copyright (C) 2012 The Regents of the University of California.
 * Authors: Alistair King, Chiara Orsini
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __BGPCORSARO_PLUGIN_H
#define __BGPCORSARO_PLUGIN_H

#include "bgpcorsaro_int.h"

/** @file
 *
 * @brief Header file dealing with the bgpcorsaro plugin manager
 *
 * @author Alistair King
 *
 */

/** Convenience macro that defines all the function prototypes for the
 * bgpcorsaro
 * plugin API
 *
 * @todo split this into bgpcorsaro-out and bgpcorsaro-in macros
 */
#define BGPCORSARO_PLUGIN_GENERATE_PROTOS(plugin)                              \
  bgpcorsaro_plugin_t *plugin##_alloc();                                       \
  int plugin##_init_output(struct bgpcorsaro *bgpcorsaro);                     \
  int plugin##_close_output(struct bgpcorsaro *bgpcorsaro);                    \
  int plugin##_start_interval(struct bgpcorsaro *bgpcorsaro,                   \
                              struct bgpcorsaro_interval *int_start);          \
  int plugin##_end_interval(struct bgpcorsaro *bgpcorsaro,                     \
                            struct bgpcorsaro_interval *int_end);              \
  int plugin##_process_record(struct bgpcorsaro *bgpcorsaro,                   \
                              struct bgpcorsaro_record *record);

/** Convenience macro that defines all the function pointers for the bgpcorsaro
 * plugin API
 */
#define BGPCORSARO_PLUGIN_GENERATE_PTRS(plugin)                                \
  plugin##_init_output, plugin##_close_output, plugin##_start_interval,        \
    plugin##_end_interval, plugin##_process_record

/** Convenience macro that defines all the 'remaining' blank fields in a
 * bgpcorsaro
 *  plugin object
 *
 *  This becomes useful if we add more fields to the end of the plugin
 *  structure, because each plugin does not need to be updated in order to
 *  correctly 'zero' these fields.
 */
#define BGPCORSARO_PLUGIN_GENERATE_TAIL                                        \
  NULL,  /* next */                                                            \
    0,   /* argc */                                                            \
    NULL /* argv */

/** Convenience macro to cast the state pointer in the plugin
 *
 * Plugins should use extend this macro to provide access to their state
 */
#define BGPCORSARO_PLUGIN_STATE(bgpcorsaro, type, id)                          \
  ((struct bgpcorsaro_##type##_state_t                                         \
      *)((bgpcorsaro)->plugin_manager->plugins_state[(id)-1]))

/** Convenience macro to get this plugin from bgpcorsaro
 *
 * Plugins should use extend this macro to provide access to themself
 */
#define BGPCORSARO_PLUGIN_PLUGIN(bgpcorsaro, id)                               \
  ((bgpcorsaro)->plugin_manager->plugins[(id)-1])

/** A unique identifier for a plugin, used when writing binary data
 *
 * @note this identifier does not affect the order in which plugins are passed
 *       records. Plugin precedence is determined either by the order of the
 *       ED_WITH_PLUGIN macros in configure.ac, or by the order of the plugins
 *       that have been explicitly enabled using \ref bgpcorsaro_enable_plugin
 */
typedef enum bgpcorsaro_plugin_id {
  /** Prefix Monitor plugin */
  BGPCORSARO_PLUGIN_ID_PFXMONITOR = 1,

  /** Pacifier plugin */
  BGPCORSARO_PLUGIN_ID_PACIFIER = 2,

  /** AS Monitor plugin */
  BGPCORSARO_PLUGIN_ID_ASMONITOR = 3,

  /** Maximum plugin ID assigned */
  BGPCORSARO_PLUGIN_ID_MAX = BGPCORSARO_PLUGIN_ID_ASMONITOR
} bgpcorsaro_plugin_id_t;

/** An bgpcorsaro packet processing plugin */
/* All functions should return -1, or NULL on failure */
typedef struct bgpcorsaro_plugin {
  /** The name of this plugin used in the ascii output and eventually to allow
   * plugins to be enabled and disabled */
  const char *name;

  /** The version of this plugin */
  const char *version;

  /** The bgpcorsaro plugin id for this plugin */
  const bgpcorsaro_plugin_id_t id;

  /** Initializes an output file using the plugin
   *
   * @param bgpcorsaro	The bgpcorsaro output to be initialized
   * @return 0 if successful, -1 in the event of error
   */
  int (*init_output)(struct bgpcorsaro *bgpcorsaro);

  /** Concludes an output file and cleans up the plugin data.
   *
   * @param bgpcorsaro 	The output file to be concluded
   * @return 0 if successful, -1 if an error occurs
   */
  int (*close_output)(struct bgpcorsaro *bgpcorsaro);

  /** Starts a new interval
   *
   * @param bgpcorsaro 	The output object to start the interval on
   * @param int_start   The start structure for the interval
   * @return 0 if successful, -1 if an error occurs
   */
  int (*start_interval)(struct bgpcorsaro *bgpcorsaro,
                        bgpcorsaro_interval_t *int_start);

  /** Ends an interval
   *
   * @param bgpcorsaro 	The output object end the interval on
   * @param int_end     The end structure for the interval
   * @return 0 if successful, -1 if an error occurs
   *
   * This is likely when the plugin will write it's data to it's output file
   */
  int (*end_interval)(struct bgpcorsaro *bgpcorsaro,
                      bgpcorsaro_interval_t *int_end);

  /**
   * Process a record
   *
   * @param bgpcorsaro  The output object to process the record for
   * @param packet      The packet to process
   * @return 0 if successful, -1 if an error occurs
   *
   * This is where the magic happens, the plugin should do any processing needed
   * for this record and update internal state and optionally update the
   * bgpcorsaro_record_state object to pass on discoveries to later plugins.
   */
  int (*process_record)(struct bgpcorsaro *bgpcorsaro,
                        struct bgpcorsaro_record *record);

  /** Next pointer. Used by the plugin manager. */
  struct bgpcorsaro_plugin *next;

  /** Count of arguments in argv */
  int argc;

  /** Array of plugin arguments.
   * This is populated by the plugin manager in bgpcorsaro_plugin_enable_plugin.
   * It is the responsibility of the plugin to do something sensible with it
   */
  char **argv;

#ifdef WITH_PLUGIN_TIMING
  /* variables that hold timing information for this plugin */

  /** Number of usec spent in the init_output function */
  uint64_t init_output_usec;

  /** Number of usec spent in the process_packet or process_flowtuple
      functions */
  uint64_t process_packet_usec;

  /** Number of usec spent in the start_interval function */
  uint64_t start_interval_usec;

  /** Number of usec spent in the end_interval function */
  uint64_t end_interval_usec;
#endif

} bgpcorsaro_plugin_t;

/** Holds the metadata for the plugin manager
 *
 * This allows both bgpcorsaro_t and bgpcorsaro_in_t objects to use the plugin
 * infrastructure without needing to pass references to themselves
 */
typedef struct bgpcorsaro_plugin_manager {
  /** An array of plugin ids that have been enabled by the user
   *
   * If this array is NULL, then assume all have been enabled.
   */
  uint16_t *plugins_enabled;

  /** The number of plugin ids in the plugins_enabled array */
  uint16_t plugins_enabled_cnt;

  /** A pointer to the array of plugins in use */
  bgpcorsaro_plugin_t **plugins;

  /** A pointer to the first plugin in the list */
  bgpcorsaro_plugin_t *first_plugin;

  /** A pointer to the array of plugin states */
  void **plugins_state;

  /** The number of active plugins */
  uint16_t plugins_cnt;

  /** A pointer to the logfile to use */
  iow_t *logfile;

} bgpcorsaro_plugin_manager_t;

/** Initialize the plugin manager and all in-use plugins
 *
 * @return A pointer to the plugin manager state or NULL if an error occurs
 */
bgpcorsaro_plugin_manager_t *bgpcorsaro_plugin_manager_init();

/** Start the plugin manager
 *
 * @param manager  The manager to start
 */
int bgpcorsaro_plugin_manager_start(bgpcorsaro_plugin_manager_t *manager);

/** Free the plugin manager and all in-use plugins
 *
 * @param manager  The plugin manager to free
 *
 * @note the plugins registered with the manager MUST have already
 * been closed (either plugin->close_output or plugin->close_input).
 * Also, the logfile is NOT closed, as it is assumed to be shared with
 * another object (bgpcorsaro_t or bgpcorsaro_in_t).
 */
void bgpcorsaro_plugin_manager_free(bgpcorsaro_plugin_manager_t *manager);

/** Attempt to retrieve a plugin by id
 *
 * @param manager   The plugin manager to search with
 * @param id        The id of the plugin to get
 * @return the plugin corresponding to the id if found, NULL otherwise
 */
bgpcorsaro_plugin_t *
bgpcorsaro_plugin_get_by_id(bgpcorsaro_plugin_manager_t *manager, int id);

/** Attempt to retrieve a plugin by name
 *
 * @param manager   The plugin manager to search with
 * @param name      The name of the plugin to get
 * @return the plugin corresponding to the name if found, NULL otherwise
 */
bgpcorsaro_plugin_t *
bgpcorsaro_plugin_get_by_name(bgpcorsaro_plugin_manager_t *manager,
                              const char *name);

/** Retrieve the next plugin in the list
 *
 * @param manager   The plugin manager to get the next plugin for
 * @param plugin    The current plugin
 * @return the plugin which follows the current plugin, NULL if the end of the
 * plugin list has been reached. If plugin is NULL, the first plugin will be
 * returned.
 */
bgpcorsaro_plugin_t *
bgpcorsaro_plugin_next(bgpcorsaro_plugin_manager_t *manager,
                       bgpcorsaro_plugin_t *plugin);

/** Register the state for a plugin
 *
 * @param manager   The plugin manager to register state with
 * @param plugin    The plugin to register state for
 * @param state     A pointer to the state object to register
 */
void bgpcorsaro_plugin_register_state(bgpcorsaro_plugin_manager_t *manager,
                                      bgpcorsaro_plugin_t *plugin, void *state);

/** Free the state for a plugin
 *
 * @param manager   The plugin manager associated with the state
 * @param plugin    The plugin to free state for
 */
void bgpcorsaro_plugin_free_state(bgpcorsaro_plugin_manager_t *manager,
                                  bgpcorsaro_plugin_t *plugin);

/** Get the name of a plugin given it's ID number
 *
 * @param manager    The plugin manager associated with the state
 * @param id         The plugin id to retrieve the name for
 * @return the name of the plugin as a string, NULL if no plugin matches
 * the given id
 */
const char *
bgpcorsaro_plugin_get_name_by_id(bgpcorsaro_plugin_manager_t *manager, int id);

/** Determine whether this plugin is enabled for use
 *
 * @param manager    The plugin manager associated with the state
 * @param plugin     The plugin to check the status of
 * @return 1 if the plugin is enabled, 0 if it is disabled
 *
 *  A plugin is enabled either explicitly by the bgpcorsaro_enable_plugin()
 *  function, or implicitly because all plugins are enabled.
 */
int bgpcorsaro_plugin_is_enabled(bgpcorsaro_plugin_manager_t *manager,
                                 bgpcorsaro_plugin_t *plugin);

/** Attempt to enable a plugin by its name
 *
 * @param manager      The plugin manager associated with the state
 * @param plugin_name  The name of the plugin to enable
 * @param plugin_args  The arguments to pass to the plugin (for config)
 * @return 0 if the plugin was successfully enabled, -1 otherwise
 *
 * See bgpcorsaro_enable_plugin for more details.
 */
int bgpcorsaro_plugin_enable_plugin(bgpcorsaro_plugin_manager_t *manager,
                                    const char *plugin_name,
                                    const char *plugin_args);

#endif /* __BGPCORSARO_PLUGIN_H */
