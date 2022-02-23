import sys
from trusimd import *

# Expect one argument
if len(sys.argv) == 1:
    print('{}: error: usage: {} search_string'. \
          format(sys.argv[0], sys.argv[0]))
    sys.exit(1)

# Poll hardware and select hardware based on argv[1]
hardwares = poll_hardware()
h = [x for x in hardwares \
     if x.description.lower().find(sys.argv[1].lower()) >= 0]

if len(h) == 0:
    print('{}: info: no hardware could be selected'.format(sys.argv[0]))
else:
    h = h[0]
    print('{}: info: selected {}'.format(sys.argv[0], h))

# Create memory buffers
n = 1024
a = buffer_pair(h, n, float32)
b = buffer_pair(h, n, float32)
c = buffer_pair(h, n, float32)

# Fill buffers with numbers
for i in range(n):
    b[i] = i
    c[i] = i

# Kernel
with kernel('vector_add', float32ptr, float32ptr, float32ptr) as vector_add:
    arg(0)[gid] = arg(1)[gid] + arg(2)[gid]

# Print Kernel source code for debugging
print(vector_add)

# Copy data to device, compile and execute kernel
b.copy_to_device()
c.copy_to_device()
vector_add.run(h, n, a, b, c);

# Check result
a.copy_to_host();
for i in range(n):
    if a[i] != b[i] + c[i]:
        print('{}: error: {} vs. {}'.format(sys.argv[0], a[i], b[i] + c[i]))
        sys.exit(-1)
    else:
        print('{}: info: {} --> {}'.format(sys.argv[0], a[i], b[i] + c[i]))

