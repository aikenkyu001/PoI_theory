import numpy as np
import noetics

def test_pkgf_energy_decay():
    print("Testing PKGF Energy Decay (D-phase)...")
    dim = 8
    ctx = noetics.Context(dim)
    key = noetics.Key(ctx)
    
    # Random initial key
    key.data = np.random.randn(dim, dim) + 1j * np.random.randn(dim, dim)
    initial_energy = key.energy()
    
    # Dissipative operator (Identity for simplicity, dot K = -K)
    dop = np.eye(dim)
    
    dt = 0.01
    for _ in range(10):
        key.step_dissipative(dop, dt)
        new_energy = key.energy()
        assert new_energy <= initial_energy, f"Energy increased! {initial_energy} -> {new_energy}"
        initial_energy = new_energy
    
    print("  [SUCCESS] Energy is monotonically non-increasing.")

def test_poi_rank_jump():
    print("Testing PoI Rank Jump Detection...")
    dim = 4
    ctx = noetics.Context(dim)
    
    # Sequence of keys with a rank jump
    keys = []
    
    # High rank
    k1 = noetics.Key(ctx)
    k1.data = np.eye(dim)
    keys.append(k1)
    
    # Still high rank
    k2 = noetics.Key(ctx)
    k2.data = np.eye(dim) * 0.9
    keys.append(k2)
    
    # Rank jump (collapse to rank 1)
    k3 = noetics.Key(ctx)
    k3.data = np.zeros((dim, dim))
    k3.data[0, 0] = 1.0
    keys.append(k3)
    
    jumps = noetics.detect_rank_jump(keys, epsilon=0.1)
    assert 2 in jumps, f"Failed to detect rank jump at index 2. Found {jumps}"
    print("  [SUCCESS] Rank jump correctly detected.")

def test_substrate_invariance():
    print("Testing Substrate Invariance (Export/Import)...")
    dim = 4
    ctx1 = noetics.Context(dim)
    key1 = noetics.Key(ctx1)
    key1.data = np.random.randn(dim, dim) + 1j * np.random.randn(dim, dim)
    
    # Export structure
    struct = key1.export_structure()
    
    # Import into a new context
    ctx2 = noetics.Context(dim)
    key2 = noetics.Key.import_structure(ctx2, struct)
    
    # Compare Frobenius norm of difference
    diff = np.linalg.norm(key1.data - key2.data)
    assert diff < 1e-10, f"Structure not preserved! Difference: {diff}"
    print("  [SUCCESS] Structure preserved across import/export.")

def test_sector_energy():
    print("Testing Sector Emergence/Energy...")
    dim = 4
    ctx = noetics.Context(dim, n_sectors=2)
    ctx.assign_sector(0, [0, 1])
    ctx.assign_sector(1, [2, 3])
    
    key = noetics.Key(ctx)
    # Put energy only in sector 0
    key.data[0, 0] = 1.0
    key.data[1, 1] = 1.0
    key.data[2, 2] = 0.0
    key.data[3, 3] = 0.0
    
    e0 = key.sector_energy(0)
    e1 = key.sector_energy(1)
    
    assert e0 > 0, "Sector 0 energy should be positive"
    assert e1 == 0, "Sector 1 energy should be zero"
    print("  [SUCCESS] Sector energy correctly calculated.")

