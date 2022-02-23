#ifdef WITH_OPENCL
#define CL_TARGET_OPENCL_VERSION 120
#if defined (__APPLE__) || defined(MACOSX)
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <iostream>

// ----------------------------------------------------------------------------

TRUSIMD_TLS cl_int opencl_errno;
TRUSIMD_TLS char opencl_build_log[256 + sizeof("CL_BUILD_PROGRAM_FAILURE: ")];

// ----------------------------------------------------------------------------

static inline int opencl_poll(std::vector<trusimd_hardware> *v) {
  cl_uint nb_platforms;
  opencl_errno = clGetPlatformIDs(0, NULL, &nb_platforms);
  if (opencl_errno == -1001 /* CL_PLATFORM_NOT_FOUND_KHR */) {
    return 0;
  } else if (opencl_errno != CL_SUCCESS) {
    trusimd_errno = TRUSIMD_EOPENCL;
    return -1;
  }
  std::vector<cl_platform_id> platforms(nb_platforms);
  opencl_errno = clGetPlatformIDs(nb_platforms, &platforms[0], NULL);
  if (opencl_errno != CL_SUCCESS) {
    trusimd_errno = TRUSIMD_EOPENCL;
    return -1;
  }
  for (size_t p = 0; p < nb_platforms; p++) {
    cl_uint nb_devices;
    opencl_errno =
        clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_ALL, 0, NULL, &nb_devices);
    if (opencl_errno == CL_DEVICE_NOT_FOUND) {
      continue;
    } else if (opencl_errno != CL_SUCCESS) {
      trusimd_errno = TRUSIMD_EOPENCL;
      return -1;
    }
    std::vector<cl_device_id> devices(nb_devices);
    opencl_errno = clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_ALL, nb_devices,
                                  &devices[0], NULL);
    if (opencl_errno != CL_SUCCESS) {
      trusimd_errno = TRUSIMD_EOPENCL;
      return -1;
    }
    for (size_t d = 0; d < nb_devices; d++) {
      size_t length;
      opencl_errno =
             clGetDeviceInfo(devices[d], CL_DEVICE_NAME, 0, NULL, &length);
      if (opencl_errno != CL_SUCCESS) {
        trusimd_errno = TRUSIMD_EOPENCL;
        return -1;
      }
      std::vector<char> buf(length + 1);
      opencl_errno =
          clGetDeviceInfo(devices[d], CL_DEVICE_NAME, length, &buf[0], NULL);
      if (opencl_errno != CL_SUCCESS) {
        trusimd_errno = TRUSIMD_EOPENCL;
        return -1;
      }
      std::string buf2("OpenCL ");
      buf2 += &buf[0];
      trusimd_hardware h;
      memset((void *)h.id, 0, sizeof(h.id));
      memcpy((void *)h.id, (void *)&platforms[p], sizeof(platforms[p]));
      memcpy((void *)(h.id + sizeof(void *)), (void *)&devices[d],
             sizeof(devices[d]));
      memset((void *)h.param1, 0, sizeof(h.param1));
      memset((void *)h.param2, 0, sizeof(h.param2));
      h.accelerator = TRUSIMD_OPENCL;
      my_strlcpy(h.description, buf2.c_str(), sizeof(h.description));
      v->push_back(h);
    }
  }
  return 0;
}

// ----------------------------------------------------------------------------

