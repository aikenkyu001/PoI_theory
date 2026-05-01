#ifndef NOETICS_H
#define NOETICS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
 * @brief Context for Noetics operations.
 * Holds dimensionality and sector information.
 */
typedef struct noetics_context noetics_context;

/**
 * @brief Parallel Key (K).
 * The primary state variable representing internal cognitive structure.
 */
typedef struct noetics_key noetics_key;

/**
 * @brief Matrix Operator (Standard).
 * Used for Semantic Potential (Omega) and Dissipative Operator (D).
 */
typedef struct {
    int dim;        /* Matrix size dim x dim */
    double* data;   /* row-major array of size dim*dim */
} noetics_op;

/**
 * @brief Substrate Structure for isomorphism.
 * Used to export/import the canonical form of a Parallel Key.
 */
typedef struct {
    int dim;
    double* eigenvalues;   /* Array of size dim */
    double* eigenvectors;  /* Array of size dim*dim (row-major) */
} noetics_structure;

/* --- Context Lifecycle --- */
noetics_context* noetics_create_context(int dim, int n_sectors);
void             noetics_destroy_context(noetics_context* ctx);

/* --- Key Lifecycle --- */
noetics_key* noetics_key_create(noetics_context* ctx);
void         noetics_key_destroy(noetics_key* key);

/* --- Flows (PKGF) --- */
/* Constructive: nabla K = [Omega, K] */
int noetics_step_conservative(noetics_key* key, const noetics_op* omega, double dt);

/* Dissipative: dot K = -lambda * D(K) */
int noetics_step_dissipative(noetics_key* key, const noetics_op* dop, double dt);

/* Unified: nabla K = [Omega, K] - lambda * D(K) */
int noetics_step_unified(noetics_key* key, const noetics_op* omega, const noetics_op* dop, double lambda, double dt);

/* --- Observables (PoI) --- */
double noetics_compute_energy(const noetics_key* key);
double noetics_compute_effective_dimension(const noetics_key* key, double epsilon);
double noetics_compute_entropy(const noetics_key* key);

/* --- Spectral Flow --- */
int noetics_compute_spectral_flow(const noetics_key* const* key_traj, int n_steps, int* out_sf_value);

/* --- Rank Jump / Phase Transition --- */
int noetics_detect_rank_jump(const noetics_key* const* key_traj, int n_steps);
int noetics_detect_phase_transition(const double* d_eff_series, int n_steps, int* out_indices);

/* --- Sector / Gauge Management --- */
int    noetics_assign_sector_indices(noetics_key* key, int sector_id, const int* indices, int count);
int    noetics_active_sectors(const noetics_key* key, int* out_sectors);
double noetics_compute_sector_energy(const noetics_key* key, int sector_id);

/* --- Substrate Invariance --- */
noetics_structure* noetics_export_structure(const noetics_key* key);
noetics_key*       noetics_import_structure(noetics_context* ctx, const noetics_structure* s);

/* --- WASM SDK Extended Exports --- */
double             wasm_noetics_compute_entropy(const noetics_key* key);
int                wasm_noetics_detect_rank_jump(const noetics_key* const* key_traj, int n_steps);
noetics_structure* wasm_noetics_export_structure(const noetics_key* key);
noetics_key*       wasm_noetics_import_structure(noetics_context* ctx, const noetics_structure* s);

#ifdef __cplusplus
}
#endif

#endif /* NOETICS_H */
