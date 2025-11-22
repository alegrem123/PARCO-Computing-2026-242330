#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

// CSR structure: stores matrix dimensions, nnz, and the three CSR arrays
typedef struct {
    int nrows, ncols, nnz;
    int *row_ptr, *col_idx;
    double *values;
} CSRMatrix;

// Read a Matrix Market file and convert it to CSR format
CSRMatrix read_matrix_market_to_csr(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error: cannot open %s\n", filename);
        exit(1);
    }

    // Skip Matrix Market comment lines starting with '%'
    char line[256];
    do {
        if (!fgets(line, sizeof(line), f)) exit(1);
    } while (line[0] == '%');

    // Read matrix dimensions (M rows, N columns, NNZ nonzeros)
    int M, N, NNZ;
    sscanf(line, "%d %d %d", &M, &N, &NNZ);

    // Temporary COO arrays
    int *row = malloc(NNZ * sizeof(int));
    int *col = malloc(NNZ * sizeof(int));
    double *val = malloc(NNZ * sizeof(double));

    // Matrix Market is 1-indexed; convert to 0-index
    for (int i = 0; i < NNZ; i++) {
        fscanf(f, "%d %d %lf", &row[i], &col[i], &val[i]);
        row[i]--;
        col[i]--;
    }
    fclose(f);

    // Allocate final CSR structure
    CSRMatrix A;
    A.nrows = M;
    A.ncols = N;
    A.nnz = NNZ;

    A.row_ptr = calloc(M + 1, sizeof(int));
    A.col_idx = malloc(NNZ * sizeof(int));
    A.values  = malloc(NNZ * sizeof(double));

    // Count how many nonzeros belong to each row → row_ptr
    for (int i = 0; i < NNZ; i++)
        A.row_ptr[row[i] + 1]++;

    // Prefix sum → starting index of each row in CSR format
    for (int i = 0; i < M; i++)
        A.row_ptr[i + 1] += A.row_ptr[i];

    // Tracks the current insertion position for each row
    int *offset = calloc(M, sizeof(int));

    // Fill the CSR arrays
    for (int i = 0; i < NNZ; i++) {
        int r = row[i];
        int dest = A.row_ptr[r] + offset[r]++;
        A.col_idx[dest] = col[i];
        A.values[dest]  = val[i];
    }

    free(row);
    free(col);
    free(val);
    free(offset);
    return A;
}

// CSR SpMV kernel (OpenMP parallel version). Parallelism is row-based.
void spmv(const CSRMatrix *A, const double *x, double *y) {
    #pragma omp parallel for schedule(runtime)   // scheduling chosen via OMP_SCHEDULE
    for (int i = 0; i < A->nrows; i++) {
        double sum = 0.0;

        // Iterate over the nonzeros in row i
        for (int j = A->row_ptr[i]; j < A->row_ptr[i + 1]; j++)
            sum += A->values[j] * x[A->col_idx[j]];

        y[i] = sum;
    }
}

int main(int argc, char **argv) {

    if (argc != 2) {
        printf("Usage: %s <matrix>\n", argv[0]);
        return 1;
    }

    // Load and convert the matrix into CSR format
    CSRMatrix A = read_matrix_market_to_csr(argv[1]);
    printf("Read %d×%d with %d nnz\n", A.nrows, A.ncols, A.nnz);

    // Allocate input and output vectors
    double *x = malloc(A.ncols * sizeof(double));
    double *y = malloc(A.nrows * sizeof(double));

    // Initialize vector x with random values in [-1000, 1000]
    srand(42);
    for (int i = 0; i < A.ncols; i++)
        x[i] = -1000.0 + 2000.0 * ((double)rand() / RAND_MAX);

    // Number of timed runs (default 1)
    int L = atoi(getenv("L") ? getenv("L") : "1");

    // Warm-up run to avoid cold-start effects
    spmv(&A, x, y);

    // Timed benchmark loop
    for (int k = 1; k <= L; k++) {
        double t0 = omp_get_wtime();
        spmv(&A, x, y);
        double t1 = omp_get_wtime();
        printf("SpMV time: %.3f ms\n", (t1 - t0) * 1000.0);
    }

    // Cleanup
    free(x);
    free(y);
    free(A.row_ptr);
    free(A.col_idx);
    free(A.values);

    return 0;
}
