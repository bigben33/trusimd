program simple_kernel

  use trusimd

  ! Variables declaration
  character(256) :: buf
  character(len=:), allocatable :: argv0, argv1
  integer :: n, code
  type(trusimd_hardware), pointer :: h
  type(trusimd_buffer_pair) :: a, b, c

  ! Expect one argument
  call get_command_argument(0, buf)
  argv0 = trim(buf)
  call get_command_argument(1, buf, n, code)
  if (code > 0) then
    print '(256A, 256A, 256A, 256A)', argv0, ': error: usage: ', &
                                      argv0, ' search_string'
    stop -1
  end if
  argv1 = trim(buf)

  ! Poll hardware and select hardware based on argv[1]
  h => trusimd_find_hardware(argv1)
  if (.not.associated(h)) then
    print '(256A, 256A)', argv0, ': info: no hardware selected'
    stop 0
  end if
  print '(256A, 256A, 256A)', argv0, ': info: selected ', h%description

  ! Create memory buffers
  n = 4096
  a = trusimd_buffer_pair(h, n, float32)
  b = trusimd_buffer_pair(h, n, float32)
  c = trusimd_buffer_pair(h, n, float32)

  ! Kernel
  call begin_kernel((/ /))

end program simple_kernel
