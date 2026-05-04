import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from standard_logistic import simulate_standard
from noetics_logistic import simulate_noetics
from parameters import n_steps, r, noise_level

import matplotlib
matplotlib.use('Agg')

print(f"Running Experiment: r={r}, noise={noise_level}")

std = simulate_standard()
noe, d_eff, alignment = simulate_noetics()

# CSV出力用のデータフレーム作成
df = pd.DataFrame({
    'step': np.arange(n_steps),
    'standard_x': std,
    'noetics_x': noe,
    'effective_dimension': d_eff,
    'geometric_alignment': alignment
})
df.to_csv('experiment_results.csv', index=False)
print("Experiment results saved to experiment_results.csv")

fig, axes = plt.subplots(3, 1, figsize=(12, 12), sharex=True)

# 1. 時系列比較（復元能力の視覚化）
axes[0].plot(std, label='Standard (Noise-prone)', alpha=0.5, color='gray')
axes[0].plot(noe, label='Noetics (Autonomous Restoration)', color='blue', linewidth=2)
axes[0].set_title('Time Series: Standard vs Noetics (Structural Restoration)')
axes[0].legend()
axes[0].grid(True)

# 2. 有効次元 (Predictive Capacity)
axes[1].plot(d_eff, color='green')
axes[1].set_title('Effective Dimension ($d_{eff}$) - Predictive Capacity')
axes[1].set_ylabel('Dimension')
axes[1].grid(True)

# 3. 幾何学的整合性 (Alignment)
axes[2].plot(alignment, color='red')
axes[2].set_title('Geometric Alignment |[$\Omega, K$]| - Conflict/Entropy')
axes[2].set_xlabel('Iteration')
axes[2].set_ylabel('Commutator Norm')
axes[2].grid(True)

plt.tight_layout()
plt.savefig('poi_logistic_analysis.png')
print("Analysis plot saved as poi_logistic_analysis.png")

# 統計的なRMSEではなく、構造の「安定度」を評価
stability = np.std(noe)
print(f"Noetics Structural Stability (StdDev): {stability:.5f}")
print(f"Final Effective Dimension: {d_eff[-1]}")
