#include <trusimd.hpp>
#include <iostream>

int main(int, char **argv) {
  trusimd_hardware *h;
  int n = trusimd_poll(&h);
  if (n == -1) {
    std::cerr << argv[0] << ": error: " << trusimd_strerror(trusimd_errno)
              << std::endl;
    return -1;
  }
  for (int i = 0; i < n; i++) {
    std::cout << h[i].description << std::endl;
  }
  return 0;
}
