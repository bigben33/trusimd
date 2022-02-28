program poll_hardware

  use trusimd

  type(trusimd_hardware), allocatable :: hardwares(:)
  
  if (trusimd_poll(hardwares) == -1) then
    print '(A)', trim(trusimd_strerror(trusimd_errno))
    stop 0
  end if

  do i = 1, size(hardwares)
    print '(A)', trim(hardwares(i)%description)
  end do

end program poll_hardware
