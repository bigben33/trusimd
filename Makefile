PREFIX   = /usr
CXX      = g++ -static-libgcc
BUILD    = .
ROOT     = .
CXXFLAGS = -Wall -Wextra -pedantic -O3 -g
LDFLAGS  =

# -----------------------------------------------------------------------------

.POSIX:
.DEFAULT:
.SUFFIXES:

# -----------------------------------------------------------------------------
# Compilation of the library

$(BUILD)/libtrusimd.so: $(ROOT)/trusimd.cpp $(ROOT)/trusimd.h
	$(CXX) $(CXXFLAGS) -fPIC -shared $(ROOT)/trusimd.cpp $(LDFLAGS) -o $@

clean:
	rm -f $(BUILD)/libtrusimd.so

install: $(BUILD)/libtrusimd.so $(ROOT)/trusimd.h $(ROOT)/trusimd.hpp
	mkdir -p ${PREFIX}
	cp $(BUILD)/libtrusimd.so ${PREFIX}/lib
	cp $(ROOT)/trusimd.h $(ROOT)/trusimd.hpp ${PREFIX}/include

# -----------------------------------------------------------------------------
# C++ tests

ELDFLAGS  = $(LDFLAGS) -L$(BUILD) -ltrusimd -Wl,-rpath=$(BUILD)
ECXXFLAGS = $(CXXFLAGS) -I$(ROOT)
TARGETS = $(BUILD)/libtrusimd.so $(ROOT)/trusimd.h $(ROOT)/trusimd.hpp

$(BUILD)/simple_kernel: $(ROOT)/tests/simple_kernel.cpp $(TARGETS)
	$(CXX) $(ECXXFLAGS) $(ROOT)/tests/simple_kernel.cpp $(ELDFLAGS) -o $@

$(BUILD)/poll_hardware: $(ROOT)/tests/poll_hardware.cpp $(TARGETS)
	$(CXX) $(ECXXFLAGS) $(ROOT)/tests/poll_hardware.cpp $(ELDFLAGS) -o $@

# -----------------------------------------------------------------------------
# Python tests

$(BUILD)/trusimd.py: $(ROOT)/trusimd.py $(BUILD)/libtrusimd.so
	cp -f $(ROOT)/trusimd.py $@

$(BUILD)/poll_hardware.py: $(ROOT)/tests/poll_hardware.py $(BUILD)/trusimd.py
	cp -f $(ROOT)/tests/poll_hardware.py $@

$(BUILD)/simple_kernel.py: $(ROOT)/tests/simple_kernel.py $(BUILD)/trusimd.py
	cp -f $(ROOT)/tests/simple_kernel.py $@

# -----------------------------------------------------------------------------

tests: $(BUILD)/simple_kernel $(BUILD)/poll_hardware \
       $(BUILD)/simple_kernel.py $(BUILD)/poll_hardware.py
