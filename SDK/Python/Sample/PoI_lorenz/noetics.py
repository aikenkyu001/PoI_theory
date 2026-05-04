import numpy as np
from scipy.linalg import expm

class Context:
    def __init__(self, dim: int, n_sectors: int = 1):
        self.dim = dim
        self.n_sectors = n_sectors
        self.sectors = {} # sector_id -> list of indices

    def assign_sector(self, sector_id: int, indices: list):
        if 0 <= sector_id < self.n_sectors:
            self.sectors[sector_id] = np.array(indices)

class Key:
    def __init__(self, ctx: Context):
        self.ctx = ctx
        self.dim = ctx.dim
        # Initialize as Identity matrix
        self.data = np.eye(self.dim, dtype=np.complex128)

    def step_conservative(self, omega: np.ndarray, dt: float):
        # dot K = [Omega, K] => K(t+dt) = exp(Omega*dt) K(t) exp(-Omega*dt)
        e_om = expm(omega * dt)
        e_om_inv = expm(-omega * dt)
        self.data = e_om @ self.data @ e_om_inv

    def step_dissipative(self, dop: np.ndarray, dt: float):
        # dot K = -D(K) => K(t+dt) = exp(-D*dt) K(t)
        e_dop = expm(-dop * dt)
        self.data = e_dop @ self.data

    def step_unified(self, omega: np.ndarray, dop: np.ndarray, lam: float, dt: float):
        # dot K = [Omega, K] - lambda * D(K)
        # Using Strang splitting for 2nd order accuracy if possible, 
        # but Lie-Trotter is fine for now.
        e_om = expm(omega * dt)
        e_om_inv = expm(-omega * dt)
        e_dop = expm(-lam * dop * dt)
        
        # Apply conservative flow then dissipative flow
        self.data = e_om @ self.data @ e_om_inv
        self.data = e_dop @ self.data

    def energy(self) -> float:
        # E = 0.5 * ||K||_F^2
        return 0.5 * np.linalg.norm(self.data)**2

    def effective_dimension(self, epsilon: float = 1e-3) -> float:
        # Count eigenvalues with magnitude > epsilon
        s = np.linalg.svd(self.data, compute_uv=False)
        return float(np.sum(s > epsilon))

    def entropy(self) -> float:
        # Shannon entropy of singular value distribution
        s = np.linalg.svd(self.data, compute_uv=False)
        s = np.abs(s)
        s_sum = np.sum(s)
        if s_sum < 1e-12: return 0.0
        p = s / s_sum
        p = p[p > 1e-12]
        return float(-np.sum(p * np.log(p)))

    def sector_energy(self, sector_id: int) -> float:
        if sector_id not in self.ctx.sectors:
            return 0.0
        idx = self.ctx.sectors[sector_id]
        # Extract submatrix K_S
        k_s = self.data[np.ix_(idx, idx)]
        return 0.5 * np.linalg.norm(k_s)**2

    def export_structure(self):
        # Return eigenvalues and eigenvectors
        evals, evecs = np.linalg.eig(self.data)
        return {"dim": self.dim, "eigenvalues": evals, "eigenvectors": evecs}

    @staticmethod
    def import_structure(ctx, structure):
        key = Key(ctx)
        evals = structure["eigenvalues"]
        evecs = structure["eigenvectors"]
        # Reconstruct K = V * diag(L) * V^-1
        key.data = evecs @ np.diag(evals) @ np.linalg.inv(evecs)
        return key

def compute_spectral_flow(l0: np.ndarray, keys: list) -> int:
    # Simplified spectral flow: count net changes in signs of eigenvalues of L0 + K
    def count_negative_evals(k_data):
        m = l0 + k_data
        evals = np.linalg.eigvals(m)
        return np.sum(np.real(evals) < 0)

    start_neg = count_negative_evals(keys[0].data)
    end_neg = count_negative_evals(keys[-1].data)
    return int(end_neg - start_neg)

def detect_rank_jump(keys: list, epsilon: float = 1e-3) -> list:
    ranks = [k.effective_dimension(epsilon) for k in keys]
    jumps = []
    for i in range(1, len(ranks)):
        if abs(ranks[i] - ranks[i-1]) > 0.5: # Discontinuous jump
            jumps.append(i)
    return jumps

def detect_phase_transition(d_eff_series: list) -> list:
    # Detect sharp changes in d_eff
    diff = np.abs(np.diff(d_eff_series))
    if len(diff) == 0: return []
    threshold = np.mean(diff) + 2 * np.std(diff)
    return [i+1 for i, val in enumerate(diff) if val > threshold]
