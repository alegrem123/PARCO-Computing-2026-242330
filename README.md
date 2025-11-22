# PARCO-Computing-2026-242330
# SpMV: Sequential vs Parallel Evaluation  
**Author:** Alessandro Gremes (ID 242330)  
**Course:** Parallel Computing 2025/2026  
**University:** University of Trento  

![Language](https://img.shields.io/badge/Language-C-blue)
![OpenMP](https://img.shields.io/badge/OpenMP-Enabled-brightgreen)
![HPC](https://img.shields.io/badge/HPC-Unitn_Cluster-orange)
![Reproducibility](https://img.shields.io/badge/Reproducibility-Verified-success)
![Benchmarks](https://img.shields.io/badge/Benchmarks-10_runs_per_matrix-red)
![Matrices](https://img.shields.io/badge/Matrices-6_selected-purple)


---

## 1. Project Description
This project implements and evaluates Sparse Matrix–Vector Multiplication (SpMV) using the CSR format, comparing:

- **Sequential SpMV**
- **Parallel SpMV using OpenMP**, evaluating:
  - Different thread counts (2–64)
  - Scheduling policies (`static`, `dynamic`, `guided`, `auto`)
  - Chunk sizes (10, 100, 1000)
  - Performance counters through `perf`
  - Cache behaviour through Valgrind Cachegrind

1.1 Reproducibility — Full Setup Script

The following block can be copied as-is into any Linux terminal to fully reproduce the environment, download matrices, compile code, and (optionally) submit PBS jobs on HPC UniTN.

⚠️ NOTE

- On local machines, PBS jobs are skipped automatically.
- On HPC UniTN, PBS jobs are automatically submitted.
- ### HPC Cluster Notes (UniTN)

All batch jobs were executed on the **public queue `short_cpuQ`**, which provides:

- Max walltime: **6 hours**
- Max concurrent jobs: **30**
- Access: free (public queue)
- Suitable for: medium-size workloads and benchmarking runs

This queue was selected because the project requires:
- short-to-medium executions (< 6h),
- multiple repeated runs (10× per matrix),
- access to up to 64 CPU cores for OpenMP evaluation.


Setup Script
```bash
echo "=== 1) Clone repository ==="
git clone https://github.com/alegrem123/PARCO-Computing-2026-242330.git
cd PARCO-Computing-2026-242330 || exit 1

echo "=== 2) Download SuiteSparse matrices ==="
cd mtx

for url in \
  "https://suitesparse-collection-website.herokuapp.com/MM/HB/1138_bus.tar.gz" \
  "https://suitesparse-collection-website.herokuapp.com/MM/BenElechi/BenElechi1.tar.gz" \
  "https://suitesparse-collection-website.herokuapp.com/MM/Williams/consph.tar.gz" \
  "https://suitesparse-collection-website.herokuapp.com/MM/Gupta/gupta2.tar.gz" \
  "https://suitesparse-collection-website.herokuapp.com/MM/Boeing/pwtk.tar.gz" \
  "https://suitesparse-collection-website.herokuapp.com/MM/Bova/rma10.tar.gz"
do
  fname=$(basename "$url")
  dirname="${fname%.tar.gz}"

  echo "Downloading $fname ..."
  wget "$url"

  echo "Extracting $fname ..."
  tar -xzf "$fname"

  echo "Cleaning archive..."
  rm "$fname"
done

cd ..

echo "=== 3) Compile sequential code ==="
cd src/sequentialCode
gcc -std=c99 seqCode.c -o seqCode
cd ../..

echo "=== 4) Compile parallel code (OpenMP) ==="
cd src/parallelCode
gcc -std=c99 -fopenmp parCode.c -o parCode
cd ../..

echo "=== 5) Submitting PBS jobs (only if running on HPC Unitn) ==="

if [[ "$HOSTNAME" == *"hpc"* ]]; then
    echo "Detected HPC environment — submitting jobs..."

    cd scripts/seqScript
    qsub -v L=10 seqCode.pbs
    qsub -v L=1 valgrind_seq.pbs

    cd ../parScript
    qsub -v L=10 parCode.pbs
    qsub -v L=1 perf_par.pbs

    echo "All jobs submitted!"
else
    echo "You are NOT on the HPC cluster."
    echo "Run these manually after ssh:"
    echo ""
    echo "Sequential:"
    echo "  cd scripts/seqScript"
    echo "  qsub -v L=10 seqCode.pbs"
    echo "  qsub -v L=1 valgrind_seq.pbs"
    echo ""
    echo "Parallel:"
    echo "  cd scripts/parScript"
    echo "  qsub -v L=10 parCode.pbs"
    echo "  qsub -v L=1 perf_par.pbs"
fi

echo "=== SETUP COMPLETE ==="
```
## Running Locally (Full  Guide)
1. Clone the repository
```bash
git clone https://github.com/alegrem123/PARCO-Computing-2026-242330.git
cd PARCO-Computing-2026-242330
```
3. Download the SuiteSparse matrices
```bash
cd mtx
```
```bash
for url in \
  "https://suitesparse-collection-website.herokuapp.com/MM/HB/1138_bus.tar.gz" \
  "https://suitesparse-collection-website.herokuapp.com/MM/BenElechi/BenElechi1.tar.gz" \
  "https://suitesparse-collection-website.herokuapp.com/MM/Williams/consph.tar.gz" \
  "https://suitesparse-collection-website.herokuapp.com/MM/Gupta/gupta2.tar.gz" \
  "https://suitesparse-collection-website.herokuapp.com/MM/Boeing/pwtk.tar.gz" \
  "https://suitesparse-collection-website.herokuapp.com/MM/Bova/rma10.tar.gz"
do
  fname=$(basename "$url")
  dirname="${fname%.tar.gz}"
  wget "$url"
  tar -xzf "$fname"
  rm "$fname"
done

cd ..
```
3. Compile locally

Sequential:
```bash
gcc -std=c99 src/sequentialCode/seqCode.c -o seqCode
```

Parallel (OpenMP):
```bash
gcc -std=c99 -fopenmp src/parallelCode/parCode.c -o parCode
```
4. Run locally
Sequential
```bash
./seq mtx/1138_bus/1138_bus.mtx
```
Parallel
```bash
export OMP_NUM_THREADS=8
export OMP_SCHEDULE="static,10"
./par mtx/1138_bus/1138_bus.mtx
```

5. Run all matrices

Sequential
```bash
for m in mtx/*/*.mtx; do ./seq "$m"; done
```

Parallel
```bash
export OMP_NUM_THREADS=8
export OMP_SCHEDULE="dynamic,100"
for m in mtx/*/*.mtx; do ./par "$m"; done
```
Notes

All runs use fixed seed: srand(42)

Matrix file structure must be:
```text
mtx/<matrix_name>/<matrix_name>.mtx
```

For benchmarking, recommended: 10 runs for sequential, 1 warm-up + 10 timed for parallel.
---

## 2. Tasks and Objectives
As required by the deliverable:

- Evaluate at least **five sparse matrices** with different sparsity patterns.
- Perform **≥ 10 runs** of each configuration and report:
  - mean time  
  - **90th percentile**
- Compare:
  - Sequential vs Parallel  
  - Scheduling policies  
  - Thread scaling  
- Identify performance bottlenecks:
  - Memory-bound vs compute-bound  
  - Cache misses (L1, LLC)
  - Branch behaviour
- Provide a fully reproducible repository.

---

## 3. Repository Structure

```text
PARCO-Computing-2026-242330/
├── README.md
├── src/
│   ├── sequentialCode/
│   │   └── seqCode.c
│   └── parallelCode/
│       └── parCode.c
├── scripts/
│   ├── seqScript/
│   │   ├── seqCode.pbs
│   │   ├── valgrind_seq.pbs
│   │   
│   └── parScript/
│       ├── parCode.pbs
│       ├── perf_par.pbs
│      
├── results/
│   ├── seqResults/
│   │   ├── benchMarking/
│   │   │   └── results_sequential.csv
│   │   └── benchMarkingCache/
│   │       └── valgrind_<matrix>.csv
│   └── parResults/
│       ├── benchMarking/
│       │   └── results_parallel_<threads>.csv
│       └── benchMarkingCache/
│           └── perf_parallel.csv
├── plots/
│   └── (generated locally: speedup, efficiency, scheduling comparison…)
└── mtx/
    └── <matrix_name>/<matrix_name>.mtx
```


> ⚠️ **Matrices >100 MB are not included in the repository** due to GitHub file size limits.  
The README provides instructions on how to download them.

---

## 4. Implementation  
### 4.1 Sequential Code  
`src/sequentialCode/seqCode.c`  
The implementation uses:
- CSR (Compressed Sparse Row) format
- Random dense input vector `x` generated with reproducible `srand(42)`
- SpMV repeated for `L` loops (default: 1, benchmarking: 10)

➡️ Full source in the repository.

---

### 4.2 Parallel Code  
`src/parallelCode/parCode.c`  

Parallel version uses:
- `#pragma omp parallel for schedule(runtime)`
- Runtime scheduling configured through PBS scripts
- CSR format identical to sequential version
- Repeated `L` times with `omp_get_wtime()`

➡️ Full source in the repository.

---

### 4.3 Supported scheduling strategies
- `static`
- `dynamic`
- `guided`
- `auto`

Chunk sizes: **10**, **100**, **1000**.

Thread counts: **2, 4, 8, 16, 32, 64**.

---

## 5. Experimental Setup

### 5.1 Hardware — HPC UniTN Node

```text
Architecture: x86_64
CPUs: 96 (Intel Xeon Gold 6252N @ 2.30GHz)
Sockets: 4
Cores per socket: 24
NUMA nodes: 4
L1 cache: 32 KB
L2 cache: 1 MB
L3 cache: 36 MB
```

### 5.2 Software Modules

```text
gcc91
valgrind-3.15.0
perf
```

### 5.3 Compilation

Sequential:

```text
gcc -std=c99 seqCode.c -o seqCode
```

Parallel:
```text
module load gcc91
gcc -std=c99 -fopenmp parCode.c -o parCode
```

---

## 6. Running Experiments (PBS)

### 6.1 Sequential Benchmarking (10 runs)

'''bash
cd scripts/seqScript
qsub -v L=10 seqCode.pbs
'''

### 6.2 Sequential Cache Analysis (Valgrind)

```bash
cd scripts/seqScript
qsub -v L=1 valgrind_seq.pbs
```

### 6.3 Parallel Benchmarking (OMP)

```bash
cd src/parallelCode
gcc -std=c99 -fopenmp parCode.c -o parCode
cd scripts/parScript
qsub -v L=10 parCode.pbs
```

### 6.4 Parallel Performance Counters (perf)

```bash
cd scripts/parScript
qsub -v L=1 perf_par.pbs
```

---

## 7. Matrices Used
All matrices are from the SuiteSparse Matrix Collection:

| Matrix       | nnz | nrows | ncols | Notes / Sparsity |
|--------------|-----|--------|--------|------------------|
| 1138_bus     | 4,054 | 1,138 | 1,138 | Power Network Problem |
| BenElechi1   | 13,150,496 | 245,874 | 245,874 | FEM structure |
| consph        | 6,010,480 | 83,334 | 83,334 | denser |
| gupta2        | 4,248,286 | 62,064 | 62,064 | irregular sparsity |
| pwtk          | 11,524,432 | 217,918 | 217,918 | large, expensive |
| rma10         | 2,329,092 | 46,835 | 46,835 | moderately sparse |

---

## 8. Results
- 10 runs per matrix for sequential and parallel times  
- Performance counters via `perf`  
- Cache counters via Valgrind  
- Thread scaling 2→64  
- Scheduling comparison  
- **Speedup**, **parallel efficiency**, **90th percentile**

---

## 9. Discussion
- Small matrices (e.g., **1138_bus**) do **not benefit** from parallelism due to parallel overhead.
- Medium and large matrices show meaningful speedups, especially with:
  - **static scheduling** for balanced and regular matrices  
  - **dynamic/guided** for irregular matrices (e.g., gupta2)
- Bottlenecks:
  - SpMV is **memory-bound** on all matrices  
  - Perf and Cachegrind confirm high LLC miss rates  
  - Parallel speedup limited by memory bandwidth saturation  

---


## 10. How to Download the Matrices
Since GitHub limits files to 100 MB, users must download the matrices manually:


```bash
cd mtx
for url in \
"https://suitesparse-collection-website.herokuapp.com/MM/Gupta/gupta2.tar.gz" \
"https://suitesparse-collection-website.herokuapp.com/MM/Williams/consph.tar.gz" \
"https://suitesparse-collection-website.herokuapp.com/MM/BenElechi/BenElechi1.tar.gz" \
"https://suitesparse-collection-website.herokuapp.com/MM/Boeing/pwtk.tar.gz" \
"https://suitesparse-collection-website.herokuapp.com/MM/Bova/rma10.tar.gz"
do
    name=$(basename "$url" .tar.gz)
    wget "$url" -O "$name.tar.gz"
    tar -xzf "$name.tar.gz"
    mkdir -p "$name"
    mv "$name/$name.mtx" "$name/"
    rm -rf "$name.tar.gz"
done


```
