// See LICENSE for license details.

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef BAREMETAL
#include <sys/mman.h>
#endif
#include "include/gemmini_testutils.h"

int main() {
#ifndef BAREMETAL
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
      perror("mlockall failed");
      exit(1);
    }
#endif

  printf("Flush Gemmini TLB of stale virtual addresses\n");
  gemmini_flush(0);

  printf("Initialize our input and output matrices in main memory\n");
  elem_t A1[DIM][DIM];
  elem_t A2[DIM][DIM];
  elem_t B[DIM][DIM];
  elem_t C[DIM][DIM];

  for (size_t i = 0; i < DIM; i++) {
    for (size_t j = 0; j < DIM; j++) {
      A1[i][j] = i == j;
      A2[i][j] = i == j;
      B[i][j] = i == j;
    }
  }

  printf("Calculate the scratchpad addresses of all our matrices\n");
  printf("  Note: The scratchpad is \"row-addressed\", where each address contains one matrix row\n");
  size_t A1_sp_addr = 0;
  size_t A2_sp_addr = DIM;
  size_t B_sp_addr = 2*DIM;
  size_t C_acc_addr = 0x80000000;

  printf("Move matrices from main memory into Gemmini's scratchpad\n");
  gemmini_config_ld(DIM * sizeof(elem_t));
  gemmini_config_st(DIM * sizeof(elem_t));
  gemmini_mvin(A1, A1_sp_addr);
  gemmini_mvin(A2, A2_sp_addr);
  gemmini_mvin(B, B_sp_addr);

  gemmini_config_ex(WEIGHT_STATIONARY, 0, 0);

  printf("C = A1 * B\n");
  gemmini_preload(B_sp_addr, C_acc_addr);
  gemmini_compute_preloaded(A1_sp_addr, GARBAGE_ADDR);

  printf("C += A2 * B\n");
  gemmini_preload(GARBAGE_ADDR, C_acc_addr);
  gemmini_compute_accumulated(A2_sp_addr, C_acc_addr);

  printf("Move \"C\" matrix from Gemmini's scratchpad into main memory\n");
  gemmini_config_st(DIM * sizeof(elem_t));
  gemmini_mvout(C, C_acc_addr);

  printf("Fence till Gemmini completes all memory operations\n");
  gemmini_fence();

  printf("Input and output matrices:\n");
  printMatrix(A1);
  printMatrix(A2);
  printMatrix(B);
  printMatrix(C);

  exit(0);
}
