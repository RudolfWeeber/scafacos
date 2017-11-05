/*
  Copyright (C) 2011, 2012, 2013, 2014, 2015 Olaf Lenz, Michael Hofmann
  
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

#ifndef _TESTCASE_HPP
#define _TESTCASE_HPP

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <vector>
#include <string>

#include "fcs.h"

#include "common/gridsort/gridsort.h"

#include "particles.hpp"
#include "common.hpp"
#include "Generator.hpp"

#include "rapidxml/rapidxml.hpp"

using namespace std;
using namespace rapidxml;

static const fcs_float EPSILON_METALLIC = -1.0;


class Configuration {
public:
  struct {
    bool reuse_fcs;
    fcs_float box_origin[3];
    fcs_float box_a[3];
    fcs_float box_b[3];
    fcs_float box_c[3];
    fcs_int periodicity[3];
    fcs_float epsilon;
    fcs_int decomposition;
    fcs_int periodic_duplications[3];

    fcs_float result_density;
  } params;

  fcs_int total_duplications[3], total_duplication;

  fcs_float unscale_box[3];

  PlainParticles input_plain;
  fcs_int input_plain_nparticles;
#if SCAFACOS_TEST_WITH_DIPOLES
  fcs_int dipole_input_plain_nparticles;
#endif /* SCAFACOS_TEST_WITH_DIPOLES */

  vector<FileParticles> input_files;
  fcs_int input_file_nparticles;
#if SCAFACOS_TEST_WITH_DIPOLES
  fcs_int dipole_input_file_nparticles;
#endif /* SCAFACOS_TEST_WITH_DIPOLES */

  vector<Generator> input_generators;
  fcs_int input_generator_nparticles;
#if SCAFACOS_TEST_WITH_DIPOLES
  fcs_int dipole_input_generator_nparticles;
#endif /* SCAFACOS_TEST_WITH_DIPOLES */
  
  Duplicate input_duplication;

  FileParticles input_ref;
  fcs_int input_ref_nparticles;

  particles_t input_particles;
  bool input_particles_allocated;

  fcs_int have_reference_values[2], have_result_values[2];
#if SCAFACOS_TEST_WITH_DIPOLES
  fcs_int dipole_have_reference_values[2], dipole_have_result_values[2];
#endif /* SCAFACOS_TEST_WITH_DIPOLES */

  particles_t decomp_particles;

  MPI_Comm decomp_comm;

  fcs_float *reference_potentials, *reference_field;
#if SCAFACOS_TEST_WITH_DIPOLES
  fcs_float *dipole_reference_potentials, *dipole_reference_field;
#endif /* SCAFACOS_TEST_WITH_DIPOLES */
  
  fcs_float field_correction[3], energy_correction;

  MPI_Comm cart_comm;
  fcs_gridsort_t gridsort;

  Configuration();
  ~Configuration();

  void read_config(xml_node<> *config_node, const char *basename);
  void write_config(xml_document<> *doc, xml_node<> *config_node, const char *binfilename = NULL, const char* portable_filename = NULL, bool keep_dupgen = false);

  void write_particles(xml_document<> *doc, xml_node<> *config_node);

  void print_config(fcs_int n);

  void broadcast_config();

  void broadcast_input();
  void generate_input_particles(fcs_float minalloc, fcs_float overalloc);

  void create_cart_comm();
  void destroy_cart_comm();

  void decompose_particles(bool alloc_potentials, bool alloc_field, fcs_float minalloc = 0, fcs_float overalloc = 0);
  void equalize_particles();
  void almost_master_particles();
  void gather_particles();
  bool compute_errors(errors_t *e);
  void free_decomp_particles(bool quiet = false);

private:
  void determine_total_duplication();
};


class Testcase {
public:
  string name;
  string description;
  string reference_method;
  fcs_float error_potential;
  fcs_float error_field;
  vector<Configuration*> configurations;

  Testcase();
  ~Testcase();

  void read_file(const char* filename, fcs_int *periodic_duplications, fcs_int decomposition = -1);
  void write_file(const char* outfilename, const char* binfilename = NULL, const char* portable_filename = NULL, bool keep_dupgen = false);

  void broadcast_config(int root, MPI_Comm comm);

  const char *get_method_config();

private:
  string method_config;

  void read_method_config(xml_node<> *config_node);
};

#endif
