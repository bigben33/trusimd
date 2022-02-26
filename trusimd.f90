module trusimd

! -----------------------------------------------------------------------------
! Declarations

  use, intrinsic :: iso_c_binding

  implicit none

  ! Global errno
  integer(c_int), bind(c, name="trusimd_errno") :: trusimd_errno

  ! Hardware struct
  type, bind(c) :: trusimd_hardware
    character(kind=c_char) :: id(16)
    character(kind=c_char) :: param1(8)
    character(kind=c_char) :: param2(8)
    integer(c_int)         :: accelerator
    character(kind=c_char) :: description(256)
  end type

! -----------------------------------------------------------------------------
! Implementation

contains

  ! ---------------------------------------------------------------------------
  ! strlen function to get C-string length
  function c_strlen(s) result(l)
    type(c_ptr), intent(in) :: s
    integer :: l
    interface
      function c_strlen_(s_) result(l_) bind(c, name="strlen")
        import
        type(c_ptr), value :: s_
        integer(kind=c_size_t) :: l_
      end function
    end interface
    l = int(c_strlen_(s))
  end function

  ! ---------------------------------------------------------------------------
  ! Error handling

  function trusimd_strerror(code) result(s)
    integer, intent(in) :: code
    character(kind=c_char), pointer :: s(:)
    interface
      function c_trusimd_strerror(code_) result(s_) &
               bind(c, name="trusimd_strerror")
        import
        integer(kind=c_int), value :: code_
        type(c_ptr) :: s_
      end function
    end interface
    type(c_ptr) :: s_
    s_ = c_trusimd_strerror(int(code, kind=c_int))
    call c_f_pointer(s_, s, [c_strlen(s_)])
  end function

  ! ---------------------------------------------------------------------------
  ! Variable abstraction


  ! ---------------------------------------------------------------------------
  ! Hardware abstraction

  function trusimd_poll(ptr) result(n)
    type(trusimd_hardware), pointer, intent(out) :: ptr(:)
    integer :: n
    interface
      function c_trusimd_poll(ptr_) result(n_) bind(c, name="trusimd_poll")
        import
        type(c_ptr), intent(out) :: ptr_
        integer(kind=c_int) :: n_
      end function
    end interface
    type(c_ptr) :: ptr_
    n = int(c_trusimd_poll(ptr_))
    if (n == -1) then
      return
    end if
    call c_f_pointer(ptr_, ptr, [n])
  end function

end module trusimd
