#ifdef WITH_CUDA
#include <cuda_runtime_api.h>
#include <cuda_runtime.h>
#include <nvrtc.h>
#include <cuda.h>

// ----------------------------------------------------------------------------

#define CUDART_ERROR 0
#define NVRTC_ERROR  1
#define CU_ERROR     2

TRUSIMD_TLS int cuda_error_type;
TRUSIMD_TLS cudaError_t cuda_errno;
TRUSIMD_TLS nvrtcResult cuda_rtc_errno;
TRUSIMD_TLS CUresult cuda_cu_errno;
TRUSIMD_TLS char cuda_build_log[256];

// ----------------------------------------------------------------------------

static inline int cuda_poll(std::vector<trusimd_hardware> *v) {
  int nb_dev;
  cuda_error_type = CUDART_ERROR;
  cuda_errno = cudaGetDeviceCount(&nb_dev);
  if (cuda_errno == cudaErrorNoDevice) {
    return 0;
  } else if (cuda_errno != cudaSuccess) {
    trusimd_errno = TRUSIMD_ECUDA;
    return -1;
  }
  for (int i = 0; i < nb_dev; i++) {
    struct cudaDeviceProp prop;
    cuda_errno = cudaGetDeviceProperties(&prop, i);
    if (cuda_errno != cudaSuccess) {
      trusimd_errno = TRUSIMD_ECUDA;
      return -1;
    }
    trusimd_hardware h;
    memcpy((void *)h.id, (void *)&i, sizeof(int));
    h.accelerator = TRUSIMD_CUDA;
    std::string buf("CUDA ");
    buf += prop.name;
    my_strlcpy(h.description, buf.c_str(), sizeof(h.description));
    v->push_back(h);
  }
  return 0;
}

// ----------------------------------------------------------------------------

