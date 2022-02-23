#include "trusimd.h"

#include <vector>
#include <set>
#include <map>
#include <cerrno>
#include <cstring>
#include <string>
#include <sstream>

#ifndef NO_EXCEPTIONS
#include <exception>
#define THROW(e) throw e
#else
#include <iostream>
#define THROW(e)                                                              \
  do {                                                                        \
    std::cerr << "ERROR: " << __FILE__ << ": " << __LINE__ << std::endl;      \
    abort();                                                                  \
  } while (0)
#endif

// ----------------------------------------------------------------------------
// Helpers/utils

static inline void my_strlcpy(char *dst, const char *src, size_t n) {
  strncpy(dst, src, n - 1);
  dst[n - 1] = '\0';
}

template <typename T> static inline void print_T(std::string *buf_, T v) {
  std::stringstream ss;
  ss << v;
  (*buf_) += ss.str();
}

// ----------------------------------------------------------------------------
// type

typedef trusimd_type type;

static inline bool operator==(type const &a, type const &b) {
  return a.scalar_vector == b.scalar_vector && a.width == b.width &&
         a.kind == b.kind && a.nb_times_ptr == b.nb_times_ptr;
}

static inline bool operator!=(type const &a, type const &b) {
  return !(a == b);
}

static inline bool is_int(type t) {
  return t.kind == TRUSIMD_SIGNED || t.kind == TRUSIMD_UNSIGNED;
}

static inline bool is_signed(type t) { return t.kind == TRUSIMD_SIGNED; }

static inline bool is_bool(type t) { return is_int(t) && t.width == 1; }

static inline bool is_pointer(type t) { return t.nb_times_ptr > 0; }

static inline type remove_pointer(type t) {
  type res = t;
  res.nb_times_ptr = 0;
  return res;
}

// ----------------------------------------------------------------------------

struct trusimd_kernel {
  std::string name;

  // LLVM IR
  std::set<int> user_vars;
  std::vector<size_t> type_pos;
  std::string llvm_ir_vec, llvm_ir_sca;
  int ir_indentation;

  // CUDA/OpenCL
  std::map<int, std::string> expr;
  std::string cuda_code;
  std::string opencl_code;
  int c_indentation;

  // Common to all
  std::vector<type> vars;
  std::vector<type> args;
  std::vector<int> args_vars;
  int next_var;
  int global_index_var;
};

typedef trusimd_kernel kernel;

// ----------------------------------------------------------------------------
// Backends

#include "backend_opencl.cpp"
#include "backend_cuda.cpp"
#include "backend_llvm.cpp"

// ----------------------------------------------------------------------------
// Get next variable number

static inline int pick_next_var(kernel *k, type t) {
  k->next_var++;
  k->vars.resize(k->next_var + 1);
  k->vars[k->next_var] = t;
  return k->next_var;
}

// ----------------------------------------------------------------------------
// Print LLVM IR type

static inline void print_ir_type(std::string *buf_, type t) {
  std::string &buf = *buf_;
  switch(t.kind) {
  case TRUSIMD_SIGNED:
  case TRUSIMD_UNSIGNED:
    buf += 'i';
    print_T(&buf, t.width);
    break;
  case TRUSIMD_FLOAT:
    if (t.width == 16) {
      buf += "half";
    } else if (t.width == 32) {
      buf += "float";
    } else if (t.width == 64) {
      buf += "double";
    }
    break;
  case TRUSIMD_BFLOAT:
    buf += "bfloat";
    break;
  }
}

static inline void print_irsca_type(std::string *buf_, type t) {
  std::string &buf = *buf_;
  print_ir_type(&buf, t);
  for (int i = 0; i < t.nb_times_ptr; i++) {
    buf.push_back('*');
  }
}

static inline void print_irvec_type(kernel *k, std::string *buf_, type t) {
  std::string &buf = *buf_;
  if (t.scalar_vector == TRUSIMD_SCALAR) {
    print_irsca_type(&buf, t);
    return;
  }
  k->type_pos.push_back(buf.size() + 1);
  buf += "<?????????? x ";
  print_ir_type(&buf, t);
  buf.push_back('>');
  if (t.nb_times_ptr > 0) {
    buf.push_back(' ');
    for (int i = 0; i < t.nb_times_ptr; i++) {
      buf.push_back('*');
    }
  }
}

