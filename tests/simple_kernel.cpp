#include <trusimd.hpp>
#include <iostream>
#include <cstring>

int main(int argc, char **argv) {
  using namespace trusimd;

  // Expect one argument
  if (argc != 2) {
    std::cerr << argv[0] << ": error: usage: " << argv[0]
              << " search_string\n";
    return -1;
  }

  // Poll hardware and select hardware based on argv[1]
  hardware &h = find_hardware(argv[1]);
  std::cout << argv[0] << ": info: selected " << h.description << '\n';

  // Create memory buffers
  const int n = 4096;
  buffer_pair<float> a(h, n), b(h, n), c(h, n);

  // Fill buffers with numbers
  for (int i = 0; i < n; i++) {
    b[i] = float(i);
    c[i] = float(i);
  }

  // Kernel
  kernel vector_add("vector_add", float32ptr, float32ptr, float32ptr);
  {
    arg(0)[gid] = arg(1)[gid] + arg(2)[gid];
  }

  // Print Kernel source code for debugging
  //std::cout << vector_add << std::endl;

  // Copy data to device, compile and execute kernel
  b.copy_to_device();
  c.copy_to_device();
  vector_add(h, n, a, b, c);

  // Check result
  a.copy_to_host();
  for (int i = 0; i < n; i++) {
    if (a[i] != b[i] + c[i]) {
      std::cerr << argv[0] << ": error: " << a[i] << " vs. " << (b[i] + c[i])
                << std::endl;
      return -1;
    } else {
      std::cout << argv[0] << ": info: " << a[i] << " --> " << (b[i] + c[i])
                << std::endl;
    }
  }

  return 0;
}
