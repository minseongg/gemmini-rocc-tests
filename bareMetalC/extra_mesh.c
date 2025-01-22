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
  elem_t In[DIM][DIM];
  elem_t Out0[DIM][DIM]; // Uses Mesh 0.
  elem_t Out1[DIM][DIM]; // Uses Mesh 1.

  elem_t Identity[DIM][DIM];
  for (size_t i = 0; i < DIM; i++)
    for (size_t j = 0; j < DIM; j++)
      Identity[i][j] = i == j;

  printf("Calculate the scratchpad addresses of all our matrices\n");
  printf("  Note: The scratchpad is \"row-addressed\", where each address contains one matrix row\n");
  size_t In_sp_addr = 0;
  size_t Out_sp_addr0 = DIM;
  size_t Out_sp_addr1 = 2*DIM;
  size_t Identity_sp_addr = 3*DIM;

  printf("Move \"In\" matrix from main memory into Gemmini's scratchpad\n");
  gemmini_config_ld(DIM * sizeof(elem_t));
  gemmini_config_st(DIM * sizeof(elem_t));
  gemmini_mvin(In, In_sp_addr);

  printf("Move \"Identity\" matrix from main memory into Gemmini's scratchpad\n");
  gemmini_mvin(Identity, Identity_sp_addr);

  printf("Multiply \"In\" matrix with \"Identity\" matrix with a bias of 0, using Mesh 0\n");
  gemmini_config_ex(OUTPUT_STATIONARY, 0, 0);
  gemmini_preload_zeros(Out_sp_addr0);
  gemmini_compute_preloaded(In_sp_addr, Identity_sp_addr);

  printf("Move \"Out0\" matrix from Gemmini's scratchpad into main memory\n");
  gemmini_config_st(DIM * sizeof(elem_t));
  gemmini_mvout(Out0, Out_sp_addr0);

  printf("Multiply \"In\" matrix with \"Identity\" matrix with a bias of 0, using Mesh 1\n");
  gemmini_preload_zeros2(Out_sp_addr1);
  gemmini_compute_preloaded2(In_sp_addr, Identity_sp_addr);

  printf("Move \"Out1\" matrix from Gemmini's scratchpad into main memory\n");
  gemmini_config_st(DIM * sizeof(elem_t));
  gemmini_mvout(Out1, Out_sp_addr1);

  printf("Fence till Gemmini completes all memory operations\n");
  gemmini_fence();

  printf("Check whether \"In\" and \"Out0\" matrices are identical\n");
  if (!is_equal(In, Out0)) {
    printf("Input and output matrices are different!\n");
    printf("\"In\" matrix:\n");
    printMatrix(In);
    printf("\"Out0\" matrix:\n");
    printMatrix(Out0);
    printf("\n");

    exit(1);
  }

  printf("Check whether \"In\" and \"Out1\" matrices are identical\n");
  if (!is_equal(In, Out1)) {
    printf("Input and output matrices are different!\n");
    printf("\"In\" matrix:\n");
    printMatrix(In);
    printf("\"Out1\" matrix:\n");
    printMatrix(Out1);
    printf("\n");

    exit(1);
  }

  printf("Input and output matrices are identical, as expected\n");
  exit(0);
}

