/*
 * This file is part of bgpstream
 *
 * Copyright (C) 2015 The Regents of the University of California.
 * Authors: Alistair King, Chiara Orsini
 *
 * All rights reserved.
 *
 * This code has been developed by CAIDA at UC San Diego.
 * For more information, contact bgpstream-info@caida.org
 *
 * This source code is proprietary to the CAIDA group at UC San Diego and may
 * not be redistributed, published or disclosed without prior permission from
 * CAIDA.
 *
 * Report any bugs, questions or comments to bgpstream-info@caida.org
 *
 */

#ifndef _BGPDUMP_PROCESS_H
#define _BGPDUMP_PROCESS_H

#include "bgpdump_lib.h"

/** Print a BGPDUMP_ENTRY record to stdout in the same way that the RIPE bgpdump
 * tool does.
 *
 * @param entry         pointer to a BGP Dump entry
 */
void bgpdump_print_entry(BGPDUMP_ENTRY *entry);

#endif