// ----------------------------------------------------------------------------
// Print CUDA/OpenCL type

static inline void print_c_type(std::string *buf_, type t) {
  std::string &buf = *buf_;
  if (t.width == 1) {
    buf += "bool";
  } else {
    switch(t.kind) {
    case TRUSIMD_SIGNED:
      buf += "signed";
      break;
    case TRUSIMD_UNSIGNED:
      buf += "unsigned";
      break;
    case TRUSIMD_FLOAT:
      if (t.width == 16) {
        buf += "half";
      } else if (t.width == 32) {
        buf += "float";
      } else if (t.width == 64) {
        buf += "double";
      }
      break;
    case TRUSIMD_BFLOAT:
      buf += "bfloat16";
      break;
    }
    if (t.kind == TRUSIMD_SIGNED || t.kind == TRUSIMD_UNSIGNED) {
      buf.push_back(' ');
      if (t.width == 8) {
        buf += "char";
      } else if (t.width == 16) {
        buf += "short";
      } else if (t.width == 32) {
        buf += "int";
      } else if (t.width == 64) {
        buf += "long";
      }
    }
  }
  for (int i = 0; i < t.nb_times_ptr; i++) {
    buf.push_back('*');
  }
}

static inline void print_cuda_type(std::string *buf_, type t) {
  print_c_type(buf_, t);
}

static inline void print_opencl_type(std::string *buf_, type t) {
  print_c_type(buf_, t);
}

// ----------------------------------------------------------------------------
// Print helper

enum PrintLang { IRVec, IRSca, CU, CL };

static inline void print(PrintLang lang, kernel *k, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  bool has_exceptionned = false;
  std::exception e;
#ifndef NO_EXCEPTIONS
  try {
#endif
    std::string *buf_;
    int indentation;
    switch (lang) {
    case IRVec:
      buf_ = &k->llvm_ir_vec;
      indentation = k->ir_indentation;
      break;
    case IRSca:
      buf_ = &k->llvm_ir_sca;
      indentation = k->ir_indentation;
      break;
    case CU:
      buf_ = &k->cuda_code;
      indentation = k->c_indentation;
      break;
    case CL:
      buf_ = &k->opencl_code;
      indentation = k->c_indentation;
      break;
    }
    std::string &buf = *buf_;
    for (const char *s = fmt; s[0]; s++) {
      switch (s[0]) {
      case '\\':
        if (s[1] != 0) {
          buf.push_back(s[1]);
          s++;
        }
        break;
      case '|': {
        for (int i = 0; i < indentation; i++) {
          buf.push_back(' ');
        }
        break;
      }
      case 'S': {
        const char *t = va_arg(ap, const char *);
        buf += t;
        break;
      }
      case 'D': {
        int d = va_arg(ap, int);
        print_T(&buf, d);
        break;
      }
      case 'V': {
        if (lang == IRVec || lang == IRSca) {
          buf += "%v";
        } else {
          buf.push_back('v');
        }
        int var_num = va_arg(ap, int);
        print_T(&buf, var_num);
        break;
      }
      case 'T': {
        type t = va_arg(ap, type);
        switch (lang) {
        case IRVec:
          print_irvec_type(k, &buf, t);
          break;
        case IRSca:
          print_irsca_type(&buf, t);
          break;
        case CU:
          print_cuda_type(&buf, t);
          break;
        case CL:
          print_opencl_type(&buf, t);
          break;
        }
        break;
      }
      default:
        buf.push_back(s[0]);
        break;
      }
    }
#ifndef NO_EXCEPTIONS
  } catch (std::exception &e_) {
    e = e_;
    has_exceptionned = true;
  }
#endif
  va_end(ap);
  if (has_exceptionned) {
    THROW(e);
  }
}

// ----------------------------------------------------------------------------
// LLVM IR helper to retrieve a variable

