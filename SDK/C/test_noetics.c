#include "noetics.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

int main() {
    printf("Testing Standalone C SDK...\n");

    /* Create context */
    noetics_context* ctx = noetics_create_context(4, 1);
    if (!ctx) {
        printf("Failed to create context.\n");
        return 1;
    }

    /* Create key */
    noetics_key* key = noetics_key_create(ctx);
    if (!key) {
        printf("Failed to create key.\n");
        return 1;
    }

    /* Initial Energy calculation */
    double energy = noetics_compute_energy(key);
    printf("  Initial Energy: %f\n", energy);

    if (fabs(energy - 2.0) < 1e-7) {
        printf("  [SUCCESS] Energy calculation correct.\n");
    } else {
        printf("  [FAILURE] Energy calculation incorrect.\n");
        return 1;
    }

    /* Cleanup */
    noetics_key_destroy(key);
    noetics_destroy_context(ctx);

    printf("All C SDK foundational tests PASSED.\n");
    return 0;
}
