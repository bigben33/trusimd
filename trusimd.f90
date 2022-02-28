module trusimd

! -----------------------------------------------------------------------------
! Declarations

  use, intrinsic :: iso_c_binding
  implicit none

  ! Some constant
  integer, parameter :: TRUSIMD_SIGNED   = 0
  integer, parameter :: TRUSIMD_UNSIGNED = 1
  integer, parameter :: TRUSIMD_FLOAT    = 2

  integer, parameter :: TRUSIMD_SCALAR   = 0
  integer, parameter :: TRUSIMD_VECTOR   = 1

  ! Base types
  type, bind(c) :: trusimd_type
    integer(kind=c_int) :: scalar_vector, kind_, width, nb_times_ptr;
  end type

  ! Base types
  type(trusimd_type), parameter :: int8    = trusimd_type(TRUSIMD_SCALAR, TRUSIMD_SIGNED,    8, 0)
  type(trusimd_type), parameter :: uint8   = trusimd_type(TRUSIMD_SCALAR, TRUSIMD_UNSIGNED,  8, 0)
  type(trusimd_type), parameter :: int16   = trusimd_type(TRUSIMD_SCALAR, TRUSIMD_SIGNED,   16, 0)
  type(trusimd_type), parameter :: uint16  = trusimd_type(TRUSIMD_SCALAR, TRUSIMD_UNSIGNED, 16, 0)
  type(trusimd_type), parameter :: int32   = trusimd_type(TRUSIMD_SCALAR, TRUSIMD_SIGNED,   32, 0)
  type(trusimd_type), parameter :: uint32  = trusimd_type(TRUSIMD_SCALAR, TRUSIMD_UNSIGNED, 32, 0)
  type(trusimd_type), parameter :: float32 = trusimd_type(TRUSIMD_SCALAR, TRUSIMD_FLOAT,    32, 0)
  type(trusimd_type), parameter :: int64   = trusimd_type(TRUSIMD_SCALAR, TRUSIMD_SIGNED,   64, 0)
  type(trusimd_type), parameter :: uint64  = trusimd_type(TRUSIMD_SCALAR, TRUSIMD_UNSIGNED, 64, 0)
  type(trusimd_type), parameter :: float64 = trusimd_type(TRUSIMD_SCALAR, TRUSIMD_FLOAT,    64, 0)

  ! Pointers to base types
  type(trusimd_type), parameter :: int8ptr     = trusimd_type(TRUSIMD_SCALAR, TRUSIMD_SIGNED,    8, 1)
  type(trusimd_type), parameter :: uint8ptr    = trusimd_type(TRUSIMD_SCALAR, TRUSIMD_UNSIGNED,  8, 1)
  type(trusimd_type), parameter :: int16ptr    = trusimd_type(TRUSIMD_SCALAR, TRUSIMD_SIGNED,   16, 1)
  type(trusimd_type), parameter :: uint16ptr   = trusimd_type(TRUSIMD_SCALAR, TRUSIMD_UNSIGNED, 16, 1)
  type(trusimd_type), parameter :: float16ptr  = trusimd_type(TRUSIMD_SCALAR, TRUSIMD_FLOAT,    16, 1)
  type(trusimd_type), parameter :: int32ptr    = trusimd_type(TRUSIMD_SCALAR, TRUSIMD_SIGNED,   32, 1)
  type(trusimd_type), parameter :: uint32ptr   = trusimd_type(TRUSIMD_SCALAR, TRUSIMD_UNSIGNED, 32, 1)
  type(trusimd_type), parameter :: float32ptr  = trusimd_type(TRUSIMD_SCALAR, TRUSIMD_FLOAT,    32, 1)
  type(trusimd_type), parameter :: int64ptr    = trusimd_type(TRUSIMD_SCALAR, TRUSIMD_SIGNED,   64, 1)
  type(trusimd_type), parameter :: uint64ptr   = trusimd_type(TRUSIMD_SCALAR, TRUSIMD_UNSIGNED, 64, 1)
  type(trusimd_type), parameter :: float64ptr  = trusimd_type(TRUSIMD_SCALAR, TRUSIMD_FLOAT,    64, 1)

  ! Hardware struct
  type, bind(c) :: trusimd_hardware
    character(kind=c_char) :: id(16)
    character(kind=c_char) :: param1(8)
    character(kind=c_char) :: param2(8)
    integer(c_int)         :: accelerator
    character(kind=c_char) :: description(256)
  end type

  ! Memory buffer pair struct
  type :: trusimd_buffer_pair
    integer                         :: n
    type(trusimd_hardware), pointer :: h
    type(c_ptr)                     :: host_ptr, dev_ptr
  contains
    final :: buffer_pair_dtor
  end type

  interface trusimd_buffer_pair
    procedure :: buffer_pair_ctor
  end interface

  ! Global errno
  integer(c_int), bind(c, name="trusimd_errno") :: trusimd_errno

