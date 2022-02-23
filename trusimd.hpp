#ifndef TRUSIMD_HPP
#define TRUSIMD_HPP

// ----------------------------------------------------------------------------

#include <trusimd.h>
#include <cstdarg>
#include <cstdlib>
#include <string>
#include <cstring>
#include <iostream>

#ifndef NO_EXCEPTIONS
#include <stdexcept>
#endif

// ----------------------------------------------------------------------------
// Detect C++ version in use

#if defined(_MSC_VER) && defined(_MSVC_LANG)
  #define TRUSIMD__cplusplus _MSVC_LANG
#else
  #define TRUSIMD__cplusplus __cplusplus
#endif

// ----------------------------------------------------------------------------

namespace trusimd {

// ----------------------------------------------------------------------------
// Exception handling

#ifndef NO_EXCEPTIONS

struct runtime_error : public std::runtime_error {
  int code;
  runtime_error(int code_) : std::runtime_error(""), code(code_) {}
  const char *what() const throw() { return trusimd_strerror(code); }
};

#define TRUSIMD_THROW(error_code) throw trusimd::runtime_error(error_code)

#else

#define TRUSIMD_THROW(error_code)                                             \
  do {                                                                        \
    std::cerr << "ERROR: " << __FILE__ << ": " << __LINE__ << std::endl;      \
    abort();                                                                  \
  } while (0)

#endif

#define TRUSIMD_THROW_IF_ERROR_INT(expr)                                      \
  do {                                                                        \
    if ((expr) == -1) {                                                       \
      TRUSIMD_THROW(trusimd_errno);                                           \
    }                                                                         \
  } while (0)

#define TRUSIMD_THROW_IF_ERROR_PVOID(expr)                                    \
  do {                                                                        \
    if ((expr) == NULL) {                                                     \
      TRUSIMD_THROW(trusimd_errno);                                           \
    }                                                                         \
  } while (0)

// ----------------------------------------------------------------------------
// kernel

TRUSIMD_TLS trusimd_kernel *current_kernel = NULL;

// ----------------------------------------------------------------------------
// Global index type and variable

struct gid_type {
} gid;

// Base types
const trusimd_type int8 = {TRUSIMD_SCALAR, TRUSIMD_SIGNED, 8, 0};
const trusimd_type uint8 = {TRUSIMD_SCALAR, TRUSIMD_UNSIGNED, 8, 0};
const trusimd_type int16 = {TRUSIMD_SCALAR, TRUSIMD_SIGNED, 16, 0};
const trusimd_type uint16 = {TRUSIMD_SCALAR, TRUSIMD_UNSIGNED, 16, 0};
const trusimd_type float16 = {TRUSIMD_SCALAR, TRUSIMD_FLOAT, 16, 0};
const trusimd_type bfloat16 = {TRUSIMD_SCALAR, TRUSIMD_BFLOAT, 16, 0};
const trusimd_type int32 = {TRUSIMD_SCALAR, TRUSIMD_SIGNED, 32, 0};
const trusimd_type uint32 = {TRUSIMD_SCALAR, TRUSIMD_UNSIGNED, 32, 0};
const trusimd_type float32 = {TRUSIMD_SCALAR, TRUSIMD_FLOAT, 32, 0};
const trusimd_type int64 = {TRUSIMD_SCALAR, TRUSIMD_SIGNED, 64, 0};
const trusimd_type uint64 = {TRUSIMD_SCALAR, TRUSIMD_UNSIGNED, 64, 0};
const trusimd_type float64 = {TRUSIMD_SCALAR, TRUSIMD_FLOAT, 64, 0};

// Pointers to base types
const trusimd_type int8ptr = {TRUSIMD_SCALAR, TRUSIMD_SIGNED, 8, 1};
const trusimd_type uint8ptr = {TRUSIMD_SCALAR, TRUSIMD_UNSIGNED, 8, 1};
const trusimd_type int16ptr = {TRUSIMD_SCALAR, TRUSIMD_SIGNED, 16, 1};
const trusimd_type uint16ptr = {TRUSIMD_SCALAR, TRUSIMD_UNSIGNED, 16, 1};
const trusimd_type float16ptr = {TRUSIMD_SCALAR, TRUSIMD_FLOAT, 16, 1};
const trusimd_type bfloat16ptr = {TRUSIMD_SCALAR, TRUSIMD_BFLOAT, 16, 1};
const trusimd_type int32ptr = {TRUSIMD_SCALAR, TRUSIMD_SIGNED, 32, 1};
const trusimd_type uint32ptr = {TRUSIMD_SCALAR, TRUSIMD_UNSIGNED, 32, 1};
const trusimd_type float32ptr = {TRUSIMD_SCALAR, TRUSIMD_FLOAT, 32, 1};
const trusimd_type int64ptr = {TRUSIMD_SCALAR, TRUSIMD_SIGNED, 64, 1};
const trusimd_type uint64ptr = {TRUSIMD_SCALAR, TRUSIMD_UNSIGNED, 64, 1};
const trusimd_type float64ptr = {TRUSIMD_SCALAR, TRUSIMD_FLOAT, 64, 1};

class var {
private:
  int id;
  int index_id;

public:
  var(trusimd_type const &t) : index_id(-1) {
    TRUSIMD_THROW_IF_ERROR_INT(id = trusimd_var(current_kernel, t));
  }

private:
  var() : id(-1), index_id(-1) {}
  var(var const &other) : id(other.id), index_id(other.index_id) {}
  var(int id_) : id(id_), index_id(-1) {}

