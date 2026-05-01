program test_noetics_f
  use noetics_mod
  implicit none

  type(noetics_context) :: ctx
  type(noetics_key) :: key
  real(kind=8) :: energy

  print *, "Testing Native Fortran SDK (Static Allocation)..."

  ! Initialize context
  call noetics_init_context(ctx, 4, 1)
  
  ! Initialize key
  call noetics_init_key(key, ctx)

  ! Initial Identity matrix energy
  energy = noetics_compute_energy(key)
  print *, "  Initial Energy:", energy
  
  if (abs(energy - 2.0d0) < 1.0d-7) then
     print *, "  [SUCCESS] Energy calculation correct."
  else
     print *, "  [FAILURE] Energy calculation incorrect:", energy
     stop 1
  end if

  print *, "All Fortran SDK native standalone tests PASSED."

end program test_noetics_f
