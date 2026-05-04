module noetics_mod
  implicit none
  integer, parameter :: dp = kind(1.0d0)

  type :: noetics_op
     integer :: dim
     real(dp), allocatable :: data(:,:)
  end type noetics_op

  type :: noetics_structure
     integer :: dim
     real(dp), allocatable :: eigenvalues(:)
     real(dp), allocatable :: eigenvectors(:,:)
  end type noetics_structure

  type :: noetics_context
     integer :: dim
     integer :: n_sectors
     integer, allocatable :: sector_counts(:)
     integer, allocatable :: sector_map(:,:) ! (n_sectors, dim)
  end type noetics_context

  type :: noetics_key
     integer :: dim
     real(dp), allocatable :: data(:,:)
     type(noetics_context), pointer :: ctx => null()
  end type noetics_key

contains

  subroutine noetics_init_context(ctx, dim_in, n_sectors_in)
    type(noetics_context), intent(out) :: ctx
    integer, intent(in) :: dim_in, n_sectors_in
    ctx%dim = dim_in
    ctx%n_sectors = n_sectors_in
    allocate(ctx%sector_counts(n_sectors_in))
    allocate(ctx%sector_map(n_sectors_in, dim_in))
    ctx%sector_counts = 0
    ctx%sector_map = 0
  end subroutine noetics_init_context

  subroutine noetics_init_key(key, ctx)
    type(noetics_key), intent(out) :: key
    type(noetics_context), target, intent(in) :: ctx
    integer :: i
    key%dim = ctx%dim
    key%ctx => ctx
    allocate(key%data(ctx%dim, ctx%dim))
    key%data = 0.0_dp
    do i = 1, ctx%dim
       key%data(i,i) = 1.0_dp
    end do
  end subroutine noetics_init_key

  ! --- Flows ---
  subroutine noetics_step_conservative(key, omega, dt)
    type(noetics_key), intent(inout) :: key
    type(noetics_op), intent(in) :: omega
    real(dp), intent(in) :: dt
    real(dp), allocatable :: comm(:,:)
    allocate(comm(key%dim, key%dim))
    comm = matmul(omega%data, key%data) - matmul(key%data, omega%data)
    key%data = key%data + comm * dt
  end subroutine noetics_step_conservative

  subroutine noetics_step_dissipative(key, dop, dt)
    type(noetics_key), intent(inout) :: key
    type(noetics_op), intent(in) :: dop
    real(dp), intent(in) :: dt
    key%data = key%data - matmul(dop%data, key%data) * dt
  end subroutine noetics_step_dissipative

  subroutine noetics_step_unified(key, omega, dop, lambda, dt)
    type(noetics_key), intent(inout) :: key
    type(noetics_op), intent(in) :: omega, dop
    real(dp), intent(in) :: lambda, dt
    real(dp), allocatable :: comm(:,:)
    allocate(comm(key%dim, key%dim))
    comm = matmul(omega%data, key%data) - matmul(key%data, omega%data)
    key%data = key%data + (comm - lambda * matmul(dop%data, key%data)) * dt
  end subroutine noetics_step_unified

  ! --- Observables ---
  function noetics_compute_energy(key) result(e)
    type(noetics_key), intent(in) :: key
    real(dp) :: e
    e = 0.5_dp * sum(key%data**2)
  end function noetics_compute_energy

  function noetics_compute_entropy(key) result(h)
    type(noetics_key), intent(in) :: key
    real(dp) :: h, evals(key%dim), evecs(key%dim, key%dim), s_sum, p
    integer :: i
    call jacobi_eigen(key%dim, key%data, evals, evecs)
    s_sum = sum(abs(evals))
    h = 0.0_dp
    if (s_sum > 1.0d-12) then
       do i = 1, key%dim
          p = abs(evals(i)) / s_sum
          if (p > 1.0d-12) h = h - p * log(p)
       end do
    end if
  end function noetics_compute_entropy

  function noetics_compute_d_eff(key, eps) result(d)
    type(noetics_key), intent(in) :: key
    real(dp), intent(in) :: eps
    real(dp) :: d, evals(key%dim), evecs(key%dim, key%dim)
    integer :: i
    call jacobi_eigen(key%dim, key%data, evals, evecs)
    d = 0.0_dp
    do i = 1, key%dim
       if (abs(evals(i)) > eps) d = d + 1.0_dp
    end do
  end function noetics_compute_d_eff

  ! --- Sector Management ---
  subroutine noetics_assign_sector(ctx, sector_id, indices)
    type(noetics_context), intent(inout) :: ctx
    integer, intent(in) :: sector_id, indices(:)
    integer :: i
    if (sector_id < 1 .or. sector_id > ctx%n_sectors) return
    ctx%sector_counts(sector_id) = size(indices)
    do i = 1, size(indices)
       ctx%sector_map(sector_id, i) = indices(i)
    end do
  end subroutine noetics_assign_sector

  function noetics_compute_sector_energy(key, sector_id) result(e)
    type(noetics_key), intent(in) :: key
    integer, intent(in) :: sector_id
    real(dp) :: e
    integer :: i, j, idx_i, idx_j, n
    e = 0.0_dp
    if (.not. associated(key%ctx)) return
    n = key%ctx%sector_counts(sector_id)
    do i = 1, n
       idx_i = key%ctx%sector_map(sector_id, i)
       do j = 1, n
          idx_j = key%ctx%sector_map(sector_id, j)
          e = e + key%data(idx_i, idx_j)**2
       end do
    end do
    e = 0.5_dp * e
  end function noetics_compute_sector_energy

  ! --- Substrate Invariance ---
  subroutine noetics_export_structure(key, struct)
    type(noetics_key), intent(in) :: key
    type(noetics_structure), intent(out) :: struct
    struct%dim = key%dim
    allocate(struct%eigenvalues(key%dim))
    allocate(struct%eigenvectors(key%dim, key%dim))
    call jacobi_eigen(key%dim, key%data, struct%eigenvalues, struct%eigenvectors)
  end subroutine noetics_export_structure

  subroutine noetics_import_structure(key, ctx, struct)
    type(noetics_key), intent(out) :: key
    type(noetics_context), target, intent(in) :: ctx
    type(noetics_structure), intent(in) :: struct
    real(dp), allocatable :: temp(:,:)
    integer :: i, j, k, n
    call noetics_init_key(key, ctx)
    n = struct%dim
    allocate(temp(n,n))
    ! K = V * diag(L) * V^T
    do i = 1, n; do j = 1, n
       temp(i,j) = struct%eigenvectors(i,j) * struct%eigenvalues(j)
    end do; end do
    do i = 1, n; do j = 1, n
       key%data(i,j) = sum(temp(i,:) * struct%eigenvectors(j,:))
    end do; end do
  end subroutine noetics_import_structure

  ! --- Internal Math ---
  ! --- Spectral Flow ---
  function noetics_compute_spectral_flow(l0, keys, n_steps) result(sf)
    type(noetics_op), intent(in) :: l0
    type(noetics_key), intent(in) :: keys(n_steps)
    integer, intent(in) :: n_steps
    integer :: sf
    real(dp) :: prev_evals(l0%dim), curr_evals(l0%dim)
    real(dp) :: temp_evecs(l0%dim, l0%dim)
    integer :: i, j

    sf = 0
    if (n_steps < 2) return

    ! Get initial eigenvalues
    call jacobi_eigen(l0%dim, l0%data + keys(1)%data, prev_evals, temp_evecs)
    call shell_sort(prev_evals)

    do i = 2, n_steps
       call jacobi_eigen(l0%dim, l0%data + keys(i)%data, curr_evals, temp_evecs)
       call shell_sort(curr_evals)
       do j = 1, l0%dim
          if (prev_evals(j) < 0.0_dp .and. curr_evals(j) >= 0.0_dp) then
             sf = sf + 1
          else if (prev_evals(j) >= 0.0_dp .and. curr_evals(j) < 0.0_dp) then
             sf = sf - 1
          end if
       end do
       prev_evals = curr_evals
    end do
  end function noetics_compute_spectral_flow

  ! --- Rank Jump Detection ---
  function noetics_detect_rank_jump(keys, n_steps, eps) result(jump_idx)
    type(noetics_key), intent(in) :: keys(n_steps)
    integer, intent(in) :: n_steps
    real(dp), intent(in) :: eps
    integer :: jump_idx
    real(dp) :: prev_rank, curr_rank
    integer :: i

    jump_idx = -1
    if (n_steps < 2) return

    prev_rank = noetics_compute_d_eff(keys(1), eps)
    do i = 2, n_steps
       curr_rank = noetics_compute_d_eff(keys(i), eps)
       if (abs(curr_rank - prev_rank) > 0.5_dp) then
          jump_idx = i
          return
       end if
       prev_rank = curr_rank
    end do
  end function noetics_detect_rank_jump

  ! --- Internal Utilities ---
  subroutine shell_sort(a)
    real(dp), intent(inout) :: a(:)
    integer :: n, i, j, h
    real(dp) :: v
    n = size(a)
    h = 1
    do while (h <= n/3); h = 3*h + 1; end do
    do while (h >= 1)
       do i = h + 1, n
          v = a(i)
          j = i
          do while (j > h)
             if (a(j-h) <= v) exit
             a(j) = a(j-h)
             j = j - h
          end do
          a(j) = v
       end do
       h = h / 3
    end do
  end subroutine shell_sort

  subroutine jacobi_eigen(n, A_in, evals, evecs)
    integer, intent(in) :: n
    real(dp), intent(in) :: A_in(n,n)
    real(dp), intent(out) :: evals(n), evecs(n,n)
    real(dp) :: A(n,n), theta, tau, t, c, s, a_pp, a_qq, a_pq, a_ip, a_iq, max_off
    integer :: i, j, p, q, iter
    A = A_in
    evecs = 0.0_dp
    do i = 1, n; evecs(i,i) = 1.0_dp; end do
    do iter = 1, 50
       max_off = 0.0_dp; p = 1; q = 2
       do i = 1, n; do j = i + 1, n
          if (abs(A(i,j)) > max_off) then; max_off = abs(A(i,j)); p = i; q = j; end if
       end do; end do
       if (max_off < 1.0d-15) exit
       a_pp = A(p,p); a_qq = A(q,q); a_pq = A(p,q)
       theta = (a_qq - a_pp) / (2.0_dp * a_pq)
       if (theta >= 0.0_dp) then; t = 1.0_dp / (theta + sqrt(1.0_dp + theta**2))
       else; t = -1.0_dp / (-theta + sqrt(1.0_dp + theta**2)); end if
       c = 1.0_dp / sqrt(1.0_dp + t**2); s = t * c; tau = s / (1.0_dp + c)
       A(p,p) = a_pp - t * a_pq; A(q,q) = a_qq + t * a_pq; A(p,q) = 0.0_dp; A(q,p) = 0.0_dp
       do i = 1, n; if (i /= p .and. i /= q) then
          a_ip = A(i,p); a_iq = A(i,q)
          A(i,p) = a_ip - s * (a_iq + tau * a_ip); A(p,i) = A(i,p)
          A(i,q) = a_iq + s * (a_ip - tau * a_iq); A(q,i) = A(i,q)
       end if; end do
       do i = 1, n
          a_ip = evecs(i,p); a_iq = evecs(i,q)
          evecs(i,p) = a_ip - s * (a_iq + tau * a_ip)
          evecs(i,q) = a_iq + s * (a_ip - tau * a_iq)
       end do
    end do
    do i = 1, n; evals(i) = A(i,i); end do
  end subroutine jacobi_eigen

end module noetics_mod
