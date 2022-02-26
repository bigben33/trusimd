program poll_hardware

  use trusimd

  integer :: n
  type(trusimd_hardware), pointer :: hardwares(:)

  call trusimd_poll(hardwares, n)

  print *, trusimd_strerror(0)
  print *, trusimd_strerror(1)
  print *, trusimd_strerror(2)
  print *, trusimd_strerror(3)
  print *, trusimd_strerror(4)

  if (n == -1) then
    print '(256A)', trusimd_strerror(trusimd_errno)
    stop 0
  end if

  do i = 1, n
    print '(256A)', hardwares(i) % description
  end do

end program poll_hardware