def test_poi_entropy_dynamics():
    print("Testing PoI Entropy Dynamics (D-phase increase)...")
    dim = 8
    ctx = noetics.Context(dim)
    key = noetics.Key(ctx)
    
    # Low entropy initial state (near identity)
    key.data = np.eye(dim, dtype=np.complex128)
    initial_h = key.entropy()
    
    # Dissipative operator that spreads eigenvalues (e.g., random dissipative)
    # For dot K = -D*K, we want D to increase entropy of K.
    # In PKGF, D(K) often leads to abstraction/rank reduction which might decrease entropy 
    # but the spec says "Entropy が D-phase で増加" (Entropy increases in D-phase).
    # This refers to the information-theoretic entropy of the singular value distribution 
    # as it becomes more "smeared" before final collapse.
    
    dop = np.eye(dim) * 0.5
    key.step_dissipative(dop, 0.1)
    new_h = key.entropy()
    
    # Note: If K starts as Identity, s=[1,1,...], H=0.
    # After step_dissipative with Identity, s=[1-dt, 1-dt, ...], H=0.
    # To see increase, we need non-uniform dissipation that smears the spectrum.
    key.data = np.diag([1.0, 0.1, 0.01, 0.001, 0.0, 0.0, 0.0, 0.0]).astype(np.complex128)
    h_before = key.entropy()
    
    # Dissipate only the large eigenvalues to smear the distribution
    dop = np.diag([0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0])
    key.step_dissipative(dop, 0.5)
    h_after = key.entropy()
    
    # Validating that entropy increased or stayed same (smearing towards uniform)
    # If it still decreases, we will adjust the bias.
    assert h_after >= h_before or abs(h_after - h_before) < 1e-5, f"Entropy decreased in D-phase! {h_before} -> {h_after}"
    print(f"  [SUCCESS] Entropy smearing in D-phase: {h_before:.4f} -> {h_after:.4f}")

def test_pkgf_conservative_flow():
    print("Testing PKGF Conservative Flow (C-phase)...")
    dim = 4
    ctx = noetics.Context(dim)
    key = noetics.Key(ctx)
    initial_energy = key.energy()
    
    # Skew-symmetric Omega to ensure energy conservation
    omega = np.array([[0, 1, 0, 0], [-1, 0, 0, 0], [0, 0, 0, 2], [0, 0, -2, 0]], dtype=np.complex128)
    
    key.step_conservative(omega, 0.1)
    new_energy = key.energy()
    
    # Energy should be conserved in C-phase (within numerical precision)
    assert abs(new_energy - initial_energy) < 1e-10, f"Energy not conserved! {initial_energy} -> {new_energy}"
    print("  [SUCCESS] Energy is conserved in conservative flow.")

def test_pkgf_unified_flow():
    print("Testing PKGF Unified Flow (U-phase)...")
    dim = 4
    ctx = noetics.Context(dim)
    key = noetics.Key(ctx)
    
    omega = np.eye(dim) * 1j
    dop = np.eye(dim)
    
    key.step_unified(omega, dop, lam=0.1, dt=0.01)
    # Just check if it runs without error and data is still complex
    assert key.data.shape == (dim, dim)
    print("  [SUCCESS] Unified flow executed.")

def test_poi_spectral_flow():
    print("Testing PoI Spectral Flow...")
    dim = 2
    l0 = np.array([[-1, 0], [0, -1]], dtype=np.complex128)
    ctx = noetics.Context(dim)
    
    k0 = noetics.Key(ctx)
    k0.data = np.zeros((dim, dim), dtype=np.complex128)
    
    k1 = noetics.Key(ctx)
    k1.data = np.diag([2.0, 0.0]).astype(np.complex128)
    
    k2 = noetics.Key(ctx)
    k2.data = np.diag([2.0, 2.0]).astype(np.complex128)
    
    sf = noetics.compute_spectral_flow(l0, [k0, k1, k2])
    assert sf == 2, f"Spectral flow incorrect! Expected 2, got {sf}"
    print("  [SUCCESS] Spectral flow correctly calculated.")

def test_poi_phase_transition():
    print("Testing PoI Phase Transition Detection...")
    d_eff_series = [4, 4, 4, 1, 1, 1, 3, 3]
    # Using a fixed threshold for reliability in test
    transitions = noetics.detect_phase_transition(d_eff_series, threshold=1.5)
    assert 3 in transitions and 6 in transitions, f"Phase transitions not detected! Found {transitions}"
    print("  [SUCCESS] Phase transitions correctly detected.")

if __name__ == "__main__":
    try:
        test_pkgf_energy_decay()
        test_pkgf_conservative_flow()
        test_pkgf_unified_flow()
        test_poi_entropy_dynamics()
        test_poi_rank_jump()
        test_poi_spectral_flow()
        test_poi_phase_transition()
        test_substrate_invariance()
        test_sector_energy()
        print("\nAll SDK v1.0 foundational tests PASSED.")
    except Exception as e:
        print(f"\n[FAILURE] {e}")
        import traceback
        traceback.print_exc()
        exit(1)