static inline const char *opencl_strerror(void) {
  // clang-format off
  switch(opencl_errno) {
  case 0: return "CL_SUCCESS";
  case -1: return "CL_DEVICE_NOT_FOUND";
  case -2: return "CL_DEVICE_NOT_AVAILABLE";
  case -3: return "CL_COMPILER_NOT_AVAILABLE";
  case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
  case -5: return "CL_OUT_OF_RESOURCES";
  case -6: return "CL_OUT_OF_HOST_MEMORY";
  case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
  case -8: return "CL_MEM_COPY_OVERLAP";
  case -9: return "CL_IMAGE_FORMAT_MISMATCH";
  case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
  case -11: return opencl_build_log;
  case -12: return "CL_MAP_FAILURE";
  case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
  case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
  case -15: return "CL_COMPILE_PROGRAM_FAILURE";
  case -16: return "CL_LINKER_NOT_AVAILABLE";
  case -17: return "CL_LINK_PROGRAM_FAILURE";
  case -18: return "CL_DEVICE_PARTITION_FAILED";
  case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";
  case -30: return "CL_INVALID_VALUE";
  case -31: return "CL_INVALID_DEVICE_TYPE";
  case -32: return "CL_INVALID_PLATFORM";
  case -33: return "CL_INVALID_DEVICE";
  case -34: return "CL_INVALID_CONTEXT";
  case -35: return "CL_INVALID_QUEUE_PROPERTIES";
  case -36: return "CL_INVALID_COMMAND_QUEUE";
  case -37: return "CL_INVALID_HOST_PTR";
  case -38: return "CL_INVALID_MEM_OBJECT";
  case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
  case -40: return "CL_INVALID_IMAGE_SIZE";
  case -41: return "CL_INVALID_SAMPLER";
  case -42: return "CL_INVALID_BINARY";
  case -43: return "CL_INVALID_BUILD_OPTIONS";
  case -44: return "CL_INVALID_PROGRAM";
  case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
  case -46: return "CL_INVALID_KERNEL_NAME";
  case -47: return "CL_INVALID_KERNEL_DEFINITION";
  case -48: return "CL_INVALID_KERNEL";
  case -49: return "CL_INVALID_ARG_INDEX";
  case -50: return "CL_INVALID_ARG_VALUE";
  case -51: return "CL_INVALID_ARG_SIZE";
  case -52: return "CL_INVALID_KERNEL_ARGS";
  case -53: return "CL_INVALID_WORK_DIMENSION";
  case -54: return "CL_INVALID_WORK_GROUP_SIZE";
  case -55: return "CL_INVALID_WORK_ITEM_SIZE";
  case -56: return "CL_INVALID_GLOBAL_OFFSET";
  case -57: return "CL_INVALID_EVENT_WAIT_LIST";
  case -58: return "CL_INVALID_EVENT";
  case -59: return "CL_INVALID_OPERATION";
  case -60: return "CL_INVALID_GL_OBJECT";
  case -61: return "CL_INVALID_BUFFER_SIZE";
  case -62: return "CL_INVALID_MIP_LEVEL";
  case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
  case -64: return "CL_INVALID_PROPERTY";
  case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
  case -66: return "CL_INVALID_COMPILER_OPTIONS";
  case -67: return "CL_INVALID_LINKER_OPTIONS";
  case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";
  case -69: return "CL_INVALID_PIPE_SIZE";
  case -70: return "CL_INVALID_DEVICE_QUEUE";
  case -71: return "CL_INVALID_SPEC_ID";
  case -72: return "CL_MAX_SIZE_RESTRICTION_EXCEEDED";
  case -1002: return "CL_INVALID_D3D10_DEVICE_KHR";
  case -1003: return "CL_INVALID_D3D10_RESOURCE_KHR";
  case -1004: return "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR";
  case -1005: return "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR";
  case -1006: return "CL_INVALID_D3D11_DEVICE_KHR";
  case -1007: return "CL_INVALID_D3D11_RESOURCE_KHR";
  case -1008: return "CL_D3D11_RESOURCE_ALREADY_ACQUIRED_KHR";
  case -1009: return "CL_D3D11_RESOURCE_NOT_ACQUIRED_KHR";
  case -1010: return "CL_INVALID_DX9_MEDIA_ADAPTER_KHR";
  case -1011: return "CL_INVALID_DX9_MEDIA_SURFACE_KHR";
  case -1012: return "CL_DX9_MEDIA_SURFACE_ALREADY_ACQUIRED_KHR";
  case -1013: return "CL_DX9_MEDIA_SURFACE_NOT_ACQUIRED_KHR";
  case -1093: return "CL_INVALID_EGL_OBJECT_KHR";
  case -1092: return "CL_EGL_RESOURCE_NOT_ACQUIRED_KHR";
  case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
  case -1057: return "CL_DEVICE_PARTITION_FAILED_EXT";
  case -1058: return "CL_INVALID_PARTITION_COUNT_EXT";
  case -1059: return "CL_INVALID_PARTITION_NAME_EXT";
  case -1094: return "CL_INVALID_ACCELERATOR_INTEL";
  case -1095: return "CL_INVALID_ACCELERATOR_TYPE_INTEL";
  case -1096: return "CL_INVALID_ACCELERATOR_DESCRIPTOR_INTEL";
  case -1097: return "CL_ACCELERATOR_TYPE_NOT_SUPPORTED_INTEL";
  case -1000: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
  case -1098: return "CL_INVALID_VA_API_MEDIA_ADAPTER_INTEL";
  case -1099: return "CL_INVALID_VA_API_MEDIA_SURFACE_INTEL";
  case -1100: return "CL_VA_API_MEDIA_SURFACE_ALREADY_ACQUIRED_INTEL";
  case -1101: return "CL_VA_API_MEDIA_SURFACE_NOT_ACQUIRED_INTEL";
  default: return "CL_UNKNOWN_ERROR";
  }
  // clang-format on
}

// ----------------------------------------------------------------------------

