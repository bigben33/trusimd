program simple_kernel

  use trusimd

  ! Variables declaration
  character(256) :: buf, argv1
  character(len=:), allocatable :: argv0
  real(kind=c_float), dimension(:), pointer :: fort_a, fort_b, fort_c
  integer :: i, n, code
  type(trusimd_hardware) :: h
  type(trusimd_buffer_pair) :: a, b, c

  ! Expect one argument
  call get_command_argument(0, buf)
  argv0 = trim(buf)
  call get_command_argument(1, argv1, n, code)
  if (code > 0) then
    print '(4A)', argv0, ': error: usage: ', argv0, ' search_string'
    stop -1
  end if

  ! Poll hardware and select hardware based on argv[1]
  h = trusimd_find_hardware(argv1)
  if (h%accelerator == TRUSIMD_NOHWD) then
    print '(2A)', argv0, ': info: no hardware selected'
    stop 0
  end if
  print '(3A)', argv0, ': info: selected ', trim(h%description)

  ! Create memory buffers
  n = 4096
  a = trusimd_buffer_pair(h, n, float32)
  b = trusimd_buffer_pair(h, n, float32)
  c = trusimd_buffer_pair(h, n, float32)

  ! Fill buffers with numbers
  call c_f_pointer(b%host_ptr, fort_b, (/ n /))
  call c_f_pointer(c%host_ptr, fort_c, (/ n /))
  do i = 1, n
    fort_b(i) = real(i, kind=c_float)
    fort_c(i) = real(i, kind=c_float)
  end do

  ! Kernel
  call st(arg(0), ld(arg(1)) + ld(arg(2)))

  ! Print kernel
  !TODO

  ! Copy data to device, compile and execute kernel
  if (trusimd_copy_to_device(b) == -1) then
    print '(3A)', argv0, ': error: ', trim(trusimd_strerror(trusimd_errno))
    stop -1
  end if
  if (trusimd_copy_to_device(c) == -1) then
    print '(3A)', argv0, ': error: ', trim(trusimd_strerror(trusimd_errno))
    stop -1
  end if

  ! Check result
  if (trusimd_copy_to_host(a) == -1) then
    print '(3A)', argv0, ': error: ', trim(trusimd_strerror(trusimd_errno))
    stop -1
  end if
  call c_f_pointer(a%host_ptr, fort_a, (/ n /))
  do i = 1, n
    if (fort_a(i) /= fort_b(i) + fort_c(i)) then
      print '(2A, F0.0, A, F0.0)', argv0, ': error: ', fort_a(i), ' vs. ', &
            (fort_b(i) + fort_c(i))
      stop -1
    else
      print '(2A, F0.0, A, F0.0)', argv0, ': error: ', fort_a(i), ' --> ', &
            (fort_b(i) + fort_c(i))
    end if
  end do

end program simple_kernel
