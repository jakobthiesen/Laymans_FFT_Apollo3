import numpy as np
import matplotlib.pyplot as plt


N = 4
X = [0]*N


def fill_sin(f, phi,N):
    X = [0]*N
    step = f*2*np.pi/N
    for n in range(N):
        X[n] = np.cos(step*n+phi)

    return X

def bit_reverse_indices(N):
    """Return list of bit-reversed indices for an N-point FFT."""
    L = N.bit_length() - 1  # log2(N)
    out = []
    for i in range(N):
        r = int('{:0{width}b}'.format(i, width=L)[::-1], 2)
        out.append(r)
    return out

def bit_reverse_reorder(arr):
    N = len(arr)
    rev = bit_reverse_indices(N)
    return [arr[r] for r in rev]

for n in range(N):
    X[n] = n

L = int(np.log2(N))
X = bit_reverse_reorder(X)
print(X)
print(L)
# X = fill_sin(1,0, N)

mask_a = (0XFFFF>>(16-L))
mask_b = (0XFFFF>>(17-L))
print("")
for i in range(L):
    print("level: ", i)
    for j in range(int(N/2)):
        ja = j<<1
        jb = ja +1
        ja = ((ja<<i) | (ja>>(L-i))) & mask_a
        jb = ((jb<<i) | (jb>>(L-i))) & mask_b

        print(ja)
        print(jb)
        print("")




# Parameters
N = 4          # FFT size
L = 2          # log2(N), number of levels

# Masks like in C code
mask_a = (0xFFFF >> (16 - L))       # keeps L bits
mask_b = (0xFFFF >> (17 - L))       # one less than mask_a

print(f"mask_a: {mask_a:04X}, mask_b: {mask_b:04X}\n")

# Loop through FFT levels and compute addresses
for i in range(L):
    print(f"Level: {i}")
    for j in range(N // 2):
        # Base addresses for the butterfly pair
        ja = j << 1
        jb = ja + 1

        # Circular shift (emulates 16-bit C behavior)
        ja = ((ja << i) & mask_a) | (ja >> (L - i))
        jb = ((jb << i) & mask_a) | (jb >> (L - i))
        tw = (((~mask_b)>>i) & mask_b) & j
        print(f"j={j}: ja={ja}, jb={jb}, twAddr={tw}")
    print("")


# plt.figure()
# plt.plot(X)
# plt.show()
