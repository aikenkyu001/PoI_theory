#include "noetics.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* --- Internal Structures --- */

struct noetics_context {
    int dim;
    int n_sectors;
    int** sector_map;    /* sector_map[n_sectors][dim] - binary mask or index list */
    int* sector_counts;
};

struct noetics_key {
    noetics_context* ctx;
    double* data;       /* Matrix data in row-major */
};

/* --- Matrix Utilities --- */

static void mat_mul(int n, const double* A, const double* B, double* C) {
    memset(C, 0, sizeof(double) * n * n);
    for (int i = 0; i < n; i++) {
        for (int k = 0; k < n; k++) {
            double aik = A[i * n + k];
            if (fabs(aik) < 1e-18) continue;
            for (int j = 0; j < n; j++) {
                C[i * n + j] += aik * B[k * n + j];
            }
        }
    }
}

static void mat_commutator(int n, const double* A, const double* B, double* res) {
    double* AB = (double*)malloc(sizeof(double) * n * n);
    double* BA = (double*)malloc(sizeof(double) * n * n);
    mat_mul(n, A, B, AB);
    mat_mul(n, B, A, BA);
    for (int i = 0; i < n * n; i++) {
        res[i] = AB[i] - BA[i];
    }
    free(AB);
    free(BA);
}

/**
 * @brief Simple Jacobi eigenvalue algorithm for symmetric matrices.
 * Used for v1.0 eigenvalue-based observables.
 */
static int jacobi_eigen(int n, double* A, double* eigenvalues, double* eigenvectors) {
    int i, j, k, p, q, iter;
    double theta, tau, t, c, s, a_pp, a_qq, a_pq, a_ip, a_iq;
    
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            eigenvectors[i * n + j] = (i == j) ? 1.0 : 0.0;
        }
    }
    
    for (iter = 0; iter < 50; iter++) {
        double max_offdiag = 0.0;
        p = 0; q = 1;
        for (i = 0; i < n; i++) {
            for (j = i + 1; j < n; j++) {
                if (fabs(A[i * n + j]) > max_offdiag) {
                    max_offdiag = fabs(A[i * n + j]);
                    p = i; q = j;
                }
            }
        }
        
        if (max_offdiag < 1e-15) break;
        
        a_pp = A[p * n + p];
        a_qq = A[q * n + q];
        a_pq = A[p * n + q];
        
        theta = (a_qq - a_pp) / (2.0 * a_pq);
        t = (theta >= 0) ? 1.0 / (theta + sqrt(1.0 + theta * theta))
                         : -1.0 / (-theta + sqrt(1.0 + theta * theta));
        c = 1.0 / sqrt(1.0 + t * t);
        s = t * c;
        tau = s / (1.0 + c);
        
        A[p * n + p] = a_pp - t * a_pq;
        A[q * n + q] = a_qq + t * a_pq;
        A[p * n + q] = 0.0;
        A[q * n + p] = 0.0;
        
        for (i = 0; i < n; i++) {
            if (i != p && i != q) {
                a_ip = A[i * n + p];
                a_iq = A[i * n + q];
                A[i * n + p] = A[p * n + i] = a_ip - s * (a_iq + tau * a_ip);
                A[i * n + q] = A[q * n + i] = a_iq + s * (a_ip - tau * a_iq);
            }
        }
        
        for (i = 0; i < n; i++) {
            a_ip = eigenvectors[i * n + p];
            a_iq = eigenvectors[i * n + q];
            eigenvectors[i * n + p] = a_ip - s * (a_iq + tau * a_ip);
            eigenvectors[i * n + q] = a_iq + s * (a_ip - tau * a_iq);
        }
    }
    
    for (i = 0; i < n; i++) eigenvalues[i] = A[i * n + i];
    return 0;
}

/* --- Context Lifecycle --- */

noetics_context* noetics_create_context(int dim, int n_sectors) {
    noetics_context* ctx = (noetics_context*)malloc(sizeof(noetics_context));
    ctx->dim = dim;
    ctx->n_sectors = n_sectors;
    ctx->sector_map = (int**)calloc(n_sectors, sizeof(int*));
    ctx->sector_counts = (int*)calloc(n_sectors, sizeof(int));
    return ctx;
}

void noetics_destroy_context(noetics_context* ctx) {
    if (!ctx) return;
    for (int i = 0; i < ctx->n_sectors; i++) {
        if (ctx->sector_map[i]) free(ctx->sector_map[i]);
    }
    free(ctx->sector_map);
    free(ctx->sector_counts);
    free(ctx);
}

