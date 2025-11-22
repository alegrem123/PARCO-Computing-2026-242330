#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

// CSR structure storing dimensions and the three CSR arrays
typedef struct {
    int nrows, ncols, nnz;
    int *row_ptr, *col_idx;
    double *values;
} CSRMatrix;

// Read a Matrix Market file and convert it to CSR format
CSRMatrix read_matrix_market_to_csr(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) exit(1);

    // Skip comment lines starting with '%'
    char line[256];
    do {
        if (!fgets(line, sizeof(line), f)) exit(1);
    } while (line[0] == '%');

    // Read matrix dimensions and number of nonzeros
    int M, N, NNZ;
    sscanf(line, "%d %d %d", &M, &N, &NNZ);

    // Temporary COO buffers (Matrix Market format)
    int *row = malloc(NNZ * sizeof(int));
    int *col = malloc(NNZ * sizeof(int));
    double *val = malloc(NNZ * sizeof(double));

    // Load COO triplets (convert to 0-indexed)
    for (int i = 0; i < NNZ; i++) {
        fscanf(f, "%d %d %lf", &row[i], &col[i], &val[i]);
        row[i]--;
        col[i]--;
    }
    fclose(f);

    // Allocate CSR data structure
    CSRMatrix A;
    A.nrows = M;
    A.ncols = N;
    A.nnz   = NNZ;

    A.row_ptr = calloc(M + 1, sizeof(int));
    A.col_idx = malloc(NNZ * sizeof(int));
    A.values  = malloc(NNZ * sizeof(double));

    // Count nonzeros per row
    for (int i = 0; i < NNZ; i++)
        A.row_ptr[row[i] + 1]++;

    // Prefix sum → starting offset of each row in CSR layout
    for (int i = 0; i < M; i++)
        A.row_ptr[i + 1] += A.row_ptr[i];

    // Track insertion offsets during COO → CSR conversion
    int *offset = calloc(M, sizeof(int));

    // Fill CSR arrays in row-major order
    for (int i = 0; i < NNZ; i++) {
        int r = row[i];
        int dest = A.row_ptr[r] + offset[r]++;
        A.col_idx[dest] = col[i];
        A.values[dest]  = val[i];
    }

    free(row); free(col); free(val); free(offset);
    return A;
}

// Sequential CSR SpMV kernel (row-by-row traversal)
void spmv(const CSRMatrix *A, const double *x, double *y) {
    for (int i = 0; i < A->nrows; i++) {
        double sum = 0.0;

        // Multiply-accumulate across the nonzeros of row i
        for (int j = A->row_ptr[i]; j < A->row_ptr[i + 1]; j++)
            sum += A->values[j] * x[A->col_idx[j]];

        y[i] = sum;
    }
}

// Compute checksum of the output vector (used to verify correctness)
double compute_checksum(const double *y, int n) {
    double s = 0.0;
    for (int i = 0; i < n; i++)
        s += y[i];
    return s;
}

int main(int argc, char **argv) {
    if (argc != 2) return 1;

    // Load matrix and convert it to CSR representation
    CSRMatrix A = read_matrix_market_to_csr(argv[1]);

    // Allocate input vector x and output vector y
    double *x = malloc(A.ncols * sizeof(double));
    double *y = malloc(A.nrows * sizeof(double));

    // Initialize x with deterministic random values in [-1000, 1000]
    srand(42);
    for (int i = 0; i < A.ncols; i++)
        x[i] = -1000.0 + 2000.0 * ((double)rand() / RAND_MAX);

    // Warm-up run to avoid first-iteration overheads
    spmv(&A, x, y);

    // Determine number of benchmark iterations from env var L
    char *Lenv = getenv("L");
    int loops = Lenv ? atoi(Lenv) : 1;

    // Benchmark loop (each iteration prints its own timing and checksum)
    for (int r = 1; r <= loops; r++) {
        struct timeval start, end;
        gettimeofday(&start, NULL);

        spmv(&A, x, y);

        gettimeofday(&end, NULL);

        double elapsed_ms =
            (end.tv_sec - start.tv_sec) * 1000.0 +
            (end.tv_usec - start.tv_usec) / 1000.0;

        double checksum = compute_checksum(y, A.nrows);

        printf("%d,%.3f,%.6f\n", r, elapsed_ms, checksum);
    }

    // Cleanup
    free(x);
    free(y);
    free(A.row_ptr);
    free(A.col_idx);
    free(A.values);

    return 0;
}
