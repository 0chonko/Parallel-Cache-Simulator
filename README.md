# MCPS

Implementation of a heat diffusion simulation comparing different cache coherency protocols (V-I and MOESI) with OpenMP parallelization.

## Project Structure

- `trace_example/`: Contains trace example from assignment PDF
- `heat_seq/`: Sequential implementation of heat dissipation
- `heat_omp/`: OpenMP parallel implementation with cache analysis
- `include/`: Header files for heat dissipation simulation
- `src/`: Source code files
- `images/`: Input pattern images
  - Contains makefile to generate new images (e.g., `make areas_500x500.pgm`)
- `heat_dissipation_reference_output/`: Reference outputs

## Cache Models

The project implements and compares two cache coherency protocols:
1. V-I (Valid-Invalid) protocol
2. MOESI (Modified-Owned-Exclusive-Shared-Invalid) protocol

### Cache Specifications
- Base cache size: 32kB (32768 bytes)
- Word size: 4 bytes
- Cache line size: 32 bytes (8 words)
- 8-way associative
- Total lines: 1024
- Total sets: 128

## Performance Analysis

Performance metrics analyzed:
- Hit rates with different core counts
- Memory access patterns
- Bus contention and acquisition times
- Speedup across different matrix sizes
- Cache-to-cache communication efficiency

## Implementation Versions

Five different implementations are analyzed:
1. Naive baseline version
2. Improved array access with direct pointer access
3. Tiled implementation (16x16)
4. Small-tile implementation (8x8)
5. OpenMP scheduling optimized version

## Building

```bash
# Sequential version
cd heat_seq
make

# OpenMP version
cd heat_omp
make
```

## Running

Example command:
```bash
./heat_seq -n 150 -m 100 -i 42 -e 0.0001 -c ../../images/pat1_100x150.pgm -t ../../images/pat1_100x150.pgm -r 1 -k 10 -L 0 -H 100
```

Parameters:
- `-n`: Grid width
- `-m`: Grid height
- `-i`: Iterations
- `-e`: Error threshold
- `-c`: Conductivity pattern
- `-t`: Temperature pattern
- `-r`: Report frequency
- `-k`: Thread count (OpenMP)
- `-L`: Min temperature
- `-H`: Max temperature

## Key Findings

- MOESI protocol shows more consistent hitrates (>80%) compared to V-I
- Tiled implementation (16x16) provides best performance for both protocols
- MOESI reduces memory accesses through cache-to-cache transfers
- Performance scales differently with 2, 4, and 8 cores depending on problem size
- Pattern complexity affects cache performance

