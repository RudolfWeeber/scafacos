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

#include "fcs_mmm2d.h"
#include "mmm2d/mmm2d.h"
#include "mmm2d/types.h"


#define MMM2D_CHECK_RETURN_RESULT(_h_, _f_)  do { \
  CHECK_HANDLE_RETURN_RESULT(_h_, _f_); \
  CHECK_METHOD_RETURN_RESULT(_h_, _f_, FCS_METHOD_MMM2D, "mmm2d"); \
  } while (0)

#define MMM2D_CHECK_RETURN_VAL(_h_, _f_, _v_)  do { \
  CHECK_HANDLE_RETURN_VAL(_h_, _f_, _v_); \
  CHECK_METHOD_RETURN_VAL(_h_, _f_, FCS_METHOD_MMM2D, "mmm2d", _v_); \
  } while (0)


/* initialization function for basic mmm2d parameters */
FCSResult fcs_mmm2d_init(FCS handle)
{
  MMM2D_CHECK_RETURN_RESULT(handle, __func__);

  handle->shift_positions = 1;

  handle->destroy = fcs_mmm2d_destroy;
  handle->set_parameter = fcs_mmm2d_set_parameter;
  handle->print_parameters = fcs_mmm2d_print_parameters;
  handle->tune = fcs_mmm2d_tune;
  handle->run = fcs_mmm2d_run;

  ///* @TODO: check here for unidimensional mpi grid (1,1,n)*/

  mmm2d_init(&handle->method_context, handle->communicator);

  return FCS_RESULT_SUCCESS;
}

/* internal p3m-specific tuning function */
FCSResult fcs_mmm2d_tune(FCS handle,
		       fcs_int local_particles,
		       fcs_float *positions, fcs_float *charges)
{
  FCSResult result;
  
  MMM2D_CHECK_RETURN_RESULT(handle, __func__);
  
  /* Check box periodicity */
  const fcs_int *periodicity = fcs_get_periodicity(handle);
  if (periodicity[0] != 1 ||
      periodicity[1] != 1 ||
      periodicity[2] != 0)
    return fcs_result_create(FCS_ERROR_WRONG_ARGUMENT, __func__, "mmm2d requires x and y-axis periodic boundaries.");
  
  /* Check box shape */
  ///@TODO: check for rectangular box geometry (any fcs adequate method?)
  const fcs_float *a = fcs_get_box_a(handle);
  const fcs_float *b = fcs_get_box_b(handle);
  const fcs_float *c = fcs_get_box_c(handle);
  
  /*
  if (!fcs_is_orthogonal(a, b, c))
    return fcs_result_create(FCS_ERROR_WRONG_ARGUMENT, __func__, 
          "p3m requires the box to be orthorhombic.");

  if (!fcs_uses_principal_axes(a, b, c))
    return fcs_result_create(FCS_ERROR_WRONG_ARGUMENT, __func__, 
          "p3m requires the box vectors to be parallel to the principal axes.");
  */
  
  mmm2d_set_box_a(handle->method_context, a[0]);
  mmm2d_set_box_b(handle->method_context, b[1]);
  mmm2d_set_box_c(handle->method_context, c[2]);
  
  /* Effectively, tune initializes the algorithm */
  result = mmm2d_tune(handle->method_context,
        local_particles,
        positions, charges);
  
  return result;
}

/* internal mmm2d-specific run function */
FCSResult fcs_mmm2d_run(FCS handle, 
		      fcs_int local_particles,
		      fcs_float *positions, fcs_float *charges,
		      fcs_float *fields, fcs_float *potentials)
{
  MMM2D_CHECK_RETURN_RESULT(handle, __func__);
  
  fcs_mmm2d_tune(handle, local_particles, positions, charges);
  
  fcs_int max_local_particles = fcs_get_max_local_particles(handle);
  if (local_particles > max_local_particles) max_local_particles = local_particles;

  mmm2d_run(handle->method_context, local_particles, max_local_particles, positions, charges, fields, potentials);
  
  return FCS_RESULT_SUCCESS;
}

/* clean-up function for p3m */
FCSResult fcs_mmm2d_destroy(FCS handle)
{
  MMM2D_CHECK_RETURN_RESULT(handle, __func__);

  mmm2d_destroy(handle->method_context);

  return FCS_RESULT_SUCCESS;
}

/******************************************************************************************************
 *
 *            Setter and Getter functions for mmm2d parameters
 *
 ******************************************************************************************************/

FCSResult fcs_mmm2d_set_far_cutoff(FCS handle, fcs_float cutoff) {

  MMM2D_CHECK_RETURN_RESULT(handle, __func__);

  mmm2d_set_far_cutoff(handle->method_context, cutoff);

  return FCS_RESULT_SUCCESS;
}


FCSResult fcs_mmm2d_get_far_cutoff(FCS handle, fcs_float *cutoff) {

  MMM2D_CHECK_RETURN_RESULT(handle, __func__);

  mmm2d_get_far_cutoff(handle->method_context, cutoff);

  return FCS_RESULT_SUCCESS;
}

FCSResult fcs_mmm2d_set_dielectric_contrasts(FCS handle, fcs_float delta_top, fcs_float delta_bot) {

  MMM2D_CHECK_RETURN_RESULT(handle, __func__);

  mmm2d_set_dielectric_contrasts(handle->method_context, delta_top, delta_bot);

  return FCS_RESULT_SUCCESS;
}

FCSResult fcs_mmm2d_get_dielectric_contrasts(FCS handle, fcs_float *delta_top, fcs_float *delta_bot) {

  MMM2D_CHECK_RETURN_RESULT(handle, __func__);

  mmm2d_get_dielectric_contrasts(handle->method_context, delta_top, delta_bot);

  return FCS_RESULT_SUCCESS;
}