/* --- Key Lifecycle --- */

noetics_key* noetics_key_create(noetics_context* ctx) {
    if (!ctx) return NULL;
    noetics_key* key = (noetics_key*)malloc(sizeof(noetics_key));
    key->ctx = ctx;
    key->data = (double*)calloc(ctx->dim * ctx->dim, sizeof(double));
    for (int i = 0; i < ctx->dim; i++) {
        key->data[i * ctx->dim + i] = 1.0;
    }
    return key;
}

void noetics_key_destroy(noetics_key* key) {
    if (!key) return;
    free(key->data);
    free(key);
}

/* --- Flows --- */

int noetics_step_conservative(noetics_key* key, const noetics_op* omega, double dt) {
    int n = key->ctx->dim;
    double* comm = (double*)malloc(sizeof(double) * n * n);
    mat_commutator(n, omega->data, key->data, comm);
    for (int i = 0; i < n * n; i++) {
        key->data[i] += comm[i] * dt;
    }
    free(comm);
    return 0;
}

int noetics_step_dissipative(noetics_key* key, const noetics_op* dop, double dt) {
    int n = key->ctx->dim;
    double* d_effect = (double*)malloc(sizeof(double) * n * n);
    mat_mul(n, dop->data, key->data, d_effect);
    for (int i = 0; i < n * n; i++) {
        key->data[i] -= d_effect[i] * dt;
    }
    free(d_effect);
    return 0;
}

int noetics_step_unified(noetics_key* key, const noetics_op* omega, const noetics_op* dop, double lambda, double dt) {
    int n = key->ctx->dim;
    double* comm = (double*)malloc(sizeof(double) * n * n);
    double* d_effect = (double*)malloc(sizeof(double) * n * n);
    mat_commutator(n, omega->data, key->data, comm);
    mat_mul(n, dop->data, key->data, d_effect);
    for (int i = 0; i < n * n; i++) {
        key->data[i] += (comm[i] - lambda * d_effect[i]) * dt;
    }
    free(comm);
    free(d_effect);
    return 0;
}

/* --- Observables --- */

double noetics_compute_energy(const noetics_key* key) {
    int n = key->ctx->dim;
    double energy = 0;
    for (int i = 0; i < n * n; i++) {
        energy += key->data[i] * key->data[i];
    }
    return 0.5 * energy;
}

double noetics_compute_effective_dimension(const noetics_key* key, double epsilon) {
    int n = key->ctx->dim;
    double* A = (double*)malloc(sizeof(double) * n * n);
    double* evals = (double*)malloc(sizeof(double) * n);
    double* evecs = (double*)malloc(sizeof(double) * n * n);
    
    memcpy(A, key->data, sizeof(double) * n * n);
    jacobi_eigen(n, A, evals, evecs);
    
    double d_eff = 0;
    for (int i = 0; i < n; i++) {
        if (fabs(evals[i]) > epsilon) d_eff += 1.0;
    }
    
    free(A); free(evals); free(evecs);
    return d_eff;
}

double noetics_compute_entropy(const noetics_key* key) {
    int n = key->ctx->dim;
    double* A = (double*)malloc(sizeof(double) * n * n);
    double* evals = (double*)malloc(sizeof(double) * n);
    double* evecs = (double*)malloc(sizeof(double) * n * n);
    
    memcpy(A, key->data, sizeof(double) * n * n);
    jacobi_eigen(n, A, evals, evecs);
    
    double s_sum = 0;
    for (int i = 0; i < n; i++) s_sum += fabs(evals[i]);
    
    double h = 0;
    if (s_sum > 1e-12) {
        for (int i = 0; i < n; i++) {
            double p = fabs(evals[i]) / s_sum;
            if (p > 1e-12) h -= p * log(p);
        }
    }
    
    free(A); free(evals); free(evecs);
    return h;
}

/* --- Spectral Flow --- */

int noetics_compute_spectral_flow(const noetics_key* const* key_traj, int n_steps, int* out_sf_value) {
    (void)key_traj; (void)n_steps;
    if (out_sf_value) *out_sf_value = 0;
    return 0;
}

/* --- Rank Jump / Phase Transition --- */

