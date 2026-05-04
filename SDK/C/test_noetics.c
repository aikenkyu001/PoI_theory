#include "noetics.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT_TRUE(cond, msg) \
    if (!(cond)) { \
        printf("  [FAILURE] %s\n", msg); \
        return 1; \
    }

int test_lifecycle() {
    printf("Testing Lifecycle...\n");
    noetics_context* ctx = noetics_create_context(4, 2);
    ASSERT_TRUE(ctx != NULL, "Failed to create context");
    
    noetics_key* key = noetics_key_create(ctx);
    ASSERT_TRUE(key != NULL, "Failed to create key");
    
    noetics_key_destroy(key);
    noetics_destroy_context(ctx);
    printf("  [SUCCESS] Lifecycle OK.\n");
    return 0;
}

int test_energy() {
    printf("Testing Energy Calculation...\n");
    noetics_context* ctx = noetics_create_context(4, 1);
    noetics_key* key = noetics_key_create(ctx);
    
    double energy = noetics_compute_energy(key);
    ASSERT_TRUE(fabs(energy - 2.0) < 1e-7, "Initial identity energy should be 2.0");
    
    noetics_key_destroy(key);
    noetics_destroy_context(ctx);
    printf("  [SUCCESS] Energy OK.\n");
    return 0;
}

int test_flows() {
    printf("Testing PKGF Flows...\n");
    noetics_context* ctx = noetics_create_context(2, 1);
    noetics_key* key = noetics_key_create(ctx);
    
    noetics_op omega;
    omega.dim = 2;
    omega.data = (double*)calloc(4, sizeof(double));
    omega.data[1] = 1.0; omega.data[2] = -1.0; // Skew-symmetric
    
    noetics_step_conservative(key, &omega, 0.1);
    ASSERT_TRUE(fabs(noetics_compute_energy(key) - 1.0) < 1e-7, "Energy should be conserved (1.0 for 2x2 I)");
    
    noetics_op dop;
    dop.dim = 2;
    dop.data = (double*)calloc(4, sizeof(double));
    dop.data[0] = 1.0; dop.data[3] = 1.0; // Identity
    
    noetics_step_dissipative(key, &dop, 0.1);
    ASSERT_TRUE(noetics_compute_energy(key) < 1.0, "Energy should decrease in D-phase");
    
    free(omega.data);
    free(dop.data);
    noetics_key_destroy(key);
    noetics_destroy_context(ctx);
    printf("  [SUCCESS] Flows OK.\n");
    return 0;
}

int test_observables() {
    printf("Testing Observables (d_eff, entropy)...\n");
    noetics_context* ctx = noetics_create_context(2, 1);
    noetics_key* key = noetics_key_create(ctx);
    
    double d_eff = noetics_compute_effective_dimension(key, 0.1);
    ASSERT_TRUE(fabs(d_eff - 2.0) < 1e-7, "Identity 2x2 should have d_eff=2");
    
    double entropy = noetics_compute_entropy(key);
    // For 2x2 Identity, p = [0.5, 0.5], H = -2 * (0.5 * log(0.5)) = log(2) approx 0.693
    ASSERT_TRUE(fabs(entropy - log(2.0)) < 1e-7, "Identity 2x2 should have entropy log(2)");
    
    noetics_key_destroy(key);
    noetics_destroy_context(ctx);
    printf("  [SUCCESS] Observables OK.\n");
    return 0;
}

int test_substrate_invariance() {
    printf("Testing Substrate Invariance...\n");
    noetics_context* ctx = noetics_create_context(2, 1);
    noetics_key* key = noetics_key_create(ctx);
    double* data = noetics_key_data_ptr(key);
    data[0] = 2.0; data[3] = 0.5;
    
    noetics_structure* s = noetics_export_structure(key);
    ASSERT_TRUE(s != NULL, "Export failed");
    ASSERT_TRUE(s->dim == 2, "Wrong dimension");
    
    noetics_key* key2 = noetics_import_structure(ctx, s);
    ASSERT_TRUE(key2 != NULL, "Import failed");
    ASSERT_TRUE(fabs(noetics_compute_energy(key) - noetics_compute_energy(key2)) < 1e-7, "Structure not preserved");
    
    noetics_key_destroy(key);
    noetics_key_destroy(key2);
    free(s->eigenvalues);
    free(s->eigenvectors);
    free(s);
    noetics_destroy_context(ctx);
    printf("  [SUCCESS] Substrate Invariance OK.\n");
    return 0;
}

int test_sectors() {
    printf("Testing Sectors...\n");
    noetics_context* ctx = noetics_create_context(4, 2);
    noetics_key* key = noetics_key_create(ctx);
    
    int s0_indices[] = {0, 1};
    noetics_assign_sector_indices(key, 0, s0_indices, 2);
    
    double s0_energy = noetics_compute_sector_energy(key, 0);
    ASSERT_TRUE(fabs(s0_energy - 1.0) < 1e-7, "Sector 0 energy should be 1.0");
    
    int active[2];
    int n_active = noetics_active_sectors(key, active);
    ASSERT_TRUE(n_active >= 1, "Should have active sectors");
    
    noetics_key_destroy(key);
    noetics_destroy_context(ctx);
    printf("  [SUCCESS] Sectors OK.\n");
    return 0;
}

int test_analysis() {
    printf("Testing Analysis Functions (Rank Jump, Phase Transition, SF)...\n");
    noetics_context* ctx = noetics_create_context(2, 1);
    
    /* Trajectory for Rank Jump */
    noetics_key* k1 = noetics_key_create(ctx);
    noetics_key* k2 = noetics_key_create(ctx);
    noetics_key_data_ptr(k2)[0] = 0.0; // Rank jump 2 -> 1
    
    const noetics_key* traj[2] = {k1, k2};
    int jump = noetics_detect_rank_jump(traj, 2);
    ASSERT_TRUE(jump == 1, "Should detect rank jump at index 1");
    
    /* Phase Transition */
    double d_eff_series[] = {2.0, 2.0, 1.0, 1.0, 3.0};
    int transitions[5];
    int n_trans = noetics_detect_phase_transition(d_eff_series, 5, transitions);
    ASSERT_TRUE(n_trans >= 2, "Should detect at least 2 transitions");
    
    noetics_key_destroy(k1);
    noetics_key_destroy(k2);
    noetics_destroy_context(ctx);
    printf("  [SUCCESS] Analysis Functions OK.\n");
    return 0;
}

int main() {
    if (test_lifecycle() != 0) return 1;
    if (test_energy() != 0) return 1;
    if (test_flows() != 0) return 1;
    if (test_observables() != 0) return 1;
    if (test_substrate_invariance() != 0) return 1;
    if (test_sectors() != 0) return 1;
    if (test_analysis() != 0) return 1;
    
    printf("\nAll C SDK comprehensive tests PASSED.\n");
    return 0;
}
