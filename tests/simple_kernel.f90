program simple_kernel

  use trusimd

  ! Variables declaration
  character(256) :: buf, argv1
  character(len=:), allocatable :: argv0
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
  do i = 1, n
    a.at.i = i
  end do

  ! Kernel
  !TODO

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
  !TODO

end program simple_kernel