FCSResult fcs_mmm2d_set_maxPWerror(FCS handle, fcs_float maxPWerror) {

  MMM2D_CHECK_RETURN_RESULT(handle, __func__);

  mmm2d_set_maxPWerror(handle->method_context, maxPWerror);

  return FCS_RESULT_SUCCESS;
}

FCSResult fcs_mmm2d_get_maxPWerror(FCS handle, fcs_float *maxPWerror) {

  MMM2D_CHECK_RETURN_RESULT(handle, __func__);

  mmm2d_get_maxPWerror(handle->method_context, maxPWerror);

  return FCS_RESULT_SUCCESS;
}

FCSResult fcs_mmm2d_set_layers_per_node(FCS handle, fcs_int n_layers) {

  MMM2D_CHECK_RETURN_RESULT(handle, __func__);

/*  fcs_int comm_size;
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  if (n_layers > comm_size) {
    printf("The number of layers, %d, can not be higher than the number of available processes, %d\n", n_layers, comm_size);
    return fcs_result_create(FCS_ERROR_WRONG_ARGUMENT,__func__, "The number of layers can not be higher than the number of available processes");
  }*/

  mmm2d_set_layers_per_node(handle->method_context, n_layers);

  return FCS_RESULT_SUCCESS;
}

FCSResult fcs_mmm2d_get_layers_per_node(FCS handle, fcs_int *n_layers) {

  MMM2D_CHECK_RETURN_RESULT(handle, __func__);

  mmm2d_get_layers_per_node(handle->method_context, n_layers);

  return FCS_RESULT_SUCCESS;
}

FCSResult fcs_mmm2d_require_total_energy(FCS handle, fcs_int flag) {

  MMM2D_CHECK_RETURN_RESULT(handle, __func__);

  mmm2d_require_total_energy(handle->method_context, flag);

  return FCS_RESULT_SUCCESS;
}

FCSResult fcs_mmm2d_get_total_energy(FCS handle, fcs_float *total_energy) {

  MMM2D_CHECK_RETURN_RESULT(handle, __func__);

  return mmm2d_get_total_energy(handle->method_context, total_energy);
}

FCSResult fcs_mmm2d_set_skin(FCS handle, fcs_float skin) {

  MMM2D_CHECK_RETURN_RESULT(handle, __func__);

  mmm2d_set_skin(handle->method_context, skin);

  return FCS_RESULT_SUCCESS;
}

FCSResult fcs_mmm2d_get_skin(FCS handle, fcs_float *skin) {

  MMM2D_CHECK_RETURN_RESULT(handle, __func__);

  mmm2d_get_skin(handle->method_context, skin);

  return FCS_RESULT_SUCCESS;
}

FCSResult fcs_mmm2d_set_parameter(FCS handle, fcs_bool continue_on_errors, char **current, char **next, fcs_int *matched)
{
  char *param = *current;
  char *cur = *next;

  *matched = 0;

  FCS_PARSE_IF_PARAM_THEN_FUNC1_GOTO_NEXT("mmm2d_maxPWerror",           mmm2d_set_maxPWerror,           FCS_PARSE_VAL(fcs_float));
  FCS_PARSE_IF_PARAM_THEN_FUNC1_GOTO_NEXT("mmm2d_far_cutoff",           mmm2d_set_far_cutoff,           FCS_PARSE_VAL(fcs_float));
  FCS_PARSE_IF_PARAM_THEN_FUNC2_GOTO_NEXT("mmm2d_dielectric_contrasts", mmm2d_set_dielectric_contrasts, FCS_PARSE_VAL(fcs_float), FCS_PARSE_VAL(fcs_float));
  FCS_PARSE_IF_PARAM_THEN_FUNC1_GOTO_NEXT("mmm2d_layers_per_node",      mmm2d_set_layers_per_node,      FCS_PARSE_VAL(fcs_int));
  FCS_PARSE_IF_PARAM_THEN_FUNC1_GOTO_NEXT("mmm2d_skin",                 mmm2d_set_skin,                 FCS_PARSE_VAL(fcs_float));
  FCS_PARSE_IF_PARAM_THEN_FUNC1_GOTO_NEXT("",                           mmm2d_require_total_energy,     FCS_PARSE_VAL(fcs_int));

  return FCS_RESULT_SUCCESS;

next_param:
  *current = param;
  *next = cur;

  *matched = 1;

  return FCS_RESULT_SUCCESS;
}

FCSResult fcs_mmm2d_print_parameters(FCS handle)
{
  fcs_float contrasts_min, contrasts_max;
  fcs_float PWerror;
  fcs_float cutoff;
  fcs_int layers;
  fcs_float skin;

  MMM2D_CHECK_RETURN_RESULT(handle, __func__);

  fcs_mmm2d_get_dielectric_contrasts(handle, &contrasts_min, &contrasts_max);
  fcs_mmm2d_get_far_cutoff(handle, &cutoff);
  fcs_mmm2d_get_layers_per_node(handle, &layers);
  fcs_mmm2d_get_maxPWerror(handle, &PWerror);
  fcs_mmm2d_get_skin(handle, &skin);

  printf("mmm2d dielectric contrasts: %e %e\n", contrasts_min, contrasts_max);
  printf("mmm2d far cutoff: %e\n", cutoff);
  printf("mmm2d layer per node: %" FCS_LMOD_INT "d\n", layers);
  printf("mmm2d maximum PWerror: %e\n", PWerror);
  printf("mmm2d skin: %e\n", skin);

  return FCS_RESULT_SUCCESS;
}
