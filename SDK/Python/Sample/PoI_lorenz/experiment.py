import numpy as np
import matplotlib.pyplot as plt
import csv
from noetics import Context, Key
from scipy.linalg import expm

# Standard Lorenz RK4 Integration
def lorenz_deriv(state, sigma=10, beta=8/3, rho=24.5):
    x, y, z = state
    dx = sigma * (y - x)
    dy = x * (rho - z) - y
    dz = x * y - beta * z
    return np.array([dx, dy, dz])

def lorenz_step_rk4(state, dt=0.005):
    k1 = lorenz_deriv(state)
    k2 = lorenz_deriv(state + 0.5 * dt * k1)
    k3 = lorenz_deriv(state + 0.5 * dt * k2)
    k4 = lorenz_deriv(state + dt * k3)
    return state + (dt / 6.0) * (k1 + 2*k2 + 2*k3 + k4)

def simulate_lorenz(x0, steps=600, dt=0.005): # Extended to 3s
    traj = np.zeros((steps, 3))
    traj[0] = x0
    for i in range(1, steps):
        traj[i] = lorenz_step_rk4(traj[i-1], dt=dt)
    return traj

# Noetics Lorenz with Phase Transition Tracking
def simulate_noetics_final(x0, steps=600, dt=0.005, lam0=0.1, alpha=0.05, W=None, b=None):
    ctx = Context(dim=3)
    key = Key(ctx)
    U_init = np.array([[ 0.46744091,  0.88388144, -0.01589344],
                       [-0.54304362,  0.30128247,  0.78379366],
                       [ 0.69756908, -0.35774639,  0.62081793]], dtype=np.complex128)
    U_H = U_init.conj().T
    key.data = U_init @ np.diag(x0.astype(np.complex128)) @ U_H
    
    traj = np.zeros((steps, 3))
    traj[0] = x0
    
    metrics = {
        "comm_norm": [],
        "entropy": [],
        "coherence": [],
        "rank": []
    }
    
    for i in range(1, steps):
        x, y, z = traj[i-1]
        j = np.array([[-10, 10, 0], [24.5 - z, -1, -x], [y, x, -8/3]], dtype=np.complex128)
        omega = (j - j.conj().T) / 2.0
        s = (j + j.conj().T) / 2.0
        
        # 1. Geometric Alignment
        comm = omega @ key.data - key.data @ omega
        c_norm = np.linalg.norm(comm)
        metrics["comm_norm"].append(c_norm)
        
        # Dynamic Gauge & Integration
        lam_t = lam0 * np.exp(-alpha * c_norm)
        key.step_unified(omega, -s, lam=lam_t, dt=dt)
        eps = dt * 0.1
        key.data = (np.eye(3) + eps * s) @ key.data @ (np.eye(3) - eps * s)
        
        # 2. Spectral Metrics
        eigvals = np.linalg.eigvals(key.data)
        mags = np.abs(eigvals)
        probs = mags / (np.sum(mags) + 1e-15)
        metrics["entropy"].append(-np.sum(probs * np.log(probs + 1e-15)))
        
        angles = np.angle(eigvals)
        metrics["coherence"].append(np.mean(np.cos(angles[:, None] - angles[None, :])))
        
        # 3. Effective Rank (Rank Jump Tracking)
        metrics["rank"].append(np.sum(mags > 0.1)) # threshold for rank detection

        phi = key.data.real.flatten()
        if W is not None:
            traj[i] = phi @ W + (b if b is not None else 0)
        else:
            traj[i] = np.real(np.diag(U_H @ key.data @ U_init))
            
        if np.any(np.abs(traj[i]) > 1e10):
            traj[i:] = traj[i-1]
            break
            
    return traj, metrics

