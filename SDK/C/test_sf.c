#include "noetics.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

int main() {
    printf("Testing Spectral Flow in C SDK...\n");

    int dim = 2;
    noetics_context* ctx = noetics_create_context(dim, 1);
    
    /* L0 = [[-1, 0], [0, -1]] */
    noetics_op l0;
    l0.dim = dim;
    l0.data = (double*)malloc(sizeof(double) * dim * dim);
    l0.data[0] = -1.0; l0.data[1] = 0.0;
    l0.data[2] = 0.0;  l0.data[3] = -1.0;

    /* Create 3 keys for a trajectory */
    /* t=0: K = [[0, 0], [0, 0]] -> L = [[-1, 0], [0, -1]] (2 neg) */
    /* t=1: K = [[2, 0], [0, 0]] -> L = [[ 1, 0], [0, -1]] (1 neg) -> SF = +1 */
    /* t=2: K = [[2, 0], [0, 2]] -> L = [[ 1, 0], [0,  1]] (0 neg) -> SF = +2 */
    
    noetics_key* k0 = noetics_key_create(ctx);
    memset(noetics_key_data_ptr(k0), 0, sizeof(double) * dim * dim);

    noetics_key* k1 = noetics_key_create(ctx);
    memset(noetics_key_data_ptr(k1), 0, sizeof(double) * dim * dim);
    noetics_key_data_ptr(k1)[0] = 2.0;

    noetics_key* k2 = noetics_key_create(ctx);
    memset(noetics_key_data_ptr(k2), 0, sizeof(double) * dim * dim);
    noetics_key_data_ptr(k2)[0] = 2.0;
    noetics_key_data_ptr(k2)[3] = 2.0;

    const noetics_key* traj[3] = {k0, k1, k2};
    
    int sf_value = 0;
    int res = noetics_compute_spectral_flow(&l0, traj, 3, &sf_value);
    
    printf("  Spectral Flow Result: %d\n", sf_value);

    if (res == 0 && sf_value == 2) {
        printf("  [SUCCESS] Spectral Flow calculation correct.\n");
    } else {
        printf("  [FAILURE] Spectral Flow calculation incorrect (Expected 2, got %d).\n", sf_value);
        return 1;
    }

    free(l0.data);
    noetics_key_destroy(k0);
    noetics_key_destroy(k1);
    noetics_key_destroy(k2);
    noetics_destroy_context(ctx);

    return 0;
}