static inline const char *cuda_strerror(void) {
  if (cuda_error_type == CU_ERROR) {
    switch (cuda_cu_errno) {
    case CUDA_SUCCESS:
      return "CUDA_SUCCESS";
    case CUDA_ERROR_INVALID_VALUE:
      return "CUDA_ERROR_INVALID_VALUE";
    case CUDA_ERROR_OUT_OF_MEMORY:
      return "CUDA_ERROR_OUT_OF_MEMORY";
    case CUDA_ERROR_NOT_INITIALIZED:
      return "CUDA_ERROR_NOT_INITIALIZED";
    case CUDA_ERROR_DEINITIALIZED:
      return "CUDA_ERROR_DEINITIALIZED";
    case CUDA_ERROR_PROFILER_DISABLED:
      return "CUDA_ERROR_PROFILER_DISABLED";
    case CUDA_ERROR_PROFILER_NOT_INITIALIZED:
      return "CUDA_ERROR_PROFILER_NOT_INITIALIZED";
    case CUDA_ERROR_PROFILER_ALREADY_STARTED:
      return "CUDA_ERROR_PROFILER_ALREADY_STARTED";
    case CUDA_ERROR_PROFILER_ALREADY_STOPPED:
      return "CUDA_ERROR_PROFILER_ALREADY_STOPPED";
#ifdef CUDA_ERROR_STUB_LIBRARY
    case CUDA_ERROR_STUB_LIBRARY:
      return "CUDA_ERROR_STUB_LIBRARY";
#endif
    case CUDA_ERROR_NO_DEVICE:
      return "CUDA_ERROR_NO_DEVICE";
    case CUDA_ERROR_INVALID_DEVICE:
      return "CUDA_ERROR_INVALID_DEVICE";
#ifdef CUDA_ERROR_DEVICE_NOT_LICENSED
    case CUDA_ERROR_DEVICE_NOT_LICENSED:
      return "CUDA_ERROR_DEVICE_NOT_LICENSED";
#endif
    case CUDA_ERROR_INVALID_IMAGE:
      return "CUDA_ERROR_INVALID_IMAGE";
    case CUDA_ERROR_INVALID_CONTEXT:
      return "CUDA_ERROR_INVALID_CONTEXT";
    case CUDA_ERROR_CONTEXT_ALREADY_CURRENT:
      return "CUDA_ERROR_CONTEXT_ALREADY_CURRENT";
    case CUDA_ERROR_MAP_FAILED:
      return "CUDA_ERROR_MAP_FAILED";
    case CUDA_ERROR_UNMAP_FAILED:
      return "CUDA_ERROR_UNMAP_FAILED";
    case CUDA_ERROR_ARRAY_IS_MAPPED:
      return "CUDA_ERROR_ARRAY_IS_MAPPED";
    case CUDA_ERROR_ALREADY_MAPPED:
      return "CUDA_ERROR_ALREADY_MAPPED";
    case CUDA_ERROR_NO_BINARY_FOR_GPU:
      return "CUDA_ERROR_NO_BINARY_FOR_GPU";
    case CUDA_ERROR_ALREADY_ACQUIRED:
      return "CUDA_ERROR_ALREADY_ACQUIRED";
    case CUDA_ERROR_NOT_MAPPED:
      return "CUDA_ERROR_NOT_MAPPED";
    case CUDA_ERROR_NOT_MAPPED_AS_ARRAY:
      return "CUDA_ERROR_NOT_MAPPED_AS_ARRAY";
    case CUDA_ERROR_NOT_MAPPED_AS_POINTER:
      return "CUDA_ERROR_NOT_MAPPED_AS_POINTER";
    case CUDA_ERROR_ECC_UNCORRECTABLE:
      return "CUDA_ERROR_ECC_UNCORRECTABLE";
    case CUDA_ERROR_UNSUPPORTED_LIMIT:
      return "CUDA_ERROR_UNSUPPORTED_LIMIT";
    case CUDA_ERROR_CONTEXT_ALREADY_IN_USE:
      return "CUDA_ERROR_CONTEXT_ALREADY_IN_USE";
    case CUDA_ERROR_PEER_ACCESS_UNSUPPORTED:
      return "CUDA_ERROR_PEER_ACCESS_UNSUPPORTED";
    case CUDA_ERROR_INVALID_PTX:
      return "CUDA_ERROR_INVALID_PTX";
    case CUDA_ERROR_INVALID_GRAPHICS_CONTEXT:
      return "CUDA_ERROR_INVALID_GRAPHICS_CONTEXT";
    case CUDA_ERROR_NVLINK_UNCORRECTABLE:
      return "CUDA_ERROR_NVLINK_UNCORRECTABLE";
    case CUDA_ERROR_JIT_COMPILER_NOT_FOUND:
      return "CUDA_ERROR_JIT_COMPILER_NOT_FOUND";
#ifdef CUDA_ERROR_UNSUPPORTED_PTX_VERSION
    case CUDA_ERROR_UNSUPPORTED_PTX_VERSION:
      return "CUDA_ERROR_UNSUPPORTED_PTX_VERSION";
#endif
#ifdef CUDA_ERROR_JIT_COMPILATION_DISABLED
    case CUDA_ERROR_JIT_COMPILATION_DISABLED:
      return "CUDA_ERROR_JIT_COMPILATION_DISABLED";
#endif
#ifdef CUDA_ERROR_UNSUPPORTED_EXEC_AFFINITY
    case CUDA_ERROR_UNSUPPORTED_EXEC_AFFINITY:
      return "CUDA_ERROR_UNSUPPORTED_EXEC_AFFINITY";
#endif
    case CUDA_ERROR_INVALID_SOURCE:
      return "CUDA_ERROR_INVALID_SOURCE";
    case CUDA_ERROR_FILE_NOT_FOUND:
      return "CUDA_ERROR_FILE_NOT_FOUND";
    case CUDA_ERROR_SHARED_OBJECT_SYMBOL_NOT_FOUND:
      return "CUDA_ERROR_SHARED_OBJECT_SYMBOL_NOT_FOUND";
    case CUDA_ERROR_SHARED_OBJECT_INIT_FAILED:
      return "CUDA_ERROR_SHARED_OBJECT_INIT_FAILED";
    case CUDA_ERROR_OPERATING_SYSTEM:
      return "CUDA_ERROR_OPERATING_SYSTEM";
    case CUDA_ERROR_INVALID_HANDLE:
      return "CUDA_ERROR_INVALID_HANDLE";
    case CUDA_ERROR_ILLEGAL_STATE:
      return "CUDA_ERROR_ILLEGAL_STATE";
    case CUDA_ERROR_NOT_FOUND:
      return "CUDA_ERROR_NOT_FOUND";
    case CUDA_ERROR_NOT_READY:
      return "CUDA_ERROR_NOT_READY";
    case CUDA_ERROR_ILLEGAL_ADDRESS:
      return "CUDA_ERROR_ILLEGAL_ADDRESS";
    case CUDA_ERROR_LAUNCH_OUT_OF_RESOURCES:
      return "CUDA_ERROR_LAUNCH_OUT_OF_RESOURCES";
    case CUDA_ERROR_LAUNCH_TIMEOUT:
      return "CUDA_ERROR_LAUNCH_TIMEOUT";
    case CUDA_ERROR_LAUNCH_INCOMPATIBLE_TEXTURING:
      return "CUDA_ERROR_LAUNCH_INCOMPATIBLE_TEXTURING";
    case CUDA_ERROR_PEER_ACCESS_ALREADY_ENABLED:
      return "CUDA_ERROR_PEER_ACCESS_ALREADY_ENABLED";
    case CUDA_ERROR_PEER_ACCESS_NOT_ENABLED:
      return "CUDA_ERROR_PEER_ACCESS_NOT_ENABLED";
    case CUDA_ERROR_PRIMARY_CONTEXT_ACTIVE:
      return "CUDA_ERROR_PRIMARY_CONTEXT_ACTIVE";
    case CUDA_ERROR_CONTEXT_IS_DESTROYED:
      return "CUDA_ERROR_CONTEXT_IS_DESTROYED";
    case CUDA_ERROR_ASSERT:
      return "CUDA_ERROR_ASSERT";
    case CUDA_ERROR_TOO_MANY_PEERS:
      return "CUDA_ERROR_TOO_MANY_PEERS";
    case CUDA_ERROR_HOST_MEMORY_ALREADY_REGISTERED:
      return "CUDA_ERROR_HOST_MEMORY_ALREADY_REGISTERED";
    case CUDA_ERROR_HOST_MEMORY_NOT_REGISTERED:
      return "CUDA_ERROR_HOST_MEMORY_NOT_REGISTERED";
    case CUDA_ERROR_HARDWARE_STACK_ERROR:
      return "CUDA_ERROR_HARDWARE_STACK_ERROR";
    case CUDA_ERROR_ILLEGAL_INSTRUCTION:
      return "CUDA_ERROR_ILLEGAL_INSTRUCTION";
    case CUDA_ERROR_MISALIGNED_ADDRESS:
      return "CUDA_ERROR_MISALIGNED_ADDRESS";
    case CUDA_ERROR_INVALID_ADDRESS_SPACE:
      return "CUDA_ERROR_INVALID_ADDRESS_SPACE";
    case CUDA_ERROR_INVALID_PC:
      return "CUDA_ERROR_INVALID_PC";
    case CUDA_ERROR_LAUNCH_FAILED:
      return "CUDA_ERROR_LAUNCH_FAILED";
    case CUDA_ERROR_COOPERATIVE_LAUNCH_TOO_LARGE:
      return "CUDA_ERROR_COOPERATIVE_LAUNCH_TOO_LARGE";
    case CUDA_ERROR_NOT_PERMITTED:
      return "CUDA_ERROR_NOT_PERMITTED";
    case CUDA_ERROR_NOT_SUPPORTED:
      return "CUDA_ERROR_NOT_SUPPORTED";
    case CUDA_ERROR_SYSTEM_NOT_READY:
      return "CUDA_ERROR_SYSTEM_NOT_READY";
    case CUDA_ERROR_SYSTEM_DRIVER_MISMATCH:
      return "CUDA_ERROR_SYSTEM_DRIVER_MISMATCH";
    case CUDA_ERROR_COMPAT_NOT_SUPPORTED_ON_DEVICE:
      return "CUDA_ERROR_COMPAT_NOT_SUPPORTED_ON_DEVICE";
#ifdef CUDA_ERROR_MPS_CONNECTION_FAILED
    case CUDA_ERROR_MPS_CONNECTION_FAILED:
      return "CUDA_ERROR_MPS_CONNECTION_FAILED";
#endif
#ifdef CUDA_ERROR_MPS_RPC_FAILURE
    case CUDA_ERROR_MPS_RPC_FAILURE:
      return "CUDA_ERROR_MPS_RPC_FAILURE";
#endif
#ifdef CUDA_ERROR_MPS_SERVER_NOT_READY
    case CUDA_ERROR_MPS_SERVER_NOT_READY:
      return "CUDA_ERROR_MPS_SERVER_NOT_READY";
#endif
#ifdef CUDA_ERROR_MPS_MAX_CLIENTS_REACHED
    case CUDA_ERROR_MPS_MAX_CLIENTS_REACHED:
      return "CUDA_ERROR_MPS_MAX_CLIENTS_REACHED";
#endif
#ifdef CUDA_ERROR_MPS_MAX_CONNECTIONS_REACHED
    case CUDA_ERROR_MPS_MAX_CONNECTIONS_REACHED:
      return "CUDA_ERROR_MPS_MAX_CONNECTIONS_REACHED";
#endif
    case CUDA_ERROR_STREAM_CAPTURE_UNSUPPORTED:
      return "CUDA_ERROR_STREAM_CAPTURE_UNSUPPORTED";
    case CUDA_ERROR_STREAM_CAPTURE_INVALIDATED:
      return "CUDA_ERROR_STREAM_CAPTURE_INVALIDATED";
    case CUDA_ERROR_STREAM_CAPTURE_MERGE:
      return "CUDA_ERROR_STREAM_CAPTURE_MERGE";
    case CUDA_ERROR_STREAM_CAPTURE_UNMATCHED:
      return "CUDA_ERROR_STREAM_CAPTURE_UNMATCHED";
    case CUDA_ERROR_STREAM_CAPTURE_UNJOINED:
      return "CUDA_ERROR_STREAM_CAPTURE_UNJOINED";
    case CUDA_ERROR_STREAM_CAPTURE_ISOLATION:
      return "CUDA_ERROR_STREAM_CAPTURE_ISOLATION";
    case CUDA_ERROR_STREAM_CAPTURE_IMPLICIT:
      return "CUDA_ERROR_STREAM_CAPTURE_IMPLICIT";
    case CUDA_ERROR_CAPTURED_EVENT:
      return "CUDA_ERROR_CAPTURED_EVENT";
    case CUDA_ERROR_STREAM_CAPTURE_WRONG_THREAD:
      return "CUDA_ERROR_STREAM_CAPTURE_WRONG_THREAD";
    case CUDA_ERROR_TIMEOUT:
      return "CUDA_ERROR_TIMEOUT";
    case CUDA_ERROR_GRAPH_EXEC_UPDATE_FAILURE:
      return "CUDA_ERROR_GRAPH_EXEC_UPDATE_FAILURE";
#ifdef CUDA_ERROR_EXTERNAL_DEVICE
    case CUDA_ERROR_EXTERNAL_DEVICE:
      return "CUDA_ERROR_EXTERNAL_DEVICE";
#endif
    case CUDA_ERROR_UNKNOWN:
      return "CUDA_ERROR_UNKNOWN";
    default: return "Unknown CU error";
    }
  } else if (cuda_error_type == NVRTC_ERROR) {
    return nvrtcGetErrorString(cuda_rtc_errno);
  }
  return cudaGetErrorString(cuda_errno);
}

