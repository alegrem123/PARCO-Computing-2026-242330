#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

typedef struct {
    int nrows, ncols, nnz;
    int *row_ptr, *col_idx;
    double *values;
} CSRMatrix;

CSRMatrix read_matrix_market_to_csr(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error: cannot open %s\n", filename);
        exit(1);
    }

    char line[256];
    do {
        if (!fgets(line, sizeof(line), f)) exit(1);
    } while (line[0] == '%');

    int M, N, NNZ;
    sscanf(line, "%d %d %d", &M, &N, &NNZ);

    int *row = malloc(NNZ * sizeof(int));
    int *col = malloc(NNZ * sizeof(int));
    double *val = malloc(NNZ * sizeof(double));

    for (int i = 0; i < NNZ; i++) {
        fscanf(f, "%d %d %lf", &row[i], &col[i], &val[i]);
        row[i]--;
        col[i]--;
    }
    fclose(f);

    CSRMatrix A;
    A.nrows = M;
    A.ncols = N;
    A.nnz = NNZ;

    A.row_ptr = calloc(M + 1, sizeof(int));
    A.col_idx = malloc(NNZ * sizeof(int));
    A.values = malloc(NNZ * sizeof(double));

    for (int i = 0; i < NNZ; i++)
        A.row_ptr[row[i] + 1]++;

    for (int i = 0; i < M; i++)
        A.row_ptr[i + 1] += A.row_ptr[i];

    int *offset = calloc(M, sizeof(int));

    for (int i = 0; i < NNZ; i++) {
        int r = row[i];
        int dest = A.row_ptr[r] + offset[r]++;
        A.col_idx[dest] = col[i];
        A.values[dest] = val[i];
    }

    free(row);
    free(col);
    free(val);
    free(offset);
    return A;
}

void spmv(const CSRMatrix *A, const double *x, double *y) {
    #pragma omp parallel for schedule(runtime)
    for (int i = 0; i < A->nrows; i++) {
        double sum = 0.0;
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

    CSRMatrix A = read_matrix_market_to_csr(argv[1]);
    printf("Read %d×%d with %d nnz\n", A.nrows, A.ncols, A.nnz);

    double *x = malloc(A.ncols * sizeof(double));
    double *y = malloc(A.nrows * sizeof(double));

    srand(42);
    for (int i = 0; i < A.ncols; i++)
        x[i] = -1000.0 + 2000.0 * ((double)rand() / RAND_MAX);

    int L = atoi(getenv("L") ? getenv("L") : "1");

    spmv(&A, x, y);  

    for (int k = 1; k <= L; k++) {
        double t0 = omp_get_wtime();
        spmv(&A, x, y);
        double t1 = omp_get_wtime();
        printf("SpMV time: %.3f ms\n", (t1 - t0) * 1000.0);
    }

    free(x);
    free(y);
    free(A.row_ptr);
    free(A.col_idx);
    free(A.values);

    return 0;
}