! -----------------------------------------------------------------------------
! Implementation

contains

  ! ---------------------------------------------------------------------------
  ! strlen function to get C-string length
  pure function c_strlen(s) result(l)
    type(c_ptr), intent(in) :: s
    integer :: l
    interface
      pure function c_strlen_(s_) result(l_) bind(c, name="strlen")
        import
        type(c_ptr), value :: s_
        integer(kind=c_size_t) :: l_
      end function
    end interface
    l = int(c_strlen_(s))
  end function

  ! ---------------------------------------------------------------------------
  ! My sizeof
  function trusimd_sizeof(t) result(s)
    type(trusimd_type), intent(in) :: t
    integer :: s
    s = t%width / 8
  end function

  ! ---------------------------------------------------------------------------
  ! Tolower
  subroutine tolower(s)
    character(*), intent(inout) :: s
    integer :: i, c
    do i = 1, len_trim(s)
      c = iachar(s(i:i))
      if (c >= iachar('A') .and. c <= iachar('Z')) then
        s(i:i) = achar(c - (iachar('A') - iachar('a')))
      end if
    end do
  end subroutine

  ! ---------------------------------------------------------------------------
  ! Error handling
  function trusimd_strerror(code) result(s)
    integer, intent(in) :: code
    character(kind=c_char, len=:), pointer :: s
    type(c_ptr) :: s_
    interface
      function c_trusimd_strerror(code_) result(s_) &
               bind(c, name="trusimd_strerror")
        import
        integer(kind=c_int), value :: code_
        type(c_ptr) :: s_
      end function
    end interface
    s_ = c_trusimd_strerror(int(code, kind=c_int))
    block
      character(kind=c_char, len=c_strlen(s_)), pointer :: s2
      call c_f_pointer(s_, s2)
      s => s2
    end block
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

  function trusimd_find_hardware(s_) result(h)
    character(*), intent(in) :: s_
    character(len=256) :: desc
    character(:), allocatable :: s
    type(trusimd_hardware), pointer :: h, hardwares(:)
    integer :: i, j, k

    h => null()
    if (trusimd_poll(hardwares) == -1) then
      return
    end if

    s = s_
    call tolower(s)
    do i = 1, size(hardwares)
      do j = 1, 256
        desc(j:j) = hardwares(i)%description(j)
        if (iachar(desc(j:j)) == 0) then
          do k = j, 256
            desc(k:k) = ' '
          end do
          exit
        end if
      end do
      call tolower(desc)
      if (index(desc, s) > 0) then
        h => hardwares(i)
        return
      end if
    end do

  end function

  ! ---------------------------------------------------------------------------
  ! Memory buffer abstraction

  function buffer_pair_ctor(h, n, t) result(this)
    type(trusimd_hardware), pointer :: h
    integer, intent(in) :: n
    type(trusimd_type), intent(in) :: t
    type(trusimd_buffer_pair) :: this
    interface
      function c_trusimd_device_malloc(h_, n_) result(ptr_) &
               bind(c, name="trusimd_device_malloc")
        import
        type(c_ptr), value :: h_
        integer(kind=c_size_t), value :: n_
        type(c_ptr) :: ptr_
      end function
      function c_malloc(n_) result(ptr_) bind(c, name="malloc")
        import
        integer(kind=c_size_t), value :: n_
        type(c_ptr) :: ptr_
      end function
    end interface
    this%n = n * trusimd_sizeof(t)
    this%host_ptr = c_malloc(int(n, kind=c_size_t))
    this%dev_ptr = c_trusimd_device_malloc(c_loc(h), int(n, kind=c_size_t))
    this%h => h
  end function

  subroutine buffer_pair_dtor(this)
    type(trusimd_buffer_pair), intent(in) :: this
    interface
      subroutine c_trusimd_device_free(h, ptr) &
                 bind(c, name="trusimd_device_free")
        import
        type(c_ptr), value :: h, ptr
      end subroutine
      subroutine c_free(ptr) bind(c, name="free")
        import
        type(c_ptr), value :: ptr
      end subroutine
    end interface
    call c_trusimd_device_free(c_loc(this%h), this%dev_ptr)
    call c_free(this%host_ptr)
  end subroutine

  function trusimd_copy_to_device(b) result(code)
    type(trusimd_buffer_pair), pointer, intent(inout) :: h
    integer :: code
    interface
      function c_trusimd_copy_to_device(h_, dst, src, n) result(code_)
        type(c_ptr), value :: h_, dst, src
        integer(kind=c_size_t), value :: n
        integer(kind=c_int) :: code_
      end function
    end interface
    code = c_trusimd_copy_to_device(b%h, b%dev_ptr, b%host_ptr, n)
  end function

end module trusimd
