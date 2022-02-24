#ifdef WITH_LLVM
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Error.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/Passes/PassBuilder.h>

// ----------------------------------------------------------------------------

TRUSIMD_TLS char llvm_error[256];

// ----------------------------------------------------------------------------

static inline int llvm_poll(std::vector<trusimd_hardware> *v) {
  llvm::StringRef cpu = llvm::sys::getHostCPUName();
  llvm::StringMap<bool> features;
  if (!llvm::sys::getHostCPUFeatures(features)) {
    return -1;
  }
  const char *known_features[] = {"sse",    "sse2", "sse3", "ssse3", "sse4.1",
                                  "sse4.2", "avx",  "avx2", "neon",  "asimd"};
  const int simd_width[] = {128, 128, 128, 128, 128, 128, 256, 256, 128, 128};
  for (int i = 0; i < int(sizeof(known_features) / sizeof(const char *));
       i++) {
    if (features.find(known_features[i]) != features.end()) {
      std::string buf("LLVM ");
      buf += cpu.str();
      buf += ' ';
      buf += known_features[i];
      trusimd_hardware h;
      strcpy(h.id, known_features[i]);
      memcpy((void *)h.param1, (void *)&simd_width[i], sizeof(int));
      h.accelerator = TRUSIMD_LLVM;
      my_strlcpy(h.description, buf.c_str(), sizeof(h.description));
      v->push_back(h);
    }
  }
  return 0;
}

// ----------------------------------------------------------------------------

static inline const char *llvm_strerror(void) { return llvm_error; }

// ----------------------------------------------------------------------------

static inline void *llvm_device_malloc(trusimd_hardware *, size_t n) {
  void *res = malloc(n);
  if (res == NULL) {
    trusimd_errno = TRUSIMD_ENOMEM;
  }
  return res;
}

// ----------------------------------------------------------------------------

static inline void llvm_device_free(trusimd_hardware *, void *ptr) {
  free(ptr);
}

// ----------------------------------------------------------------------------

int llvm_copy_to_device(trusimd_hardware *, void *dst, void *src, size_t n) {
  memcpy(dst, src, n);
  return 0;
}

// ----------------------------------------------------------------------------

int llvm_copy_to_host(trusimd_hardware *, void *dst, void *src, size_t n) {
  memcpy(dst, src, n);
  return 0;
}

// ----------------------------------------------------------------------------

static inline void llvm_ir_replace(std::string *s_, size_t pos,
                                   std::string const &r) {
  std::string &s = *s_;
  for (size_t i = 0; i < r.size(); i++) {
    s[pos + i] = r[i];
  }
}