// ----------------------------------------------------------------------------

static inline int cuda_retrieve_defaults(cudaStream_t *s,
                                         trusimd_hardware *h_) {
  cuda_error_type = CUDART_ERROR;
  trusimd_hardware &h = *h_;
  void *tmp;
  memcpy((void *)&tmp, (void *)h.param1, sizeof(void *));
  if (tmp == NULL) {
    cudaStream_t stream;
    cuda_errno = cudaStreamCreate(&stream);
    if (cuda_errno != cudaSuccess) {
      return -1;
    }
    memcpy((void *)h.param1, (void *)&stream, sizeof(cudaStream_t));
  }
  memcpy((void *)s, (void *)h.param1, sizeof(cudaStream_t));
  return 0;
}

// ----------------------------------------------------------------------------

static inline void *cuda_device_malloc(trusimd_hardware *h_, size_t n) {
  cuda_error_type = CUDART_ERROR;
  void *res;
#if CUDART_VERSION >= 11200
  trusimd_hardware &h = *h_;
  cudaStream_t s;
  if (cuda_retrieve_defaults(&s, &h) == -1) {
    return NULL;
  }
  cuda_errno = cudaMallocAsync(&res, n, s);
#else
  (void)h_;
  cuda_errno = cudaMalloc(&res, n);
#endif
  if (cuda_errno != cudaSuccess) {
    res = NULL;
    trusimd_errno = TRUSIMD_ECUDA;
  }
  return res;
}

