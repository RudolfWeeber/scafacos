/*
  Copyright (C) 2011-2012 Pedro Sanchez
  Copyright (C) 2016 Michael Hofmann

  This file is part of ScaFaCoS.

  ScaFaCoS is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  ScaFaCoS is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser Public License for more details.

  You should have received a copy of the GNU Lesser Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>. 
*/



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fcs_mmm1d.h"
#include "mmm1d/mmm1d.h"
#include "mmm1d/types.h"


static void swapf(fcs_float *a, fcs_float *b)
{ fcs_float tmp = *a; *a = *b; *b = tmp; }


#define MMM1D_CHECK_RETURN_RESULT(_h_, _f_)  do { \
  CHECK_HANDLE_RETURN_RESULT(_h_, _f_); \
  CHECK_METHOD_RETURN_RESULT(_h_, _f_, FCS_METHOD_MMM1D, "mmm1d"); \
  } while (0)

#define MMM1D_CHECK_RETURN_VAL(_h_, _f_, _v_)  do { \
  CHECK_HANDLE_RETURN_VAL(_h_, _f_, _v_); \
  CHECK_METHOD_RETURN_VAL(_h_, _f_, FCS_METHOD_MMM1D, "mmm1d", _v_); \
  } while (0)


/* initialization function for basic p3m parameters */
FCSResult fcs_mmm1d_init(FCS handle)
{
  MMM1D_CHECK_RETURN_RESULT(handle, __func__);

  handle->shift_positions = 1;

  handle->destroy = fcs_mmm1d_destroy;
  handle->set_parameter = fcs_mmm1d_set_parameter;
  handle->print_parameters = fcs_mmm1d_print_parameters;
  handle->tune = fcs_mmm1d_tune;
  handle->run = fcs_mmm1d_run;

  handle->mmm1d_param = malloc(sizeof(fcs_mmm1d_parameters_t));

  mmm1d_init(&handle->method_context, handle->communicator);

  return FCS_RESULT_SUCCESS;
}

FCSResult fcs_mmm1d_tune(FCS handle, 
		       fcs_int local_particles,
		       fcs_float *positions, fcs_float *charges)
{
  FCSResult result;
  
  MMM1D_CHECK_RETURN_RESULT(handle, __func__);

  /* Check box periodicity */
  const fcs_int *periodicity = fcs_get_periodicity(handle);
  if ((periodicity[0] ? 1 : 0) + (periodicity[1] ? 1 : 0) + (periodicity[2] ? 1 : 0) != 1)
    return fcs_result_create(FCS_ERROR_WRONG_ARGUMENT, __func__, "mmm1d requires exactly one axis with periodic boundary.");
  
  /* Check box shape */
  const fcs_float *a = fcs_get_box_a(handle);
  const fcs_float *b = fcs_get_box_b(handle);
  const fcs_float *c = fcs_get_box_c(handle);

  if (!fcs_is_orthogonal(a, b, c))
    return fcs_result_create(FCS_ERROR_WRONG_ARGUMENT, __func__, 
          "mmm1d requires the box to be orthorhombic.");

  if (!fcs_uses_principal_axes(a, b, c))
    return fcs_result_create(FCS_ERROR_WRONG_ARGUMENT, __func__, 
          "mmm1d requires the box vectors to be parallel to the principal axes.");

  /* setup unshuffled */
  for(fcs_int i = 0; i < 3; ++i) handle->mmm1d_param->shuffle[i] = i;
  fcs_float box_l[] = { a[0], b[1], c[2] };

  /* shuffle such that z is periodic */
  fcs_int periodic_c;
  for (periodic_c = 0; periodic_c < 2; ++periodic_c) {
    if (periodicity[periodic_c]) break;
  }
  handle->mmm1d_param->shuffle[periodic_c] = 2;
  handle->mmm1d_param->shuffle[2] = periodic_c;
  swapf(&box_l[periodic_c], &box_l[2]);

  mmm1d_set_box_a(handle->method_context, box_l[0]);
  mmm1d_set_box_b(handle->method_context, box_l[1]);
  mmm1d_set_box_c(handle->method_context, box_l[2]);

  /* Effectively, tune initializes the algorithm */
  result = mmm1d_tune(handle->method_context, 
        local_particles,
        positions, charges);
  
  return result;
}

