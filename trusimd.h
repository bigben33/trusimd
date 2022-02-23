#ifndef TRUSIMD_H
#define TRUSIMD_H

#if __cplusplus > 0
#include <cstdarg>
#include <cstdlib>
#else
#include <stdarg.h>
#include <stdlib.h>
#endif

#if __cplusplus > 0
extern "C" {
#endif

/* ------------------------------------------------------------------------- */

#define TRUSIMD_LLVM     0
#define TRUSIMD_CUDA     1
#define TRUSIMD_OPENCL   2

struct trusimd_hardware {
  char id[2 * sizeof(void *)];
  char param1[sizeof(void *)]; // OpenCL: default context
  char param2[sizeof(void *)]; // OpenCL: default queue/stream
  int accelerator;
  char description[256];
};

struct trusimd_kernel;

#define TRUSIMD_SIGNED    0
#define TRUSIMD_UNSIGNED  1
#define TRUSIMD_FLOAT     2
#define TRUSIMD_BFLOAT    3

#define TRUSIMD_SCALAR    0
#define TRUSIMD_VECTOR    1

struct trusimd_type {
  int scalar_vector, kind, width, nb_times_ptr;
};

const trusimd_type trusimd_notype = {0, 0, 0, 0};

#ifdef _MSC_VER
#define TRUSIMD_TLS __declspec(thread)
#else
#define TRUSIMD_TLS __thread
#endif

extern TRUSIMD_TLS int trusimd_errno;

trusimd_kernel *trusimd_create_kernel_ap(const char *, va_list);
trusimd_kernel *trusimd_create_kernel(const char *, ...);
void trusimd_clear_kernel(trusimd_kernel *);
void trusimd_end_kernel(trusimd_kernel *);
int trusimd_nb_kernel_args(trusimd_kernel *);
int trusimd_get_kernel_arg(trusimd_kernel *, int);
const char *trusimd_get_cuda(trusimd_kernel *);
const char *trusimd_get_opencl(trusimd_kernel *);
const char *trusimd_get_llvmir(trusimd_kernel *);
const char *trusimd_strerror(int);
int trusimd_var(trusimd_kernel *, trusimd_type);
int trusimd_assign(trusimd_kernel *, int, int);
int trusimd_load(trusimd_kernel *, int, int);
int trusimd_store(trusimd_kernel *, int, int, int);
int trusimd_add(trusimd_kernel *, int, int);
int trusimd_get_global_id(trusimd_kernel *);
int trusimd_poll(trusimd_hardware **);
void *trusimd_device_malloc(trusimd_hardware *, size_t);
void trusimd_device_free(trusimd_hardware *, void *);
trusimd_hardware *trusimd_find_first_hardware(trusimd_hardware *, int, int);
int trusimd_copy_to_device(trusimd_hardware *, void *, void *, size_t);
int trusimd_copy_to_host(trusimd_hardware *, void *, void *, size_t);
int trusimd_compile_run(trusimd_hardware *, trusimd_kernel *, int, ...);
int trusimd_compile_run_ap(trusimd_hardware *, trusimd_kernel *, int, va_list);

#define TRUSIMD_NOERR    0
#define TRUSIMD_ENOMEM   1
#define TRUSIMD_ETYPE    2
#define TRUSIMD_EINDEX   3
#define TRUSIMD_ECUDA    4
#define TRUSIMD_EOPENCL  5
#define TRUSIMD_ELLVM    6
#define TRUSIMD_EAVAIL   7

/* ------------------------------------------------------------------------- */

#if __cplusplus > 0
} // extern "C"
#endif

#endif