static inline int need_ir_var(kernel *k, int var_num) {
  std::set<int> &s = k->user_vars;
  std::set<int>::const_iterator it = s.find(var_num);
  if (it == s.end()) {
    return var_num;
  }
  type t = k->vars[var_num];
  int nv = pick_next_var(k, t);
  print(IRVec, k, "|V = load T, T* V\n", nv, t, t, var_num);
  print(IRSca, k, "|V = load T, T* V\n", nv, t, t, var_num);
  return nv;
}

// ----------------------------------------------------------------------------
// Helper for binary operators

enum BinOp {
  Add,
  Sub,
  Mul,
  Div,
  Rem,
  Shl,
  Shr,
  Shra,
  Xor,
  And,
  Or,
  AndNot
};

static inline int trusimd_binop(kernel *k, BinOp bin_op, int left, int right) {
  type lt = k->vars[left];
  type rt = k->vars[right];

  // type checking
  switch(bin_op) {
  case Add:
  case Sub:
  case Mul:
  case Div:
    if (lt != rt) {
      THROW(TRUSIMD_ETYPE);
    }
    break;
  case Rem:
  case Xor:
  case And:
  case AndNot:
  case Or:
    if (lt != rt || !is_int(lt)) {
      THROW(TRUSIMD_ETYPE);
    }
    break;
  case Shl:
  case Shr:
  case Shra:
    if (!is_int(lt) || !is_int(rt)) {
      THROW(TRUSIMD_ETYPE);
    }
    break;
  }

  // operator selection
  const char *llvm_ir_op;
  const char *c_op;
  switch(bin_op) {
  case Add:
    c_op = " + ";
    if (is_int(lt)) {
      llvm_ir_op = "add";
    } else {
      llvm_ir_op = "fadd";
    }
    break;
  case Sub:
    c_op = " - ";
    if (is_int(lt)) {
      llvm_ir_op = "sub";
    } else {
      llvm_ir_op = "fsub";
    }
    break;
  case Mul:
    c_op = " * ";
    if (is_int(lt)) {
      llvm_ir_op = "mul";
    } else {
      llvm_ir_op = "fmul";
    }
    break;
  case Div:
    c_op = " / ";
    if (is_int(lt)) {
      llvm_ir_op = (is_signed(lt) ? "sdiv" : "udiv");
    } else {
      llvm_ir_op = "fdiv";
    }
    break;
  case Rem:
    c_op = " % ";
    llvm_ir_op = (is_signed(lt) ? "srem" : "urem");
    break;
  case Xor:
    c_op = " ^ ";
    llvm_ir_op = "xor";
    break;
  case And:
    c_op = (is_bool(lt) ? " && " : " & ");
    llvm_ir_op = "and";
    break;
  case AndNot:
    c_op = (is_bool(lt) ? " && " : " & ");
    llvm_ir_op = "andnot";
    break;
  case Or:
    c_op = (is_bool(lt) ? " || " : " | ");
    llvm_ir_op = "or";
    break;
  case Shl:
    c_op = " << ";
    llvm_ir_op = "shl";
    break;
  case Shr:
    c_op = " >> ";
    llvm_ir_op = "lshr";
    break;
  case Shra:
    c_op = " >> ";
    llvm_ir_op = "ashr";
    break;
  }

  // LLVM IR
  int vl = need_ir_var(k, left);
  int vr = need_ir_var(k, right);
  int nv = pick_next_var(k, lt);
  print(IRVec, k, "|V = S T V, V\n\n", nv, llvm_ir_op, lt, vl, vr);
  print(IRSca, k, "|V = S T V, V\n\n", nv, llvm_ir_op, lt, vl, vr);

  // C code
  k->expr[nv] = "(" + k->expr[left] + c_op + k->expr[right] + ")";

  return nv;
}

// ============================================================================
//
// FROM HERE ONLY EXPORTED FUNCTION
//
// ============================================================================

