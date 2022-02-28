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

  integer, parameter :: TRUSIMD_NOHWD    = -1
  integer, parameter :: TRUSIMD_LLVM     = 0
  integer, parameter :: TRUSIMD_CUDA     = 1
  integer, parameter :: TRUSIMD_OPENCL   = 2

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

  ! Hardware C-struct
  type, bind(c) :: c_trusimd_hardware
    character(kind=c_char) :: id(16)
    character(kind=c_char) :: param1(8)
    character(kind=c_char) :: param2(8)
    integer(c_int)         :: accelerator
    character(kind=c_char) :: description(256)
  end type

  ! Hardware Fortran struct
  type :: trusimd_hardware
    type(c_ptr)    :: h
    integer        :: accelerator
    character(256) :: description
  end type

  ! Memory buffer pair struct
  type :: trusimd_buffer_pair
    integer     :: n
    type(c_ptr) :: h, host_ptr, dev_ptr
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
  ! Make a Fortran string from a C-string
  subroutine to_fortran_string(dst, src)
    character(256), intent(out) :: dst
    character(kind=c_char, len=1), intent(in) :: src(:)
    integer :: i, j
    do i = 1, 256
      dst(i:i) = src(i)
      if (iachar(dst(i:i)) == 0) then
        do j = i, 256
          dst(j:j) = ' '
        end do
        return
      end if
    end do
  end subroutine

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
    character(256) :: s
    character(kind=c_char, len=1), pointer :: c_s(:)
    type(c_ptr) :: s_
    interface
      function c_trusimd_strerror(code_) result(s_) &
               bind(c, name="trusimd_strerror")
        import
        integer(kind=c_int), value :: code_
        type(c_ptr) :: s_
      end function
      function c_strlen(s_) result(l) bind(c, name="strlen")
        import
        type(c_ptr), value :: s_
        integer(kind=c_size_t) :: l
      end function
    end interface
    s_ = c_trusimd_strerror(int(code, kind=c_int))
    call c_f_pointer(s_, c_s, (/ c_strlen(s_) /))
    call to_fortran_string(s, c_s)
  end function

  ! ---------------------------------------------------------------------------
  ! Variable abstraction


  ! ---------------------------------------------------------------------------
  ! Hardware abstraction

  function trusimd_poll(hardwares) result(n)
    type(trusimd_hardware), allocatable, intent(out) :: hardwares(:)
    integer :: i, n
    type(c_trusimd_hardware), pointer :: ptr(:)
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
    call c_f_pointer(ptr_, ptr, (/ n /))
    allocate(hardwares(n))
    do i = 1, n
      hardwares(i)%h = c_loc(ptr(i))
      hardwares(i)%accelerator = int(ptr(i)%accelerator)
      call to_fortran_string(hardwares(i)%description, ptr(i)%description)
    end do
  end function

  function trusimd_find_hardware(s_) result(h)
    character(*), intent(in) :: s_
    character(len=len_trim(s_)) :: s
    character(len=256) :: desc
    type(trusimd_hardware) :: h
    type(trusimd_hardware), allocatable :: hardwares(:)
    integer :: i

    h%accelerator = TRUSIMD_NOHWD
    if (trusimd_poll(hardwares) == -1) then
      return
    end if

    s = s_
    call tolower(s)
    do i = 1, size(hardwares)
      desc = hardwares(i)%description
      call tolower(desc)
      if (index(desc, s) > 0) then
        h = hardwares(i)
        return
      end if
    end do
  end function

  ! ---------------------------------------------------------------------------
  ! Memory buffer abstraction

  function buffer_pair_ctor(h, n, t) result(this)
    type(trusimd_hardware), intent(in) :: h
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
    this%host_ptr = c_malloc(int(this%n, kind=c_size_t))
    this%dev_ptr = c_trusimd_device_malloc(h%h, int(this%n, kind=c_size_t))
    this%h = h%h
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
    call c_trusimd_device_free(this%h, this%dev_ptr)
    call c_free(this%host_ptr)
  end subroutine

  function trusimd_copy_to_device(b) result(code)
    type(trusimd_buffer_pair), intent(inout) :: b
    integer :: code
    interface
      function c_trusimd_copy_to_device(h, dst, src, n) result(code_) &
               bind(c, name="trusimd_copy_to_device")
        import
        type(c_ptr), value :: h, dst, src
        integer(kind=c_size_t), value :: n
        integer(kind=c_int) :: code_
      end function
    end interface
    code = int(c_trusimd_copy_to_device(b%h, b%dev_ptr, b%host_ptr, &
                                        int(b%n, kind=c_size_t)))
  end function

end module trusimd
