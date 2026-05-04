import numpy as np
from parameters import *

def simulate_standard():
    x = np.zeros(n_steps)
    x[0] = x0
    for i in range(1, n_steps):
        # 標準的なロジリスティック写像 + 外乱
        noise = np.random.normal(0, noise_level)
        x[i] = np.clip(r * x[i-1] * (1 - x[i-1]) + noise, 0.0, 1.0)
    return x
