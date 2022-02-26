PREFIX   = /usr
CXX      = g++ -static-libgcc
FC       = gfortran
ROOT     = .
CXXFLAGS = -Wall -Wextra -pedantic -O3 -g
FFLAGS   = -O3 -g
LDFLAGS  = -std=f2008 -pedantic

# -----------------------------------------------------------------------------

.POSIX:
.DEFAULT:
.SUFFIXES:

all: libtrusimd.so trusimd.o trusimd.mod

# -----------------------------------------------------------------------------
# Compilation of the library (C++ and Fortran module)

libtrusimd.so: $(ROOT)/trusimd.cpp $(ROOT)/trusimd.h
	$(CXX) $(CXXFLAGS) -fPIC -shared $(ROOT)/trusimd.cpp $(LDFLAGS) -o $@

trusimd.o: $(ROOT)/trusimd.f90 libtrusimd.so
	$(FC) $(FFLAGS) -c $(ROOT)/trusimd.f90

clean:
	rm -f libtrusimd.so

install: libtrusimd.so $(ROOT)/trusimd.h $(ROOT)/trusimd.hpp
	mkdir -p ${PREFIX}
	cp libtrusimd.so ${PREFIX}/lib
	cp $(ROOT)/trusimd.h $(ROOT)/trusimd.hpp ${PREFIX}/include

# -----------------------------------------------------------------------------
# C++ tests

ELDFLAGS   = $(LDFLAGS) -L. -ltrusimd -Wl,-rpath='$$ORIGIN'
ECXXFLAGS  = $(CXXFLAGS) -I$(ROOT)
EFFLAGS    = $(FFLAGS) -I. trusimd.o
CXXTARGETS = libtrusimd.so $(ROOT)/trusimd.hpp
FTARGETS   = trusimd.o

simple_kernel_cpp: $(ROOT)/tests/simple_kernel.cpp $(CXXTARGETS)
	$(CXX) $(ECXXFLAGS) $(ROOT)/tests/simple_kernel.cpp $(ELDFLAGS) -o $@

poll_hardware_cpp: $(ROOT)/tests/poll_hardware.cpp $(CXXTARGETS)
	$(CXX) $(ECXXFLAGS) $(ROOT)/tests/poll_hardware.cpp $(ELDFLAGS) -o $@

# -----------------------------------------------------------------------------
# Fortran tests

simple_kernel_f90: $(ROOT)/tests/simple_kernel.f90 $(FTARGETS)
	$(FC) $(EFFLAGS) $(ROOT)/tests/simple_kernel.f90 $(ELDFLAGS) -o $@

poll_hardware_f90: $(ROOT)/tests/poll_hardware.f90 $(FTARGETS)
	$(FC) $(EFFLAGS) $(ROOT)/tests/poll_hardware.f90 $(ELDFLAGS) -o $@

# -----------------------------------------------------------------------------
# Python tests

trusimd.py: $(ROOT)/trusimd.py libtrusimd.so
	cp -f $(ROOT)/trusimd.py $@

poll_hardware.py: $(ROOT)/tests/poll_hardware.py trusimd.py
	cp -f $(ROOT)/tests/poll_hardware.py $@

simple_kernel.py: $(ROOT)/tests/simple_kernel.py trusimd.py
	cp -f $(ROOT)/tests/simple_kernel.py $@

# -----------------------------------------------------------------------------

tests: simple_kernel_cpp poll_hardware_cpp simple_kernel.py poll_hardware.py \
       simple_kernel_f90 poll_hardware_f90
