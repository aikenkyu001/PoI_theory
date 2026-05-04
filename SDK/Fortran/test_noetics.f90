program test_noetics_f
  use noetics_mod
  implicit none

  type(noetics_context), target :: ctx
  type(noetics_key) :: key
  real(dp) :: energy

  print *, "Testing Native Fortran SDK (Comprehensive)..."

  ! 1. Lifecycle & Energy
  call noetics_init_context(ctx, 4, 2)
  call noetics_init_key(key, ctx)
  energy = noetics_compute_energy(key)
  print *, "  Initial Energy:", energy
  if (abs(energy - 2.0_dp) > 1.0d-7) stop 1

  ! 2. Flows
  block
    type(noetics_op) :: omega, dop
    allocate(omega%data(4,4), dop%data(4,4))
    omega%data = 0.0_dp; dop%data = 0.0_dp
    omega%dim = 4; dop%dim = 4
    omega%data(1,2) = 1.0_dp; omega%data(2,1) = -1.0_dp
    
    call noetics_step_conservative(key, omega, 0.1_dp)
    print *, "  Energy after C-step:", noetics_compute_energy(key)
    if (abs(noetics_compute_energy(key) - 2.0_dp) > 1.0d-7) stop 2
    
    dop%data(1,1) = 1.0_dp; dop%data(2,2) = 1.0_dp; dop%data(3,3) = 1.0_dp; dop%data(4,4) = 1.0_dp
    call noetics_step_dissipative(key, dop, 0.1_dp)
    print *, "  Energy after D-step:", noetics_compute_energy(key)
    if (noetics_compute_energy(key) >= 2.0_dp) stop 3

    ! Unified step
    call noetics_step_unified(key, omega, dop, 0.5_dp, 0.01_dp)
    print *, "  Energy after U-step:", noetics_compute_energy(key)
  end block

  ! 3. Observables
  print *, "  Entropy:", noetics_compute_entropy(key)
  print *, "  D_eff:", noetics_compute_d_eff(key, 0.1_dp)

  ! 4. Sectors
  block
    integer :: s0_idx(2)
    s0_idx = [1, 2]
    call noetics_assign_sector(ctx, 1, s0_idx)
    print *, "  Sector 1 Energy:", noetics_compute_sector_energy(key, 1)
    if (noetics_compute_sector_energy(key, 1) < 1.0d-7) stop 4
  end block

  ! 5. Substrate Invariance
  block
    type(noetics_structure) :: struct
    type(noetics_key) :: key2
    call noetics_export_structure(key, struct)
    call noetics_import_structure(key2, ctx, struct)
    print *, "  Imported Energy:", noetics_compute_energy(key2)
    if (abs(noetics_compute_energy(key) - noetics_compute_energy(key2)) > 1.0d-7) stop 5
  end block

  ! 6. Spectral Flow & Rank Jump (Previously added)
  block
    type(noetics_op) :: l0
    type(noetics_key) :: keys(3)
    integer :: sf, jump
    l0%dim = 2
    allocate(l0%data(2,2))
    l0%data = 0.0d0
    l0%data(1,1) = -1.0d0
    l0%data(2,2) = -1.0d0
    call noetics_init_key(keys(1), ctx)
    keys(1)%data = 0.0d0
    call noetics_init_key(keys(2), ctx)
    keys(2)%data = 0.0d0; keys(2)%data(1,1) = 2.0d0
    call noetics_init_key(keys(3), ctx)
    keys(3)%data = 0.0d0; keys(3)%data(1,1) = 2.0d0; keys(3)%data(2,2) = 2.0d0
    sf = noetics_compute_spectral_flow(l0, keys, 3)
    print *, "  Spectral Flow:", sf
    if (sf /= 2) stop 6
    
    jump = noetics_detect_rank_jump(keys(1:2), 2, 0.1d0)
    print *, "  Rank Jump Index:", jump
    if (jump /= 2) stop 7
  end block

  print *, "All Fortran SDK native standalone tests PASSED."

end program test_noetics_f
