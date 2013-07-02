/* libdogma
 * Copyright (C) 2013 Romain "Artefact2" Dalmaso <artefact2@gmail.com>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#pragma once
#ifndef _DOGMA_EXTRA_H
#define _DOGMA_EXTRA_H 1

#include <dogma.h>

/* -------- Rationale -------- */

/* This header contains methods which arguably may or may not belong
 * in a true dogma engine. Most of the methods below were made for
 * speed concerns or for ease of use in applications. Whether you make
 * use of them or not is your choice. */





/* -------- Data types -------- */

#define DOGMA_AFFECTOR_PENALIZED (1 << 0)
#define DOGMA_AFFECTOR_SINGLETON (1 << 1)

struct dogma_simple_affector_s {
	dogma_typeid_t id; /* typeid of the affector */
	dogma_attributeid_t destid; /* attributeid of the affected
	                             * attribute */

	double value;
	char operator; /* '=', '+', '-' or '*' depending on operation */

	uint8_t order; /* When computing the final value, these operations
	                * are applied by ascending order */
	uint8_t flags;  /* An OR'd list of DOGMA_AFFECTOR_* constants */
};
typedef struct dogma_simple_affector_s dogma_simple_affector_t;





/* -------- Utility functions -------- */

/* Get a list of all affectors of a certain location. You MUST call
 * dogma_free_affector_list() after you are done using the list. */
int dogma_get_affectors(dogma_context_t*, dogma_location_t, dogma_simple_affector_t**, size_t*);

/* Free the list of affectors generated by dogma_get_affectors(). */
int dogma_free_affector_list(dogma_simple_affector_t*);



/* Checks if a certain location has effects only enabled in the active
 * state or higher. */
int dogma_location_has_active_effects(dogma_context_t*, dogma_location_t, bool*);

/* Checks if a certain location has effects only enabled in the
 * overloaded state or higher. */
int dogma_location_has_overload_effects(dogma_context_t*, dogma_location_t, bool*);




#endif
