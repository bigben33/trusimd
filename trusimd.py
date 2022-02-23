# -----------------------------------------------------------------------------
# Load trusimd.so

import ctypes as C
LIB = C.CDLL('libtrusimd.so')
LIBC = C.CDLL('libc.so.6')

# Set return types here
LIBC.free.restype = None
LIBC.malloc.restype = C.c_void_p
LIB.trusimd_device_malloc.restype = C.c_void_p
LIB.trusimd_strerror.restype = C.c_char_p
LIB.trusimd_get_cuda.restype = C.c_char_p
LIB.trusimd_get_llvmir.restype = C.c_char_p
LIB.trusimd_get_opencl.restype = C.c_char_p

current_kernel = C.c_void_p(0)

def raise_on_error(a):
    if (type(a) == int and a == -1) or \
       (type(a) == C.c_void_p and (a.value == 0 or a.value == None)):
        raise Exception(LIB.trusimd_strerror(
                            C.c_int.in_dll(LIB, 'trusimd_errno')).decode())

# -----------------------------------------------------------------------------
# Global index type and variable

class gid_class:
    pass

gid = gid_class()

# Some constants
TRUSIMD_SIGNED    = 0
TRUSIMD_UNSIGNED  = 1
TRUSIMD_FLOAT     = 2
TRUSIMD_BFLOAT    = 3

TRUSIMD_SCALAR    = 0
TRUSIMD_VECTOR    = 1

# Base types
int8     = [TRUSIMD_SCALAR, TRUSIMD_SIGNED,    8, 0, C.c_byte];
uint8    = [TRUSIMD_SCALAR, TRUSIMD_UNSIGNED,  8, 0, C.c_ubyte];
int16    = [TRUSIMD_SCALAR, TRUSIMD_SIGNED,   16, 0, C.c_short];
uint16   = [TRUSIMD_SCALAR, TRUSIMD_UNSIGNED, 16, 0, C.c_ushort];
int32    = [TRUSIMD_SCALAR, TRUSIMD_SIGNED,   32, 0, C.c_int];
uint32   = [TRUSIMD_SCALAR, TRUSIMD_UNSIGNED, 32, 0, C.c_uint];
float32  = [TRUSIMD_SCALAR, TRUSIMD_FLOAT,    32, 0, C.c_float];
int64    = [TRUSIMD_SCALAR, TRUSIMD_SIGNED,   64, 0, C.c_longlong];
uint64   = [TRUSIMD_SCALAR, TRUSIMD_UNSIGNED, 64, 0, C.c_ulonglong];
float64  = [TRUSIMD_SCALAR, TRUSIMD_FLOAT,    64, 0, C.c_double];

# Pointers to base types
int8ptr     = [TRUSIMD_SCALAR, TRUSIMD_SIGNED,    8, 1, C.c_void_p];
uint8ptr    = [TRUSIMD_SCALAR, TRUSIMD_UNSIGNED,  8, 1, C.c_void_p];
int16ptr    = [TRUSIMD_SCALAR, TRUSIMD_SIGNED,   16, 1, C.c_void_p];
uint16ptr   = [TRUSIMD_SCALAR, TRUSIMD_UNSIGNED, 16, 1, C.c_void_p];
float16ptr  = [TRUSIMD_SCALAR, TRUSIMD_FLOAT,    16, 1, C.c_void_p];
bfloat16ptr = [TRUSIMD_SCALAR, TRUSIMD_BFLOAT,   16, 1, C.c_void_p];
int32ptr    = [TRUSIMD_SCALAR, TRUSIMD_SIGNED,   32, 1, C.c_void_p];
uint32ptr   = [TRUSIMD_SCALAR, TRUSIMD_UNSIGNED, 32, 1, C.c_void_p];
float32ptr  = [TRUSIMD_SCALAR, TRUSIMD_FLOAT,    32, 1, C.c_void_p];
int64ptr    = [TRUSIMD_SCALAR, TRUSIMD_SIGNED,   64, 1, C.c_void_p];
uint64ptr   = [TRUSIMD_SCALAR, TRUSIMD_UNSIGNED, 64, 1, C.c_void_p];
float64ptr  = [TRUSIMD_SCALAR, TRUSIMD_FLOAT,    64, 1, C.c_void_p];

# -----------------------------------------------------------------------------
# Variable

class var:
    def __init__(self, var_id = -1):
        self.var_id = var_id

    def __add__(self, other):
        res = var(LIB.trusimd_add(current_kernel, self.var_id, other.var_id))
        raise_on_error(res.var_id)
        return res

    def __getitem__(self, index):
        if type(index) == gid_class:
            i = LIB.trusimd_get_global_id(current_kernel)
        else:
            i = index.var_id
        res = var(LIB.trusimd_load(current_kernel, self.var_id, i))
        raise_on_error(res.var_id)
        return res

    def __setitem__(self, index, value):
        if type(index) == gid_class:
            i = LIB.trusimd_get_global_id(current_kernel)
        else:
            i = index.var_id
        raise_on_error(LIB.trusimd_store(current_kernel, self.var_id, i,
                                         value.var_id))

    def __setattr__(self, member, value):
        if member != 'value':
            self.__dict__[member] = value
        else:
            raise_on_error(LIB.trusimd_assign(current_kernel, self.var_id,
                                              value.var_id))

# -----------------------------------------------------------------------------
# Hardware abstraction

