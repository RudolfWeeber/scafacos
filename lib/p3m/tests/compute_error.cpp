#include "p3m.h"
#include "types.hpp"
#include "communication.hpp"
#include "error_estimate.hpp"
#include <cstdlib>
#include <cmath>

using namespace ScaFaCoS::P3M;

int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);

  if (argc <= 1) return 2;
  fcs_int grid = atoi(argv[1]);

  data_struct d;
  d.box_l[0] = 10.;
  d.box_l[1] = 10.;
  d.box_l[2] = 10.;
  comm_init(&d.comm, MPI_COMM_WORLD);
  comm_prepare(&d.comm, d.box_l);

  d.sum_qpart = 300.0;
  d.sum_q2 = 300.0;
  d.square_sum_q = 0.0;

  d.cao = 5;

  d.grid[0] = grid;
  d.grid[1] = grid;
  d.grid[2] = grid;

  for (d.alpha = 0.1; d.alpha < 10.0; d.alpha += 0.1) {
    k_space_error(&d);
    fcs_float y = sqrt(d.ks_error)/(d.box_l[0]*d.box_l[0]);
    fcs_float x = d.alpha*d.box_l[0];
    printf("%lf %lf\n", x, y);
  }

  MPI_Finalize();
}
