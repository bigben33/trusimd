program poll_hardware

  use trusimd

  type(trusimd_hardware), pointer :: hardwares(:)

  if (trusimd_poll(hardwares) == -1) then
    print '(256A)', trusimd_strerror(trusimd_errno)
    stop 0
  end if

  do i = 1, size(hardwares)
    print '(256A)', hardwares(i) % description
  end do

end program poll_hardware