  int operator()(void) const {
    if (index_id == -1) {
      return id;
    }
    int res;
    TRUSIMD_THROW_IF_ERROR_INT(res =
                                   trusimd_load(current_kernel, id, index_id));
    return res;
  }

  friend inline var arg(int);
  friend inline var get_global_index(void);

public:
  var &operator=(var const &other) {
    if (index_id == -1) {
      TRUSIMD_THROW_IF_ERROR_INT(
          trusimd_assign(current_kernel, (*this)(), other()));
    } else {
      TRUSIMD_THROW_IF_ERROR_INT(
          trusimd_store(current_kernel, id, index_id, other()));
    }
    return *this;
  }

  var operator+(var const &other) {
    var res;
    TRUSIMD_THROW_IF_ERROR_INT(
        res.id = trusimd_add(current_kernel, (*this)(), other()));
    return res;
  }

  var operator[](var const &index) {
    var res;
    res.id = (*this)();
    res.index_id = index();
    return res;
  }

  var operator[](gid_type const &) {
    var res;
    res.id = (*this)();
    TRUSIMD_THROW_IF_ERROR_INT(res.index_id =
                                   trusimd_get_global_id(current_kernel));
    return res;
  }
};

// ----------------------------------------------------------------------------
// Hardware abstraction

typedef trusimd_hardware hardware;

// ----------------------------------------------------------------------------
// Memory buffer abstraction

template <typename T> class buffer_pair {
private:
  void *host_ptr;
  void *dev_ptr;
  size_t n;
  trusimd_hardware h;

public:
  buffer_pair(hardware const &h_, size_t n_) {
    h = h_;
    n = n_ * sizeof(T);
    host_ptr = malloc(n);
    if (host_ptr == NULL) {
      TRUSIMD_THROW(TRUSIMD_ENOMEM);
    }
    TRUSIMD_THROW_IF_ERROR_PVOID(dev_ptr = trusimd_device_malloc(&h, n));
  }

  ~buffer_pair() {
    free(host_ptr);
    trusimd_device_free(&h, dev_ptr);
  }

  T *host() const { return (T *)host_ptr; }

  T *device() const { return (T *)dev_ptr; }

  template <typename IndexType> T &operator[](IndexType i) {
    return ((T *)host_ptr)[i];
  }

  template <typename IndexType> T const &operator[](IndexType i) const {
    return ((T *)host_ptr)[i];
  }

  void copy_to_device() {
    TRUSIMD_THROW_IF_ERROR_INT(
        trusimd_copy_to_device(&h, dev_ptr, host_ptr, n));
  }

  void copy_to_host() {
    TRUSIMD_THROW_IF_ERROR_INT(trusimd_copy_to_host(&h, host_ptr, dev_ptr, n));
  }
};

// ----------------------------------------------------------------------------
// Kernel abstraction

class kernel {
private:
  const char *to_cstr(const char *s) { return s; }
  const char *to_cstr(char *s) { return s; }
  const char *to_cstr(std::string const &s) { return s.c_str(); }
  bool finished;
  trusimd_kernel *k;

public:
#if TRUSIMD__cplusplus >= 201103L
  template <typename StringType, typename... Arg>
  kernel(StringType name, Arg... arg) {
    if (current_kernel != NULL) {
      trusimd_end_kernel(current_kernel);
      finished = true;
      current_kernel = NULL;
    }
    finished = false;
    current_kernel =
        trusimd_create_kernel(to_cstr(name), arg..., trusimd_notype);
    k = current_kernel;
    TRUSIMD_THROW_IF_ERROR_PVOID(k);
  }
#endif

