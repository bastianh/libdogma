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

#include "dogma.h"
#include "dogma-extra.h"
#include "dogma_internal.h"
#include "attribute.h"
#include "tables.h"
#include <assert.h>

#define DOGMA_Active_OR_HIGHER (DOGMA_STATE_Active &	\
                                (~(DOGMA_STATE_Offline | DOGMA_STATE_Online)))
#define DOGMA_Overload_OR_HIGHER (DOGMA_STATE_Overloaded &			\
                                  (~(DOGMA_STATE_Offline | DOGMA_STATE_Online | DOGMA_STATE_Active)))

int dogma_get_affectors(dogma_context_t* ctx, dogma_location_t loc, dogma_simple_affector_t** list, size_t* len) {
	size_t num_affectors = 0, i = 0;
	dogma_simple_affector_t* affectors;
	dogma_env_t* current_env;
	dogma_env_t* loc_env;
	dogma_fleet_context_t* fleet;
	array_t *modifiers1, *modifiers2;
	dogma_modifier_t** modifier;
	key_t attributeid;
	key_t assoctype;
	key_t index;
	bool applicable;

	DOGMA_ASSUME_OK(dogma_get_location_env(ctx, loc, &loc_env));

	/* Code below is somewhat similar to the traverse code in attribute.c */

	/* Pass 1: count the number of affecting modifiers */
	current_env = loc_env;

	while(current_env != NULL) {
		attributeid = 0;
		JLF(modifiers1, current_env->modifiers, attributeid);

		while(modifiers1 != NULL) {
			assoctype = 0;
			JLF(modifiers2, *modifiers1, assoctype);

			while(modifiers2 != NULL) {
				index = 0;
				JLF(modifier, *modifiers2, index);

				while(modifier != NULL) {
					DOGMA_ASSUME_OK(dogma_modifier_is_applicable(ctx, loc_env, *modifier, &applicable));
					if(applicable) {
						++num_affectors;
					}

					JLN(modifier, *modifiers2, index);
				}

				JLN(modifiers2, *modifiers1, assoctype);
			}

			JLN(modifiers1, current_env->modifiers, attributeid);
		}

		current_env = current_env->parent;
	}

	fleet = ctx->fleet;
	while(fleet != NULL) {
		if(fleet->booster == NULL || fleet->commander == NULL) {
			break;
		}

		attributeid = 0;
		JLF(modifiers1, fleet->booster->gang->modifiers, attributeid);

		while(modifiers1 != NULL) {
			assoctype = 0;
			JLF(modifiers2, *modifiers1, assoctype);

			while(modifiers2 != NULL) {
				index = 0;
				JLF(modifier, *modifiers2, index);

				while(modifier != NULL) {
					DOGMA_ASSUME_OK(dogma_modifier_is_applicable(ctx, loc_env, *modifier, &applicable));
					if(applicable) {
						++num_affectors;
					}

					JLN(modifier, *modifiers2, index);
				}

				JLN(modifiers2, *modifiers1, assoctype);
			}

			JLN(modifiers1, fleet->booster->gang->modifiers, attributeid);
		}

		fleet = fleet->parent;
	}

	/* Step 2: allocate the list */
	affectors = malloc(num_affectors * sizeof(dogma_simple_affector_t));
	dogma_modifier_t* affecting_modifiers[num_affectors];

	/* Step 3: fill the affecting_modifiers array */
	current_env = loc_env;

	while(current_env != NULL) {
		attributeid = 0;
		JLF(modifiers1, current_env->modifiers, attributeid);

		while(modifiers1 != NULL) {
			assoctype = 0;
			JLF(modifiers2, *modifiers1, assoctype);

			while(modifiers2 != NULL) {
				index = 0;
				JLF(modifier, *modifiers2, index);

				while(modifier != NULL) {
					DOGMA_ASSUME_OK(dogma_modifier_is_applicable(ctx, loc_env, *modifier, &applicable));
					if(applicable) {
						affecting_modifiers[i++] = *modifier;
					}

					JLN(modifier, *modifiers2, index);
				}

				JLN(modifiers2, *modifiers1, assoctype);
			}

			JLN(modifiers1, current_env->modifiers, attributeid);
		}

		current_env = current_env->parent;
	}

	fleet = ctx->fleet;
	while(fleet != NULL) {
		if(fleet->booster == NULL || fleet->commander == NULL) {
			break;
		}

		attributeid = 0;
		JLF(modifiers1, fleet->booster->gang->modifiers, attributeid);

		while(modifiers1 != NULL) {
			assoctype = 0;
			JLF(modifiers2, *modifiers1, assoctype);

			while(modifiers2 != NULL) {
				index = 0;
				JLF(modifier, *modifiers2, index);

				while(modifier != NULL) {
					DOGMA_ASSUME_OK(dogma_modifier_is_applicable(ctx, loc_env, *modifier, &applicable));
					if(applicable) {
						affecting_modifiers[i++] = *modifier;
					}

					JLN(modifier, *modifiers2, index);
				}

				JLN(modifiers2, *modifiers1, assoctype);
			}

			JLN(modifiers1, fleet->booster->gang->modifiers, attributeid);
		}

		fleet = fleet->parent;
	}

	assert(i == num_affectors);

	/* Step 4: fill the list */
	for(i = 0; i < num_affectors; ++i) {
		dogma_simple_affector_t* aff = &affectors[i];
		dogma_modifier_t* modifier = affecting_modifiers[i];

		aff->id = modifier->sourceenv->id;
		aff->destid = modifier->targetattribute;
		aff->order = modifier->assoctype;
		aff->flags = 0;

		if(modifier->penalized) aff->flags |= DOGMA_AFFECTOR_PENALIZED;
		if(modifier->singleton) aff->flags |= DOGMA_AFFECTOR_SINGLETON;

		switch(modifier->assoctype) {

		case DOGMA_ASSOC_PreAssignment:
		case DOGMA_ASSOC_PostAssignment:
			aff->operator = '=';
			aff->value = 0.0;
			break;

		case DOGMA_ASSOC_PostPercent:
		case DOGMA_ASSOC_PostMul:
		case DOGMA_ASSOC_PostDiv:
		case DOGMA_ASSOC_PreMul:
		case DOGMA_ASSOC_PreDiv:
			aff->operator = '*';
			aff->value = 1.0;
			break;

		case DOGMA_ASSOC_ModAdd:
			aff->operator = '+';
			aff->value = 0.0;
			break;

		case DOGMA_ASSOC_ModSub:
			aff->operator = '-';
			aff->value = 0.0;
			break;

		}

		DOGMA_ASSUME_OK(dogma_apply_modifier(ctx, modifier, &(aff->value)));
	}

	*list = affectors;
	*len = num_affectors;
	return DOGMA_OK;
}