TRUSIMD_LLVM   = 0
TRUSIMD_CUDA   = 1
TRUSIMD_OPENCL = 2

class c_hardware(C.Structure):
    _fields_ = [('id', C.c_char * (2 * C.sizeof(C.c_void_p))),
                ('param1', C.c_char * C.sizeof(C.c_void_p)),
                ('param2', C.c_char * C.sizeof(C.c_void_p)),
                ('accelerator', C.c_int),
                ('description', C.c_char * 256)]

class hardware:
    def __init__(self, ptr):
        self.ptr = ptr
        self.description = C.cast(ptr[0].description, C.c_char_p) \
                           .value.decode()
        self.accelerator = ptr[0].accelerator

    def __repr__(self):
        return self.description

    def __str__(self):
        return self.description

def poll_hardware():
    hs = C.POINTER(c_hardware)(c_hardware())
    n = LIB.trusimd_poll(C.byref(hs))
    raise_on_error(n)
    res = []
    for i in range(n):
        res.append(hardware(C.pointer(hs[i])))
    return res

# -----------------------------------------------------------------------------
# Memory buffer abstraction

def sizeof(t):
    return t[2] // 8

class buffer_pair:
    def __init__(self, h, n, t):
        self.t = t
        self.n = n * sizeof(t)
        self.h = h
        self.host_ptr = C.c_void_p(LIBC.malloc(C.c_size_t(n * sizeof(t))))
        raise_on_error(self.host_ptr)
        self.dev_ptr = C.c_void_p(
            LIB.trusimd_device_malloc(h.ptr, C.c_size_t(n * sizeof(t))))
        if self.dev_ptr == 0:
            LIBC.free(self.host_ptr)
            raise_on_error(self.dev_ptr)

    def __del__(self):
        if 'host_ptr' in self.__dict__:
            LIBC.free(self.host_ptr)
        if 'dev_ptr' in self.__dict__ and 'h' in self.__dict__:
            LIB.trusimd_device_free(self.h.ptr, self.dev_ptr)

    def __getitem__(self, index):
        ptr = C.cast(self.host_ptr, C.POINTER(self.t[4]))
        return ptr[index]

    def __setitem__(self, index, value):
        ptr = C.cast(self.host_ptr, C.POINTER(self.t[4]))
        ptr[index] = value

    def copy_to_device(self):
        raise_on_error(LIB.trusimd_copy_to_device(self.h.ptr, self.dev_ptr,
                                                  self.host_ptr, self.n))
    def copy_to_host(self):
        raise_on_error(LIB.trusimd_copy_to_host(self.h.ptr, self.host_ptr,
                                                self.dev_ptr, self.n))

# -----------------------------------------------------------------------------
# Kernel abstraction

class c_trusimd_type(C.Structure):
    _fields_ = [('scalar_vector', C.c_int),
                ('kind', C.c_int),
                ('width', C.c_int),
                ('nb_times_ptr', C.c_int)]

    def from_param(t):
        res = c_trusimd_type()
        res.scalar_vector = t[0]
        res.kind = t[1]
        res.width = t[2]
        res.nb_times_ptr = t[3]
        return res

c_trusimd_notype = c_trusimd_type.from_param([0, 0, 0, 0])

class kernel:
    def __init__(self, name, *args):
        LIB.trusimd_create_kernel.argstypes = [C.c_char_p] + \
                                              ([c_trusimd_type] * len(args))
        global current_kernel
        current_kernel = C.c_void_p(LIB.trusimd_create_kernel(
            C.c_char_p(name.encode()),
            *[c_trusimd_type.from_param(a) for a in args],
            c_trusimd_notype))
        raise_on_error(current_kernel)
        self.k = current_kernel

    def __del__(self):
        if 'k' in self.__dict__:
            LIB.trusimd_clear_kernel(self.k)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        raise_on_error(LIB.trusimd_end_kernel(current_kernel))
        return False

    def __repr__(self):
        return '============================================\n' + \
               'CUDA:\n\n' + \
               LIB.trusimd_get_cuda(current_kernel).decode() + '\n\n' + \
               '============================================\n' + \
               'OpenCL:\n\n' + \
               LIB.trusimd_get_opencl(current_kernel).decode() + '\n\n' + \
               '============================================\n' + \
               'LLVM IR:\n\n' + \
               LIB.trusimd_get_llvmir(current_kernel).decode() + '\n\n'

    def __str__(self):
        return self.__repr__()

    def run(self, h, n, *args):
        def typ(value):
            if type(value) == int:
                return C.c_int
            elif type(value) == float:
                return C.c_float
            elif type(value) == buffer_pair:
                return C.c_void_p
            else:
                return type(value)

        def val(value):
            if type(value) == buffer_pair:
                return value.dev_ptr
            else:
                return typ(value)(value)

        LIB.trusimd_compile_run.argstypes = \
            [C.POINTER(c_hardware), C.c_void_p, C.c_int] + \
            [typ(t) for t in args]
        raise_on_error(LIB.trusimd_compile_run(
            h.ptr, current_kernel, n, *[val(v) for v in args]))

# -----------------------------------------------------------------------------

def arg(i):
    n = LIB.trusimd_nb_kernel_args(current_kernel)
    if i < 0 or i >= n:
        raise Exception('Index argument out of range')
    res = var(LIB.trusimd_get_kernel_arg(current_kernel, i))
    return res
