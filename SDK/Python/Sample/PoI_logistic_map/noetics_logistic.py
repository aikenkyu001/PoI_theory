import numpy as np
from noetics import Context, Key
from parameters import *

def simulate_noetics():
    x = np.zeros(n_steps)
    x[0] = x0
    d_eff = np.zeros(n_steps)
    alignment = np.zeros(n_steps)
    
    ctx = Context(dim=dim)
    key = Key(ctx)
    
    # 初期構造
    key.data = np.zeros((dim, dim), dtype=np.complex128)
    key.data[0, 0] = x0

    for i in range(1, n_steps):
        current_x = x[i-1]
        
        # Omega: 主要成分への成長と循環を定義
        omega_mat = np.zeros((dim, dim), dtype=np.complex128)
        omega_mat[0, 0] = r * (1.0 - current_x) * 2.0
        # 循環を許容するため、第1列にも成分を追加
        for j in range(1, dim):
            omega_mat[0, j] = r * current_x * 0.5
            omega_mat[j, 0] = r * (1.0 - current_x) * 0.5
        
        # Dissipation: バランスの取れた散逸
        noise = np.random.normal(0, noise_level)
        dop = np.zeros((dim, dim), dtype=np.complex128)
        dop[0, 0] = 0.2 * current_x + noise
        for j in range(1, dim):
            dop[j, j] = 0.5 + noise
        
        # CDU更新 (安定化された dt を使用)
        key.step_unified(omega_mat, dop, lam=lam, dt=dt)
        
        # 主要成分の観測
        val = np.real(key.data[0, 0])
        x[i] = np.clip(val, 0.0, 1.0)
        
        # 構造的観測量
        try:
            d_eff[i] = key.effective_dimension(epsilon)
        except np.linalg.LinAlgError:
            d_eff[i] = d_eff[i-1]
            
        comm = omega_mat @ key.data - key.data @ omega_mat
        alignment[i] = np.linalg.norm(comm)
        
        # 数値的爆発の検知
        if not np.isfinite(x[i]):
            print(f"Numerical instability at step {i}")
            break
    
    return x, d_eff, alignment