static inline int llvm_compile_run(trusimd_hardware *h_, kernel *k, int n,
                                   va_list ap) {
  using namespace llvm;
  trusimd_hardware &h = *h_;

  // TODO: First we set the vector width
  // In the meantime we assume floats/int... so we take simd_width / 4
  std::string buf;
  int simd_length;
  memcpy((void *)&simd_length, (void *)h.param1, sizeof(int));
  simd_length /= 32;
  print_T(&buf, simd_length);
  buf += std::string(10 /* 10 = sizeof("??????????") */ - buf.size(), ' ');
  for (size_t i = 0; i < k->type_pos.size(); i++) {
    llvm_ir_replace(&k->llvm_ir_vec, k->type_pos[i], buf);
  }
  //std::cout << k->llvm_ir_vec << std::endl;

  // This is mandatory (once is enough though)
  InitializeNativeTarget();

  // Some needed stuff (I fail to see why these defaults are necessary)
  orc::ThreadSafeContext tls_context(std::make_unique<LLVMContext>());

  // LLVM object that represents the LLVM IR
  std::unique_ptr<MemoryBuffer> ir =
      MemoryBuffer::getMemBuffer(k->llvm_ir_vec.c_str());

  // Parse the LLVM IR
  SMDiagnostic diag;
  std::unique_ptr<Module> M =
      parseIR(*ir.get(), diag, *tls_context.getContext());
  if (M.get() == NULL) {
    std::string buf("LLVM IR:");
    print_T(&buf, diag.getLineNo());
    buf += ':';
    print_T(&buf, diag.getColumnNo());
    buf += ": ";
    buf += diag.getMessage().data();
    my_strlcpy(llvm_error, buf.c_str(), sizeof(llvm_error));
    trusimd_errno = TRUSIMD_ELLVM;
    return -1;
  }

  // Create the analysis managers.
  LoopAnalysisManager LAM;
  FunctionAnalysisManager FAM;
  CGSCCAnalysisManager CGAM;
  ModuleAnalysisManager MAM;

  // Create the new pass manager builder.
  PassBuilder PB;

  // Register all the basic analyses with the managers.
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

  // Create the pass manager.
  // This one corresponds to a typical -O3 optimization pipeline.
  ModulePassManager MPM =
      PB.buildPerModuleDefaultPipeline(PassBuilder::OptimizationLevel::O3);

  // Optimize the IR!
  MPM.run(*M.get(), MAM);

  // Create the JIT engine
  auto JIT = orc::LLJITBuilder().create();
  if (!JIT) {
    Error err = JIT.takeError();
    std::stringstream ss;
    ss << "LLVM JIT: " << toString(std::move(err));
    my_strlcpy(llvm_error, ss.str().c_str(), sizeof(llvm_error));
    trusimd_errno = TRUSIMD_ELLVM;
    return -1;
  }
  Error err = JIT.get()->addIRModule(
      orc::ThreadSafeModule(std::move(M), std::move(tls_context)));
  if (err) {
    std::stringstream ss;
    ss << "LLVM JIT: " << toString(std::move(err));
    my_strlcpy(llvm_error, ss.str().c_str(), sizeof(llvm_error));
    trusimd_errno = TRUSIMD_ELLVM;
    return -1;
  }

  // Copy arguments
  size_t nb_args = k->args.size();
  std::vector<char> args(8 * nb_args);
  for (size_t i = 0; i < nb_args; i++) {
    if (is_pointer(k->args[i])) {
      void *ptr = va_arg(ap, void *);
      memcpy((void *)&args[8 * i], (void *)&ptr, sizeof(void *));
    } else {
      switch(k->args[i].width) {
      case 8: {
        unsigned char uc = (unsigned char)va_arg(ap, unsigned int);
        memcpy((void *)&args[8 * i], (void *)&uc, 1);
        break;
      }
      case 16: {
        unsigned short us = (unsigned short)va_arg(ap, unsigned int);
        memcpy((void *)&args[8 * i], (void *)&us, 2);
        break;
      }
      case 32: {
        unsigned int ui = va_arg(ap, unsigned int);
        memcpy((void *)&args[8 * i], (void *)&ui, 4);
        break;
      }
      case 64: {
        unsigned long ul = va_arg(ap, unsigned long);
        memcpy((void *)&args[8 * i], (void *)&ul, 8);
        break;
      }
      }
    }
  }

  // Execute function
  auto func = JIT.get()->lookup(k->name.c_str());
  if (!func) {
    err = JIT.takeError();
    std::stringstream ss;
    ss << "LLVM JIT: " << toString(std::move(err));
    my_strlcpy(llvm_error, ss.str().c_str(), sizeof(llvm_error));
    trusimd_errno = TRUSIMD_ELLVM;
    return -1;
  }
  auto f = (void (*)(long, char *))func.get().getAddress();

  std::cout << "DEBUG\n";
  f(long(n), &args[0]);

  return 0;
}

// ----------------------------------------------------------------------------

#else
static inline int llvm_poll(std::vector<trusimd_hardware> *) { return 0; }
static inline const char *llvm_strerror(void) { return NULL; }
static inline void *llvm_device_malloc(trusimd_hardware *, size_t) {
  trusimd_errno = TRUSIMD_EAVAIL;
  return NULL;
}
static inline void llvm_device_free(trusimd_hardware *, void *) {}
static inline int llvm_copy_to_device(trusimd_hardware *, void *, void *, size_t) {
  trusimd_errno = TRUSIMD_EAVAIL;
  return -1;
}
static inline int llvm_copy_to_host(trusimd_hardware *, void *, void *, size_t) {
  trusimd_errno = TRUSIMD_EAVAIL;
  return -1;
}
static inline int llvm_compile_run(trusimd_hardware *, kernel *, int,
                                   va_list) {
  trusimd_errno = TRUSIMD_EAVAIL;
  return -1;
}
#endif

