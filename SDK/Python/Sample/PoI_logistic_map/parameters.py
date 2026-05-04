import numpy as np

# PoI/PKGF 実験パラメータ
n_steps = 500
r = 3.8
x0 = 0.1
noise_level = 0.04

# Noetics パラメータ
dim = 4
lam = 0.5        # 散逸を強めて安定化
dt = 0.05       # タイムステップを大幅に縮小
epsilon = 1e-2