def train_readout(x0, steps=600, dt=0.005):
    true_traj = simulate_lorenz(x0, steps=steps, dt=dt)
    ctx = Context(dim=3)
    key = Key(ctx)
    U_init = np.array([[0.46744091, 0.88388144, -0.01589344], [-0.54304362, 0.30128247, 0.78379366], [0.69756908, -0.35774639, 0.62081793]], dtype=np.complex128)
    U_H = U_init.conj().T
    key.data = U_init @ np.diag(x0.astype(np.complex128)) @ U_H
    
    phis = []
    for i in range(1, steps):
        x, y, z = true_traj[i-1]
        j = np.array([[-10, 10, 0], [24.5 - z, -1, -x], [y, x, -8/3]], dtype=np.complex128)
        omega = (j - j.conj().T) / 2.0
        s = (j + j.conj().T) / 2.0
        comm = omega @ key.data - key.data @ omega
        lam_t = 0.1 * np.exp(-0.05 * np.linalg.norm(comm))
        key.step_unified(omega, -s, lam=lam_t, dt=dt)
        eps = dt * 0.1
        key.data = (np.eye(3) + eps * s) @ key.data @ (np.eye(3) - eps * s)
        phis.append(key.data.real.flatten())
        
    Y = true_traj[1:]
    X_aug = np.column_stack([phis, np.ones(len(phis))])
    res = np.linalg.lstsq(X_aug, Y, rcond=None)[0]
    return res[:-1, :], res[-1, :]

if __name__ == "__main__":
    dt, steps = 0.005, 600
    x0 = np.array([1.0, 1.0, 1.0])
    
    W, b = train_readout(x0, steps=steps, dt=dt)
    noe_traj, metrics = simulate_noetics_final(x0, steps=steps, dt=dt, W=W, b=b)
    true_traj = simulate_lorenz(x0, steps=steps, dt=dt)
    
    time = np.arange(steps) * dt
    rmses = np.sqrt((true_traj - noe_traj)**2).mean(axis=1)
    
    # (A) Change Point Detection (Lag Analysis)
    rmse_jump = np.where(rmses > 2.0)[0]
    t_rmse_fail = time[rmse_jump[0]] if len(rmse_jump) > 0 else 0
    
    d_coh = np.abs(np.diff(metrics["coherence"]))
    coh_jump = np.where(d_coh > np.mean(d_coh) + 3*np.std(d_coh))[0]
    t_coh_fail = time[coh_jump[0]] if len(coh_jump) > 0 else 0
    
    print(f"Point-wise Failure (RMSE > 2.0): t = {t_rmse_fail:.3f} s")
    print(f"Structural Failure (Coherence Jump): t = {t_coh_fail:.3f} s")
    print(f"Structural Survival Margin: {t_coh_fail - t_rmse_fail:.3f} s")

    # (B) CSV Export
    with open('metrics.csv', 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(["step", "time", "true_x", "noe_x", "rmse", "comm_norm", "entropy", "coherence", "rank"])
        for i in range(steps - 1):
            writer.writerow([i, time[i+1], true_traj[i+1,0], noe_traj[i+1,0], rmses[i+1], 
                             metrics["comm_norm"][i], metrics["entropy"][i], metrics["coherence"][i], metrics["rank"][i]])
    
    # (C) Visualization
    plt.figure(figsize=(15, 12))
    
    # Plot 1: True vs Noe with Coherence Coloring
    plt.subplot(2, 2, 1)
    sc = plt.scatter(true_traj[1:, 0], noe_traj[1:, 0], c=metrics["coherence"], cmap='viridis', s=10)
    plt.plot([-20, 20], [-20, 20], 'r--', alpha=0.5)
    plt.colorbar(sc, label="Phase Coherence")
    plt.xlabel("True X")
    plt.ylabel("Noetics X")
    plt.title("Structural Survival Scatter Plot")
    
    # Plot 2: Time vs RMSE & Metrics
    plt.subplot(2, 2, 2)
    plt.plot(time, rmses, label="RMSE", color='red')
    plt.axvline(x=t_rmse_fail, color='red', linestyle=':', label="Point Failure")
    plt.axvline(x=t_coh_fail, color='green', linestyle='--', label="Structural Failure")
    plt.yscale('log')
    plt.title("Failure Lag Analysis")
    plt.legend()
    
    # Plot 3: Rank Jumps
    plt.subplot(2, 2, 3)
    plt.step(time[1:], metrics["rank"], where='post', label="Effective Rank")
    plt.title("Rank Spectrum Tracking")
    plt.legend()
    
    # Plot 4: Geometric Alignment
    plt.subplot(2, 2, 4)
    plt.plot(time[1:], metrics["comm_norm"], color='green', label="Comm Norm")
    plt.title("Geometric Alignment Evolution")
    plt.legend()
    
    plt.tight_layout()
    plt.savefig("analysis.png")
    print("Final Analysis saved to analysis.png and metrics.csv")