/* internal mmm1d-specific run function */
FCSResult fcs_mmm1d_run(FCS handle, 
		      fcs_int local_particles,
		      fcs_float *positions, fcs_float *charges,
		      fcs_float *fields, fcs_float *potentials)
{
  MMM1D_CHECK_RETURN_RESULT(handle, __func__);

  fcs_mmm1d_tune(handle, local_particles, positions, charges);
  
  fcs_int max_local_particles = fcs_get_max_local_particles(handle);
  if (local_particles > max_local_particles) max_local_particles = local_particles;

  fcs_float *shuffled_positions = (fcs_float *)malloc(3*sizeof(fcs_float)*max_local_particles);

  /* shuffle positions */
  fcs_int *shuffle = handle->mmm1d_param->shuffle;
  for(fcs_int i = 0; i < local_particles; ++i) {
    for(fcs_int j = 0; j < 3; ++j) {
      shuffled_positions[3*i + j] = positions[3*i + shuffle[j]];
    }
  }

  mmm1d_run(handle->method_context,
	  local_particles, max_local_particles, shuffled_positions, charges, fields, potentials);

  /* unshuffle fields */
  if (fields) {
    for(fcs_int i = 0; i < local_particles; ++i) {
      fcs_float tmp[3];
      for(fcs_int j = 0; j < 3; ++j) {
        tmp[shuffle[j]] = fields[3*i + j];
      }
      for(fcs_int j = 0; j < 3; ++j) {
        fields[3*i + j] = tmp[j];
      }
    }
  }

  free(shuffled_positions);

  return FCS_RESULT_SUCCESS;
}

/* clean-up function for mmm1d */
FCSResult fcs_mmm1d_destroy(FCS handle)
{
  MMM1D_CHECK_RETURN_RESULT(handle, __func__);

  mmm1d_destroy(handle->method_context);

  return FCS_RESULT_SUCCESS;
}

/******************************************************************************************************
 *
 *            Setter and Getter functions for mmm1d parameters
 *
 ******************************************************************************************************/
FCSResult fcs_mmm1d_set_far_switch_radius(FCS handle, fcs_float rad2) {

  MMM1D_CHECK_RETURN_RESULT(handle, __func__);

  mmm1d_set_far_switch_radius_2(handle->method_context, rad2*rad2);

  return FCS_RESULT_SUCCESS;
}

FCSResult fcs_mmm1d_get_far_switch_radius(FCS handle, fcs_float *rad2) {

  MMM1D_CHECK_RETURN_RESULT(handle, __func__);

  mmm1d_get_far_switch_radius_2(handle->method_context, rad2);

  *rad2 = sqrt(*rad2);

  return FCS_RESULT_SUCCESS;
}

FCSResult fcs_mmm1d_set_maxPWerror(FCS handle, fcs_float maxPWerror) {
  
  MMM1D_CHECK_RETURN_RESULT(handle, __func__);

  mmm1d_set_maxPWerror(handle->method_context, maxPWerror);

  return FCS_RESULT_SUCCESS;
}

FCSResult fcs_mmm1d_get_maxPWerror(FCS handle, fcs_float *maxPWerror) {

  MMM1D_CHECK_RETURN_RESULT(handle, __func__);

  mmm1d_get_maxPWerror(handle->method_context, maxPWerror);

  return FCS_RESULT_SUCCESS;
}

FCSResult fcs_mmm1d_set_bessel_cutoff(FCS handle, fcs_int cutoff) {

  MMM1D_CHECK_RETURN_RESULT(handle, __func__);

  mmm1d_set_bessel_cutoff(handle->method_context, cutoff);

  return FCS_RESULT_SUCCESS;
}

FCSResult fcs_mmm1d_get_bessel_cutoff(FCS handle, fcs_int *cutoff) {

  MMM1D_CHECK_RETURN_RESULT(handle, __func__);

  mmm1d_get_bessel_cutoff(handle->method_context, cutoff);

  return FCS_RESULT_SUCCESS;
}

FCSResult fcs_mmm1d_set_parameter(FCS handle, fcs_bool continue_on_errors, char **current, char **next, fcs_int *matched)
{
  char *param = *current;
  char *cur = *next;

  *matched = 0;

  FCS_PARSE_IF_PARAM_THEN_FUNC1_GOTO_NEXT("mmm1d_far_switch_radius", mmm1d_set_far_switch_radius, FCS_PARSE_VAL(fcs_float));
  FCS_PARSE_IF_PARAM_THEN_FUNC1_GOTO_NEXT("mmm1d_bessel_cutoff",     mmm1d_set_bessel_cutoff,     FCS_PARSE_VAL(fcs_int));
  FCS_PARSE_IF_PARAM_THEN_FUNC1_GOTO_NEXT("mmm1d_maxPWerror",        mmm1d_set_maxPWerror,        FCS_PARSE_VAL(fcs_float));

  return FCS_RESULT_SUCCESS;

next_param:
  *current = param;
  *next = cur;

  *matched = 1;

  return FCS_RESULT_SUCCESS;
}

FCSResult fcs_mmm1d_print_parameters(FCS handle)
{
  fcs_float radius;
  fcs_float PWerror;
  fcs_int cutoff;

  MMM1D_CHECK_RETURN_RESULT(handle, __func__);

  fcs_mmm1d_get_far_switch_radius(handle, &radius);
  fcs_mmm1d_get_bessel_cutoff(handle, &cutoff);
  fcs_mmm1d_get_maxPWerror(handle, &PWerror);

  printf("mmm1d bessel cutoff: %" FCS_LMOD_INT "d\n", cutoff);
  printf("mmm1d far switch radius: %e\n", radius);
  printf("mmm1d maximum PWerror: %e\n", PWerror);

  return FCS_RESULT_SUCCESS;
}