int noetics_detect_rank_jump(const noetics_key* const* key_traj, int n_steps) {
    if (n_steps < 2) return -1;
    double prev_rank = noetics_compute_effective_dimension(key_traj[0], 0.1);
    for (int i = 1; i < n_steps; i++) {
        double current_rank = noetics_compute_effective_dimension(key_traj[i], 0.1);
        if (fabs(current_rank - prev_rank) > 0.5) {
            return i;
        }
        prev_rank = current_rank;
    }
    return -1;
}

int noetics_detect_phase_transition(const double* d_eff_series, int n_steps, int* out_indices) {
    int count = 0;
    for (int i = 1; i < n_steps; i++) {
        if (fabs(d_eff_series[i] - d_eff_series[i-1]) > 0.5) {
            if (out_indices) out_indices[count] = i;
            count++;
        }
    }
    return count;
}

/* --- Sector / Gauge Management --- */

int noetics_assign_sector_indices(noetics_key* key, int sector_id, const int* indices, int count) {
    if (sector_id < 0 || sector_id >= key->ctx->n_sectors) return -1;
    noetics_context* ctx = key->ctx;
    if (ctx->sector_map[sector_id]) free(ctx->sector_map[sector_id]);
    ctx->sector_map[sector_id] = (int*)malloc(sizeof(int) * count);
    memcpy(ctx->sector_map[sector_id], indices, sizeof(int) * count);
    ctx->sector_counts[sector_id] = count;
    return 0;
}

double noetics_compute_sector_energy(const noetics_key* key, int sector_id) {
    if (sector_id < 0 || sector_id >= key->ctx->n_sectors) return 0.0;
    noetics_context* ctx = key->ctx;
    int count = ctx->sector_counts[sector_id];
    int* idx = ctx->sector_map[sector_id];
    if (!idx) return 0.0;
    double s_energy = 0;
    int n = ctx->dim;
    for (int i = 0; i < count; i++) {
        for (int j = 0; j < count; j++) {
            double val = key->data[idx[i] * n + idx[j]];
            s_energy += val * val;
        }
    }
    return 0.5 * s_energy;
}

int noetics_active_sectors(const noetics_key* key, int* out_sectors) {
    int count = 0;
    for (int i = 0; i < key->ctx->n_sectors; i++) {
        if (noetics_compute_sector_energy(key, i) > 1e-9) {
            if (out_sectors) out_sectors[count] = i;
            count++;
        }
    }
    return count;
}

/* --- Substrate Invariance --- */

noetics_structure* noetics_export_structure(const noetics_key* key) {
    int n = key->ctx->dim;
    noetics_structure* s = (noetics_structure*)malloc(sizeof(noetics_structure));
    s->dim = n;
    s->eigenvalues = (double*)calloc(n, sizeof(double));
    s->eigenvectors = (double*)calloc(n * n, sizeof(double));
    
    double* A = (double*)malloc(sizeof(double) * n * n);
    memcpy(A, key->data, sizeof(double) * n * n);
    
    jacobi_eigen(n, A, s->eigenvalues, s->eigenvectors);
    
    free(A);
    return s;
}

noetics_key* noetics_import_structure(noetics_context* ctx, const noetics_structure* s) {
    if (ctx->dim != s->dim) return NULL;
    noetics_key* key = noetics_key_create(ctx);
    int n = s->dim;
    
    /* K = V * diag(L) * V^T */
    double* V_L = (double*)malloc(sizeof(double) * n * n);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            V_L[i * n + j] = s->eigenvectors[i * n + j] * s->eigenvalues[j];
        }
    }
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            double val = 0;
            for (int k = 0; k < n; k++) {
                val += V_L[i * n + k] * s->eigenvectors[j * n + k]; /* V^T_kj = V_jk */
            }
            key->data[i * n + j] = val;
        }
    }
    
    free(V_L);
    return key;
}

/* --- WASM SDK Extended Exports --- */

double wasm_noetics_compute_entropy(const noetics_key* key) {
    return noetics_compute_entropy(key);
}

int wasm_noetics_detect_rank_jump(const noetics_key* const* key_traj, int n_steps) {
    return noetics_detect_rank_jump(key_traj, n_steps);
}

noetics_structure* wasm_noetics_export_structure(const noetics_key* key) {
    return noetics_export_structure(key);
}

noetics_key* wasm_noetics_import_structure(noetics_context* ctx, const noetics_structure* s) {
    return noetics_import_structure(ctx, s);
}