extern "C" {

// ----------------------------------------------------------------------------
// Library errno

TRUSIMD_TLS int trusimd_errno = 0;

// ----------------------------------------------------------------------------
// Hardware polling

static std::vector<trusimd_hardware> hardwares;

int trusimd_poll(trusimd_hardware **h) {
  hardwares.clear();
  if (llvm_poll(&hardwares)) {
    return -1;
  }
  if (opencl_poll(&hardwares)) {
    return -1;
  }
  if (cuda_poll(&hardwares)) {
    return -1;
  }
  *h = &hardwares[0];
  return int(hardwares.size());
}

// ----------------------------------------------------------------------------
// Kernel creation

kernel *trusimd_create_kernel_ap(const char *name, va_list ap) {
  kernel *res;
#ifndef NO_EXCEPTIONS
  try {
#endif
    res = new kernel;
    res->name = std::string(name);
    res->next_var = -1;
    res->c_indentation = 0;
    res->ir_indentation = 0;
    print(IRVec, res, "define void @S(i64 %size", name);
    print(IRSca, res, "define void @S(i64 %i, i64 %size, i8* %args) {\n\n",
          name);
    res->ir_indentation = 2;
    print(CU, res, "__kernel__ void S(int size", name);
    print(CL, res, "__kernel void S(int size", name);
    for (int arg_i = 0;; arg_i++) {
      type t = va_arg(ap, type);
      if (t == trusimd_notype) {
        break;
      }
      res->args.push_back(t);
      int argptr_i8 =
          pick_next_var(res, {TRUSIMD_SCALAR, TRUSIMD_SIGNED, 8, 0});
      int argptr_T = pick_next_var(res, t);
      int nv = pick_next_var(res, t);
      res->expr[nv] = "v";
      res->args_vars.push_back(nv);
      print_T(&(res->expr[nv]), nv);
      print(IRVec, res, ", T V", t, nv);
      print(IRSca, res,
            "|V = getelementptr inbounds i8, i8* %args, i64 D\n"
            "|V = bitcast i8* V to T*\n"
            "|V = load T, T* V\n\n",
            argptr_i8, 8 * arg_i, argptr_T, argptr_i8, t, nv, t, t, argptr_T);
      print(CU, res, ", T V", t, nv);
      print(CL, res, ", __global T V", t, nv);
    }
    int gid_var = pick_next_var(res, {TRUSIMD_SCALAR, TRUSIMD_SIGNED, 64, 0});
    res->expr[gid_var] = "v";
    print_T(&(res->expr[gid_var]), gid_var);
    res->global_index_var = gid_var;
    print(IRVec, res,
          ") {\n\n"
          "  %global_index_ptr = alloca i64\n"
          "  store i64 0, i64* %global_index_ptr\n"
          "  br label %for_cond\n\n"
          "for_cond:\n\n"
          "  V = load i64, i64* %global_index_ptr\n"
          "  %ipn = add i64 V, ??????????\n"
          "  %b = icmp sgt i64 %ipn, %size\n"
          "  br i1 %b, label %for_exit, label %for_body\n\n"
          "for_body:\n\n",
          gid_var, gid_var);
    print(IRSca, res,
          "  %global_index_ptr = alloca i64\n"
          "  store i64 %i, i64* %global_index_ptr\n"
          "  br label %for_cond\n\n"
          "for_cond:\n\n"
          "  V = load i64, i64* %global_index_ptr\n"
          "  %b = icmp sge i64 V, %size\n"
          "  br i1 %b, label %for_exit, label %for_body\n\n"
          "for_body:\n\n",
          gid_var, gid_var);
    // convenient but not opttimized
    res->type_pos.push_back(res->llvm_ir_vec.find("??????????"));
    print(
        CU, res,
        ") {\n\n"
        "  int V = (int)(blockDim.x * blockIdx.x + threadIdx.x);\n"
        "  if (V >= size) {\n"
        "    return;\n"
        "  }\n\n",
        gid_var, gid_var);
    print(CL, res,
        ") {\n\n"
        "  int V = (int)get_global_id(0);\n"
        "  if (V >= size) {\n"
        "    return;\n"
        "  }\n\n", res->global_index_var, res->global_index_var);
    res->ir_indentation = 2;
    res->c_indentation = 2;
#ifndef NO_EXCEPTIONS
  } catch(std::exception &e) {
    trusimd_errno = TRUSIMD_ENOMEM;
    res = NULL;
  }
#endif
  return res;
}

kernel *trusimd_create_kernel(const char *name, ...) {
  va_list ap;
  va_start(ap, name);
  kernel *res = trusimd_create_kernel_ap(name, ap);
  va_end(ap);
  return res;
}

void trusimd_end_kernel(kernel *k) {
  k->c_indentation = 0;
  k->ir_indentation = 0;
  print(IRVec, k,
        "  store i64 %ipn, i64* %global_index_ptr\n"
        "  br label %for_cond\n\n"
        "for_exit:\n\n"
        "  ret void\n\n"
        "}\n");
  print(IRSca, k,
        "  %ip1 = add nsw i64 V, 1\n"
        "  store i64 %ip1, i64* %global_index_ptr\n"
        "  br label %for_cond\n\n"
        "for_exit:\n\n"
        "  ret void\n\n"
        "}\n",
        k->global_index_var);
  print(CU, k, "}\n");
  print(CL, k, "}\n");
}

void trusimd_clear_kernel(kernel *k) { delete k; }

int trusimd_nb_kernel_args(kernel *k) { return int(k->args.size()); }

int trusimd_get_kernel_arg(kernel *k, int i) {
  return k->args_vars[size_t(i)];
}

// ----------------------------------------------------------------------------
// Helpers

const char *trusimd_get_cuda(kernel *k) { return k->cuda_code.c_str(); }
const char *trusimd_get_opencl(kernel *k) { return k->opencl_code.c_str(); }
const char *trusimd_get_llvmir(kernel *k) { return k->llvm_ir_vec.c_str(); }

// ----------------------------------------------------------------------------
// Get error message

const char *trusimd_strerror(int code) {
  switch(code) {
  case TRUSIMD_NOERR:
    return "Success";
  case TRUSIMD_ETYPE:
    return "Wrong type";
  case TRUSIMD_EINDEX:
    return "Index out of range";
  case TRUSIMD_EAVAIL:
    return "Function or implementation not available";
  case TRUSIMD_ELLVM:
    return llvm_strerror();
  case TRUSIMD_ECUDA:
    return cuda_strerror();
  case TRUSIMD_EOPENCL:
    return opencl_strerror();
  default:
    return "Unknown error code";
  }
}

// ----------------------------------------------------------------------------
// Binary operations

int trusimd_add(kernel *k, int left, int right) {
  return trusimd_binop(k, Add, left, right);
}

// ----------------------------------------------------------------------------
// Variable creation

int trusimd_var(kernel *k, type t) {
#ifndef NO_EXCEPTIONS
  try {
#endif
    int nv = pick_next_var(k, t);
    k->user_vars.insert(nv);

    // LLVM IR
    print(IRVec, k, "|V = alloca T\n\n", nv, t);
    print(IRSca, k, "|V = alloca T\n\n", nv, t);

    // CUDA/OpenCL
    print(CU, k, "|T V;\n", t, nv);
    print(CL, k, "|T V;\n", t, nv);
    k->expr[nv] = "v";
    print_T(&(k->expr[nv]), nv);

    return nv;
#ifndef NO_EXCEPTIONS
  } catch(std::exception &) {
    errno = ENOMEM;
    return -1;
  }
#endif
}

// ----------------------------------------------------------------------------
// Assignment

int trusimd_assign(kernel *k, int lvalue, int rvalue) {
#ifndef NO_EXCEPTIONS
  try {
#endif
    // LLVM IR
    type t = k->vars[lvalue];
    if (t != k->vars[rvalue]) {
      trusimd_errno = TRUSIMD_ETYPE;
      return -1;
    }
    print(IRVec, k, "|store T V, T* V\n\n", t, rvalue, t, lvalue);
    print(IRSca, k, "|store T V, T* V\n\n", t, rvalue, t, lvalue);

    // CUDA/OpenCL
    print(CU, k, "|V = S;\n", lvalue, k->expr[rvalue].c_str());
    print(CL, k, "|V = S;\n", lvalue, k->expr[rvalue].c_str());

    return 0;
#ifndef NO_EXCEPTIONS
  } catch(std::exception &) {
    trusimd_errno = TRUSIMD_ENOMEM;
    return -1;
  }
#endif
}

// ----------------------------------------------------------------------------
// Load from memory

int trusimd_load(kernel *k, int ptr, int offset) {
#ifndef NO_EXCEPTIONS
  try {
#endif
    // Type checking
    type ptr_t = k->vars[ptr];
    type t = remove_pointer(ptr_t);
    type vec_t = t;
    if (offset == k->global_index_var) { // should be done more seriously
      vec_t.scalar_vector = TRUSIMD_VECTOR;
    }
    type offset_t = k->vars[offset];
    if (!is_int(offset_t)) {
      trusimd_errno = TRUSIMD_ETYPE;
      return -1;
    }

    // LLVM IR
    int vptr = need_ir_var(k, ptr);
    int tmp = pick_next_var(k, vec_t);
    int voffset = need_ir_var(k, offset);
    print(IRVec, k, "|V = getelementptr inbounds T, T* V, T V\n", tmp, t, t,
          vptr, offset_t, voffset);
    int tmp2 = tmp;
    if (t != vec_t) {
      tmp2 = pick_next_var(k, vec_t);
      print(IRVec, k, "|V = bitcast T* V to T*\n", tmp2, t, tmp, vec_t);
    }
    int nv = pick_next_var(k, vec_t);
    print(IRVec, k, "|V = load T, T* V\n\n", nv, vec_t, vec_t, tmp2);

    print(IRSca, k, "|V = getelementptr inbounds T, T* V, T V\n", tmp, t, t,
          vptr, offset_t, voffset);
    print(IRSca, k, "|V = load T, T* V\n\n", nv, t, t, tmp);

    // CUDA/OpenCL
    k->expr[nv] = k->expr[ptr] + "[" + k->expr[offset] + "]";

    return nv;
#ifndef NO_EXCEPTIONS
  } catch(std::exception &) {
    trusimd_errno = TRUSIMD_ENOMEM;
    return -1;
  }
#endif
}

// ----------------------------------------------------------------------------
// Store to memory

int trusimd_store(kernel *k, int ptr, int offset, int v) {
#ifndef NO_EXCEPTIONS
  try {
#endif
    // Type checking
    type ptr_t = k->vars[ptr];
    type t = remove_pointer(ptr_t);
    type vec_t = t;
    if (offset == k->global_index_var) { // should be done more seriously
      vec_t.scalar_vector = TRUSIMD_VECTOR;
    }
    type offset_t = k->vars[offset];
    if (!is_int(offset_t) || (k->vars[v] != t && k->vars[v] != vec_t)) {
      trusimd_errno = TRUSIMD_ETYPE;
      return -1;
    }

    // LLVM IR
    int tmp = pick_next_var(k, ptr_t);
    int vptr = need_ir_var(k, ptr);
    int voffset = need_ir_var(k, offset);
    print(IRVec, k, "|V = getelementptr inbounds T, T* V, T V\n", tmp, t, t,
          vptr, offset_t, voffset);
    int tmp2 = tmp;
    if (t != vec_t) {
      tmp2 = pick_next_var(k, vec_t);
      print(IRVec, k, "|V = bitcast T* V to T*\n", tmp2, t, tmp, vec_t);
    }
    int vv = need_ir_var(k, v);
    print(IRVec, k, "|store T V, T* V\n\n", vec_t, vv, vec_t, tmp2);

    print(IRSca, k, "|V = getelementptr inbounds T, T* V, T V\n", tmp, t, t,
          vptr, offset_t, voffset);
    print(IRSca, k, "|store T V, T* V\n\n", t, vv, t, tmp);

    // CUDA/OpenCL
    print(CU, k, "|S[S] = S;\n\n", k->expr[ptr].c_str(),
          k->expr[offset].c_str(), k->expr[v].c_str());
    print(CL, k, "|S[S] = S;\n\n", k->expr[ptr].c_str(),
          k->expr[offset].c_str(), k->expr[v].c_str());

    return 0;
#ifndef NO_EXCEPTIONS
  } catch(std::exception &) {
    trusimd_errno = TRUSIMD_ENOMEM;
    return -1;
  }
#endif
}

// ----------------------------------------------------------------------------
// Get global ID

int trusimd_get_global_id(kernel *k) { return k->global_index_var; }

// ----------------------------------------------------------------------------
// Find first accelerator

trusimd_hardware *trusimd_find_first_hardware(trusimd_hardware *h, int n,
                                              int kind) {
  for (int i = 0; i < n; i++) {
    if (h->accelerator == kind) {
      return h + i;
    }
  }
  return NULL;
}

// ----------------------------------------------------------------------------
// Device malloc

void *trusimd_device_malloc(trusimd_hardware *h, size_t n) {
#ifndef NO_EXCEPTIONS
  try {
#endif
    // clang-format off
    switch(h->accelerator) {
    case TRUSIMD_LLVM: return llvm_device_malloc(h, n);
    case TRUSIMD_OPENCL: return opencl_device_malloc(h, n);
    case TRUSIMD_CUDA: return cuda_device_malloc(h, n);
    }
    // clang-format on
    return NULL; // should never be reached
#ifndef NO_EXCEPTIONS
  } catch (std::exception &e) {
    trusimd_errno = TRUSIMD_ENOMEM;
    return NULL;
  }
#endif
}

// ----------------------------------------------------------------------------
// Device free

void trusimd_device_free(trusimd_hardware *h, void *ptr) {
#ifndef NO_EXCEPTIONS
  try {
#endif
    // clang-format off
    switch(h->accelerator) {
    case TRUSIMD_LLVM: llvm_device_free(h, ptr); break;
    case TRUSIMD_OPENCL: opencl_device_free(h, ptr); break;
    case TRUSIMD_CUDA: cuda_device_free(h, ptr); break;
    }
    // clang-format on
#ifndef NO_EXCEPTIONS
  } catch (std::exception &e) {
    // no error on exception
  }
#endif
}

// ----------------------------------------------------------------------------
// Copy to device

int trusimd_copy_to_device(trusimd_hardware *h, void *dst, void *src,
                           size_t n) {
#ifndef NO_EXCEPTIONS
  try {
#endif
    // clang-format off
    switch(h->accelerator) {
    case TRUSIMD_LLVM: return llvm_copy_to_device(h, dst, src, n);
    case TRUSIMD_OPENCL: return opencl_copy_to_device(h, dst, src, n);
    case TRUSIMD_CUDA: return cuda_copy_to_device(h, dst, src, n);
    }
    // clang-format on
    return 0; // should never be reached
#ifndef NO_EXCEPTIONS
  } catch (std::exception &e) {
    trusimd_errno = TRUSIMD_ENOMEM;
    return -1;
  }
#endif
}

// ----------------------------------------------------------------------------
// Copy to host

int trusimd_copy_to_host(trusimd_hardware *h, void *dst, void *src, size_t n) {
#ifndef NO_EXCEPTIONS
  try {
#endif
    // clang-format off
    switch(h->accelerator) {
    case TRUSIMD_LLVM: return llvm_copy_to_host(h, dst, src, n);
    case TRUSIMD_OPENCL: return opencl_copy_to_host(h, dst, src, n);
    case TRUSIMD_CUDA: return cuda_copy_to_host(h, dst, src, n);
    }
    // clang-format on
    return 0; // should never be reached
#ifndef NO_EXCEPTIONS
  } catch (std::exception &e) {
    trusimd_errno = TRUSIMD_ENOMEM;
    return -1;
  }
#endif
}

// ----------------------------------------------------------------------------
// Compile and run

int trusimd_compile_run_ap(trusimd_hardware *h, kernel *k, int n, va_list ap) {
  int res = 0;
#ifndef NO_EXCEPTIONS
  try {
#endif
    switch (h->accelerator) {
    case TRUSIMD_LLVM:
      res = llvm_compile_run(h, k, n, ap);
      break;
    case TRUSIMD_OPENCL:
      res = opencl_compile_run(h, k, n, ap);
      break;
    case TRUSIMD_CUDA:
      res = cuda_compile_run(h, k, n, ap);
      break;
    }
#ifndef NO_EXCEPTIONS
  } catch (std::exception &e) {
    res = -1;
    trusimd_errno = TRUSIMD_ENOMEM;
  }
#endif
  return res;
}

int trusimd_compile_run(trusimd_hardware *h, kernel *k, int n, ...) {
  va_list ap;
  va_start(ap, n);
  int res = trusimd_compile_run_ap(h, k, n, ap);
  va_end(ap);
  return res;
}

// ----------------------------------------------------------------------------

} // extern "C"