static inline int opencl_retrieve_defaults(cl_context *c_,
                                           cl_command_queue *q_,
                                           trusimd_hardware *h_) {
  trusimd_hardware &h = *h_;
  cl_context c;
  cl_command_queue q;

  void *tmp1, *tmp2;
  memcpy((void *)&tmp1, (void *)h.param1, sizeof(void *));
  memcpy((void *)&tmp2, (void *)h.param2, sizeof(void *));
  if (tmp1 == NULL && tmp2 == NULL) {
    // Create context attached to selected platform and device
    cl_platform_id p;
    memcpy((void *)&p, (void *)h.id, sizeof(cl_platform_id));
    cl_context_properties cprops[] = {CL_CONTEXT_PLATFORM,
                                      (cl_context_properties)p,
                                      (cl_context_properties)0};
    cl_device_id d;
    memcpy((void *)&d, (void *)(h.id + sizeof(void *)), sizeof(cl_device_id));
    c = clCreateContext(cprops, 1, &d, NULL, NULL, &opencl_errno);
    if (opencl_errno != CL_SUCCESS) {
      trusimd_errno = TRUSIMD_OPENCL;
      return -1;
    }

    // Create queue attached to context
    q = clCreateCommandQueue(c, d, 0, &opencl_errno);
    if (opencl_errno != CL_SUCCESS) {
      clReleaseContext(c);
      trusimd_errno = TRUSIMD_OPENCL;
      return -1;
    }
    memcpy((void *)h.param1, (void *)&c, sizeof(cl_context_properties));
    memcpy((void *)h.param2, (void *)&q, sizeof(cl_command_queue));
  }

  // Fill arguments that are not NULL
  if (c_) {
    memcpy((void *)c_, (void *)h.param1, sizeof(cl_context_properties));
  }
  if (q_) {
    memcpy((void *)q_, (void *)h.param2, sizeof(cl_command_queue));
  }
  return 0;
}

// ----------------------------------------------------------------------------

static inline void *opencl_device_malloc(trusimd_hardware *h, size_t n) {
  cl_context c;
  if (opencl_retrieve_defaults(&c, NULL, h) == -1) {
    return NULL;
  }
  cl_mem mem = clCreateBuffer(c, CL_MEM_READ_WRITE, n, NULL, &opencl_errno);
  if (opencl_errno != CL_SUCCESS) {
    return NULL;
  }
  void *res = NULL;
  memcpy((void *)&res, (void *)&mem, sizeof(mem));
  return res;
}

// ----------------------------------------------------------------------------

static inline void opencl_device_free(trusimd_hardware *, void *ptr) {
  cl_mem mem;
  memcpy((void *)&mem, (void *)&ptr, sizeof(cl_mem));
  clReleaseMemObject(mem);
}

// ----------------------------------------------------------------------------

static inline int opencl_copy_to_device(trusimd_hardware *h, void *dst_,
                                        void *src, size_t n) {
  cl_mem dst;
  memcpy((void *)&dst, (void *)&dst_, sizeof(cl_mem));
  cl_command_queue q;
  if (opencl_retrieve_defaults(NULL, &q, h) == -1) {
    return -1;
  }
  opencl_errno =
      clEnqueueWriteBuffer(q, dst, CL_TRUE, 0, n, src, 0, NULL, NULL);
  if (opencl_errno != CL_SUCCESS) {
    return -1;
  }
  return 0;
}

// ----------------------------------------------------------------------------

static inline int opencl_copy_to_host(trusimd_hardware *h, void *dst,
                                      void *src_, size_t n) {
  cl_mem src;
  memcpy((void *)&src, (void *)&src_, sizeof(cl_mem));
  cl_command_queue q;
  if (opencl_retrieve_defaults(NULL, &q, h) == -1) {
    return -1;
  }
  opencl_errno =
      clEnqueueReadBuffer(q, src, CL_TRUE, 0, n, dst, 0, NULL, NULL);
  if (opencl_errno != CL_SUCCESS) {
    return -1;
  }
  return 0;
}

// ----------------------------------------------------------------------------