// ----------------------------------------------------------------------------

static inline void cuda_device_free(trusimd_hardware *h_, void *ptr) {
  cuda_error_type = CUDART_ERROR;
#if CUDART_VERSION >= 11200
  trusimd_hardware &h = *h_;
  cudaStream_t s;
  if (cuda_retrieve_defaults(&s, &h) == -1) {
    return;
  }
  cuda_errno = cudaFreeAsync(ptr, s);
#else
  (void)h_;
  cuda_errno = cudaFree(ptr);
#endif
  if (cuda_errno != cudaSuccess) {
    trusimd_errno = TRUSIMD_ECUDA;
  }
}

// ----------------------------------------------------------------------------

static inline int cuda_copy_to_device(trusimd_hardware *h_, void *dst,
                                      void *src, size_t n) {
  cuda_error_type = CUDART_ERROR;
  trusimd_hardware &h = *h_;
  cudaStream_t s;
  if (cuda_retrieve_defaults(&s, &h) == -1) {
    return -1;
  }
  cuda_errno = cudaMemcpyAsync(dst, src, n, cudaMemcpyHostToDevice, s);
  if (cuda_errno != cudaSuccess) {
    trusimd_errno = TRUSIMD_ECUDA;
    return -1;
  }
  return 0;
}

// ----------------------------------------------------------------------------