int dogma_free_affector_list(dogma_simple_affector_t* list) {
	free(list);
	return DOGMA_OK;
}



int dogma_location_has_active_effects(dogma_context_t* ctx, dogma_location_t loc, bool* activable) {
	dogma_env_t* loc_env;
	array_t effects;
	const dogma_type_effect_t** te;
	const dogma_effect_t* e;
	key_t index = 0;

	DOGMA_ASSUME_OK(dogma_get_location_env(ctx, loc, &loc_env));
	DOGMA_ASSUME_OK(dogma_get_type_effects(loc_env->id, &effects));

	JLF(te, effects, index);
	while(te != NULL) {
		DOGMA_ASSUME_OK(dogma_get_effect((*te)->effectid, &e));
		if((DOGMA_Active_OR_HIGHER >> e->category) & 1) {
			*activable = true;
			return DOGMA_OK;
		}

		JLN(te, effects, index);
	}

	*activable = false;
	return DOGMA_OK;
}

int dogma_location_has_overload_effects(dogma_context_t* ctx, dogma_location_t loc, bool* overloadable) {
	dogma_env_t* loc_env;
	array_t effects;
	const dogma_type_effect_t** te;
	const dogma_effect_t* e;
	key_t index = 0;

	DOGMA_ASSUME_OK(dogma_get_location_env(ctx, loc, &loc_env));
	DOGMA_ASSUME_OK(dogma_get_type_effects(loc_env->id, &effects));

	JLF(te, effects, index);
	while(te != NULL) {
		DOGMA_ASSUME_OK(dogma_get_effect((*te)->effectid, &e));
		if((DOGMA_Overload_OR_HIGHER >> e->category) & 1) {
			*overloadable = true;
			return DOGMA_OK;
		}

		JLN(te, effects, index);
	}

	*overloadable = false;
	return DOGMA_OK;
}