static inline int opencl_compile_run(trusimd_hardware *h, kernel *k, int n,
                                     va_list ap) {
  cl_context c;
  cl_command_queue q;
  if (opencl_retrieve_defaults(&c, &q, h) == -1) {
    return -1;
  }

  // Create program
  const char *source = k->opencl_code.c_str();
  cl_program p = clCreateProgramWithSource(c, 1, &source, NULL, &opencl_errno);
  if (opencl_errno != CL_SUCCESS) {
    trusimd_errno = TRUSIMD_EOPENCL;
    return -1;
  }

  // Build program
  cl_device_id d;
  memcpy((void *)&d, (void *)(h->id + sizeof(void *)), sizeof(cl_device_id));
  opencl_errno = clBuildProgram(p, 1, &d, NULL, NULL, NULL);
  if (opencl_errno != CL_SUCCESS) {
    // Retrieve build error
    size_t size;
    if (clGetProgramBuildInfo(p, d, CL_PROGRAM_BUILD_LOG, 0, NULL, &size) !=
        CL_SUCCESS) {
      strcpy(opencl_build_log, "CL_BUILD_PROGRAM_FAILURE: No log available");
      trusimd_errno = TRUSIMD_EOPENCL;
      return -1;
    }
    std::vector<char> buf(size + 1);
    if (clGetProgramBuildInfo(p, d, CL_PROGRAM_BUILD_LOG, size, &buf[0],
                              NULL) != CL_SUCCESS) {
      strcpy(opencl_build_log, "CL_BUILD_PROGRAM_FAILURE: No log available");
      trusimd_errno = TRUSIMD_EOPENCL;
      return -1;
    }
    strcpy(opencl_build_log, "CL_BUILD_PROGRAM_FAILURE: ");
    my_strlcpy(opencl_build_log + (sizeof("CL_BUILD_PROGRAM_FAILURE: ") - 1),
               &buf[0], 256);
    clReleaseProgram(p);
    trusimd_errno = TRUSIMD_EOPENCL;
    return -1;
  }

  // Create kernel object
  cl_kernel k2 = clCreateKernel(p, k->name.c_str(), &opencl_errno);
  if (opencl_errno != CL_SUCCESS) {
    clReleaseProgram(p);
    trusimd_errno = TRUSIMD_EOPENCL;
    return -1;
  }

  // Set arguments to kernel: first argument is the global work size
  opencl_errno = clSetKernelArg(k2, 0, sizeof(int), (void *)&n);
  if (opencl_errno != CL_SUCCESS) {
    clReleaseKernel(k2);
    clReleaseProgram(p);
    trusimd_errno = TRUSIMD_EOPENCL;
    return -1;
  }
  for (size_t i = 0; i < k->args.size(); i++) {
    char value[sizeof(void *)];
    size_t size;
    if (is_pointer(k->args[i])) {
      size = sizeof(cl_mem);
      void *ptr = va_arg(ap, void *);
      memcpy((void *)value, (void *)&ptr, sizeof(cl_mem));
    } else {
      size = k->args[i].width / 8;
      switch(size) {
      case 8: {
        unsigned char uc = (unsigned char)va_arg(ap, unsigned int);
        memcpy((void *)value, (void *)&uc, 1);
        break;
      }
      case 16: {
        unsigned short us = (unsigned short)va_arg(ap, unsigned int);
        memcpy((void *)value, (void *)&us, 2);
        break;
      }
      case 32: {
        unsigned int ui = va_arg(ap, unsigned int);
        memcpy((void *)value, (void *)&ui, 4);
        break;
      }
      case 64: {
        unsigned long ul = va_arg(ap, unsigned long);
        memcpy((void *)value, (void *)&ul, 8);
        break;
      }
      }
    }
    opencl_errno = clSetKernelArg(k2, cl_uint(i + 1), size, (void *)value);
    if (opencl_errno != CL_SUCCESS) {
      clReleaseKernel(k2);
      clReleaseProgram(p);
      trusimd_errno = TRUSIMD_EOPENCL;
      return -1;
    }
  }

  // Launch kernel
  size_t global_work_size = size_t(n);
  size_t local_work_size = 64;
  opencl_errno = clEnqueueNDRangeKernel(q, k2, 1, NULL, &global_work_size,
                                        &local_work_size, 0, NULL, NULL);
  if (opencl_errno != CL_SUCCESS) {
    clReleaseKernel(k2);
    clReleaseProgram(p);
    trusimd_errno = TRUSIMD_EOPENCL;
    return -1;
  }
  opencl_errno = clFinish(q);
  if (opencl_errno != CL_SUCCESS) {
    clReleaseKernel(k2);
    clReleaseProgram(p);
    trusimd_errno = TRUSIMD_EOPENCL;
    return -1;
  }
  clReleaseKernel(k2);
  clReleaseProgram(p);
  return 0;
}

// ----------------------------------------------------------------------------

#else

static inline int opencl_poll(std::vector<trusimd_hardware> *) { return 0; }
static inline const char *opencl_strerror(void) { return NULL; }
static inline void *opencl_device_malloc(trusimd_hardware *, size_t) {
  trusimd_errno = TRUSIMD_EAVAIL;
  return NULL;
}
static inline void opencl_device_free(trusimd_hardware *, void *) {}
static inline int opencl_copy_to_device(trusimd_hardware *, void *, void *,
                                        size_t) {
  trusimd_errno = TRUSIMD_EAVAIL;
  return -1;
}
static inline int opencl_copy_to_host(trusimd_hardware *, void *, void *,
                                      size_t) {
  trusimd_errno = TRUSIMD_EAVAIL;
  return -1;
}
static inline int opencl_compile_run(trusimd_hardware *, kernel *, int,
                                     va_list) {
  trusimd_errno = TRUSIMD_EAVAIL;
  return -1;
}

#endif