  template <typename StringType> kernel(StringType name, ...) {
    if (current_kernel != NULL) {
      trusimd_end_kernel(current_kernel);
      finished = true;
      current_kernel = NULL;
    }
    finished = false;
    va_list ap;
    va_start(ap, name);
    current_kernel = trusimd_create_kernel_ap(to_cstr(name), ap);
    va_end(ap);
    k = current_kernel;
    TRUSIMD_THROW_IF_ERROR_PVOID(k);
  }

#if TRUSIMD__cplusplus >= 201103L
private:
  template <typename T> T c(T a) { return a; }
  template <typename T> void *c(buffer_pair<T> const &a) { return a.device(); }

public:
  template <typename... Arg>
  void operator()(hardware &h, int n, Arg&... arg) {
    if (!finished) {
      trusimd_end_kernel(k);
      finished = true;
    }
    TRUSIMD_THROW_IF_ERROR_INT(trusimd_compile_run(&h, k, n, c(arg)...));
  }
#endif

  void operator()(hardware &h, int n, ...) {
    if (!finished) {
      trusimd_end_kernel(k);
      finished = true;
    }
    va_list ap;
    va_start(ap, n);
    int code = trusimd_compile_run_ap(&h, k, n, ap);
    va_end(ap);
    TRUSIMD_THROW_IF_ERROR_INT(code);
  }

  ~kernel() {
    trusimd_clear_kernel(k);
    current_kernel = NULL;
  }

  friend std::ostream &operator<<(std::ostream &, kernel &);
};

std::ostream &operator<<(std::ostream &o, kernel &k) {
  if (!k.finished) {
    trusimd_end_kernel(k.k);
    k.finished = true;
  }
  return o << "============================================\n"
           << "CUDA:\n\n"
           << trusimd_get_cuda(k.k) << "\n\n"
           << "============================================\n"
           << "OpenCL:\n\n"
           << trusimd_get_opencl(k.k) << "\n\n"
           << "============================================\n"
           << "LLVM IR:\n\n"
           << trusimd_get_llvmir(k.k) << "\n\n";
}

// ----------------------------------------------------------------------------

inline var arg(int i) {
  if (i < 0 || i >= trusimd_nb_kernel_args(current_kernel)) {
    TRUSIMD_THROW(TRUSIMD_EINDEX);
  }
  return var(trusimd_get_kernel_arg(current_kernel, i));
}

// ----------------------------------------------------------------------------
// Helper to select hardware

namespace detail {
static inline void tolower(char *dst, const char *src, size_t n) {
  size_t i = 0;
  for (; src[0] && i < n - 1; i++) {
    if (src[i] >= 'A' && src[i] <= 'Z') {
      dst[i] = src[i] - ('A' - 'a');
    } else {
      dst[i] = src[i];
    }
  }
  dst[i] = '\0';
}
} // namespace detail

inline hardware &find_hardware(const char *s_) {
  hardware *h;
  int nh;
  TRUSIMD_THROW_IF_ERROR_INT(nh = trusimd_poll(&h));
  char s[sizeof(h->description)];
  detail::tolower(s, s_, sizeof(h->description));
  char description[sizeof(h->description)];
  for (int i = 0; i < nh; i++) {
    detail::tolower(description, h[i].description, sizeof(h->description));
    if (strstr(description, s)) {
      return h[i];
    }
  }
  TRUSIMD_THROW(TRUSIMD_EAVAIL);
}

inline hardware const &find_hardware(std::string const &s) {
  return find_hardware(s.c_str());
}

// ----------------------------------------------------------------------------

#undef TRUSIMD_THROW_IF_ERROR_INT
#undef TRUSIMD_THROW_IF_ERROR_PVOID

} // namespace trusimd

#endif