static inline int cuda_copy_to_host(trusimd_hardware *h_, void *dst, void *src,
                                    size_t n) {
  cuda_error_type = CUDART_ERROR;
  trusimd_hardware &h = *h_;
  cudaStream_t s;
  if (cuda_retrieve_defaults(&s, &h) == -1) {
    return -1;
  }
  cuda_errno = cudaMemcpyAsync(dst, src, n, cudaMemcpyDeviceToHost, s);
  if (cuda_errno != cudaSuccess) {
    trusimd_errno = TRUSIMD_ECUDA;
    return -1;
  }
  return 0;
}

// ----------------------------------------------------------------------------

static inline int cuda_compile_run(trusimd_hardware *h_, trusimd_kernel *k,
                                   int n, va_list ap) {
  cuda_error_type = CUDART_ERROR;
  trusimd_hardware &h = *h_;
  cudaStream_t s;
  if (cuda_retrieve_defaults(&s, &h) == -1) {
    return -1;
  }

  // Create program
  nvrtcProgram prog;
  cuda_rtc_errno =
      nvrtcCreateProgram(&prog, k->cuda_code.c_str(), NULL, 0, NULL, NULL);
  if (cuda_rtc_errno != NVRTC_SUCCESS) {
    cuda_error_type = CUDART_ERROR;
    trusimd_errno = TRUSIMD_ECUDA;
    return -1;
  }

  // Compile program
  cuda_rtc_errno = nvrtcCompileProgram(prog, 0, NULL);
  if (cuda_rtc_errno != NVRTC_SUCCESS) {
    // Get compilation logs
    cuda_error_type = CUDART_ERROR;
    trusimd_errno = TRUSIMD_ECUDA;
    size_t size;
    cuda_rtc_errno = nvrtcGetProgramLogSize(prog, &size);
    if (cuda_rtc_errno != NVRTC_SUCCESS) {
      nvrtcDestroyProgram(&prog);
      return -1;
    }
    std::vector<char> buf(size + 1);
    cuda_rtc_errno = nvrtcGetProgramLog(prog, &buf[0]);
    if (cuda_rtc_errno != NVRTC_SUCCESS) {
      nvrtcDestroyProgram(&prog);
      return -1;
    }
    my_strlcpy(cuda_build_log, &buf[0], sizeof(cuda_build_log));
    nvrtcDestroyProgram(&prog);
    return -1;
  }

  // Get the PTX
  size_t size;
  cuda_rtc_errno = nvrtcGetPTXSize(prog, &size);
  if (cuda_rtc_errno != NVRTC_SUCCESS) {
    nvrtcDestroyProgram(&prog);
    return -1;
  }
  std::vector<char> ptx(size + 1);
  cuda_rtc_errno = nvrtcGetPTX(prog, &ptx[0]);
  if (cuda_rtc_errno != NVRTC_SUCCESS) {
    nvrtcDestroyProgram(&prog);
    return -1;
  }
  if ((cuda_rtc_errno = nvrtcDestroyProgram(&prog)) != NVRTC_SUCCESS) {
    return -1;
  }

  // Get Stream associated to the device and set current context
  cuda_error_type = CU_ERROR;
  CUcontext context, current_context;
  if ((cuda_cu_errno = cuStreamGetCtx(s, &context)) != CUDA_SUCCESS) {
    return -1;
  }
  if ((cuda_cu_errno = cuCtxGetCurrent(&current_context)) != CUDA_SUCCESS) {
    return -1;
  }
  if (current_context != context) {
    if ((cuda_cu_errno = cuCtxSetCurrent(context)) != CUDA_SUCCESS) {
      return -1;
    }
  }

  // Prepare and launch kernel
  CUmodule module;
  cuda_cu_errno = cuModuleLoadData(&module, (void *)&ptx[0]);
  if (cuda_cu_errno != CUDA_SUCCESS) {
    return -1;
  }
  CUfunction kernel;
  cuda_cu_errno = cuModuleGetFunction(&kernel, module, k->name.c_str());
  if (cuda_cu_errno != CUDA_SUCCESS) {
    return -1;
  }

  size_t nb_args = k->args.size() + 1;
  std::vector<char> args_value(sizeof(void *) * nb_args, 0);
  std::vector<void *> args_ptr(nb_args, NULL);
  for (size_t i = 0; i < nb_args; i++) {
    args_ptr[i] = (void *)&args_value[i * sizeof(void *)];
  }
  memcpy(args_ptr[0], (void *)&n, sizeof(int));
  for (size_t i = 0; i < nb_args - 1; i++) {
    if (is_pointer(k->args[i])) {
      void *ptr = va_arg(ap, void *);
      memcpy(args_ptr[i + 1], (void *)&ptr, sizeof(void *));
    } else {
      switch (k->args[i].width / 8) {
      case 8: {
        unsigned char uc = (unsigned char)va_arg(ap, unsigned int);
        memcpy(args_ptr[i + 1], (void *)&uc, 1);
        break;
      }
      case 16: {
        unsigned short us = (unsigned short)va_arg(ap, unsigned int);
        memcpy(args_ptr[i + 1], (void *)&us, 2);
        break;
      }
      case 32: {
        unsigned int ui = va_arg(ap, unsigned int);
        memcpy(args_ptr[i + 1], (void *)&ui, 4);
        break;
      }
      case 64: {
        unsigned long ul = va_arg(ap, unsigned long);
        memcpy(args_ptr[i + 1], (void *)&ul, 8);
        break;
      }
      }
    }
  }
  if ((cuda_cu_errno = cuLaunchKernel(kernel, (n + 127) / 128, 0, 0, 128, 0, 0,
                                      0, s, &args_ptr[0], NULL)) !=
      CUDA_SUCCESS) {
    cuModuleUnload(module);
    return -1;
  }
  if ((cuda_cu_errno = cuCtxSynchronize()) != CUDA_SUCCESS) {
    cuModuleUnload(module);
    return -1;
  }
  cuModuleUnload(module);
  return 0;
}

// ----------------------------------------------------------------------------

#else
static inline int cuda_poll(std::vector<trusimd_hardware> *) { return 0; }
static inline const char *cuda_strerror(void) { return NULL; }
static inline void *cuda_device_malloc(trusimd_hardware *, size_t) {
  trusimd_errno = TRUSIMD_EAVAIL;
  return NULL;
}
static inline void cuda_device_free(trusimd_hardware *, void *) {}
static inline int cuda_copy_to_host(trusimd_hardware *, void *, void *,
                                       size_t) {
  trusimd_errno = TRUSIMD_EAVAIL;
  return -1;
}
static inline int cuda_copy_to_device(trusimd_hardware *, void *, void *,
                                         size_t) {
  trusimd_errno = TRUSIMD_EAVAIL;
  return -1;
}
static inline int cuda_compile_run(trusimd_hardware *, trusimd_kernel *, int,
                                   va_list) {
  trusimd_errno = TRUSIMD_EAVAIL;
  return -1;
}
#endif

