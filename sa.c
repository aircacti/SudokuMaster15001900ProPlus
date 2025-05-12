#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>

#define SIZE        9
#define BLOCK       3
#define T_START     5.0
#define T_END       1e-3
#define ALPHA       0.995
#define MAX_ITER    200000

// dane, 0=puste
int puzzle[SIZE][SIZE] = {
    {5,3,0, 0,7,0, 0,0,0},
    {6,0,0, 1,9,5, 0,0,0},
    {0,9,8, 0,0,0, 0,6,0},

    {8,0,0, 0,6,0, 0,0,3},
    {4,0,0, 8,0,3, 0,0,1},
    {7,0,0, 0,2,0, 0,0,6},

    {0,6,0, 0,0,0, 2,8,0},
    {0,0,0, 4,1,9, 0,0,5},
    {0,0,0, 0,8,0, 0,7,9}
};

// stan
typedef struct {
    int grid[SIZE][SIZE];
    int cost;
} SudokuState;

void entry_state(SudokuState *state);
int  calculate_cost(const SudokuState *state);
bool generate_neighbor_inplace(SudokuState *neighbor, const SudokuState *current);
void do_sudoku_sa(double T_start, double T_end, double alpha, int max_iter);
void print_state(const SudokuState *state);
void print_iteration(int iter, double T, const SudokuState *current, const SudokuState *best, bool accepted);

// random num
static int rand_range(int a, int b) {
    return a + rand() % (b - a + 1);
}


void entry_state(SudokuState *state) {
    bool fixed[SIZE][SIZE] = {false};

    // copy
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            state->grid[i][j] = puzzle[i][j];
            fixed[i][j] = (puzzle[i][j] != 0);
        }
    }

    //  fillowanie
    for (int br = 0; br < BLOCK; br++) {
        for (int bc = 0; bc < BLOCK; bc++) {
            bool present[SIZE+1] = {false};
            for (int di = 0; di < BLOCK; di++)
                for (int dj = 0; dj < BLOCK; dj++) {
                    int v = state->grid[br*BLOCK+di][bc*BLOCK+dj];
                    if (v) present[v] = true;
                }
            int missing[SIZE], m = 0;
            for (int v = 1; v <= SIZE; v++)
                if (!present[v]) missing[m++] = v;

            // permutacje
            for (int x = m-1; x > 0; x--) {
                int y = rand() % (x+1);
                int tmp = missing[x];
                missing[x] = missing[y];
                missing[y] = tmp;
            }

            // wstawianie
            int idx = 0;
            for (int di = 0; di < BLOCK; di++)
                for (int dj = 0; dj < BLOCK; dj++) {
                    int r = br*BLOCK+di, c = bc*BLOCK+dj;
                    if (!fixed[r][c])
                        state->grid[r][c] = missing[idx++];
                }
        }
    }
    state->cost = calculate_cost(state);
}

// duplikaty liczenie
int calculate_cost(const SudokuState *state) {
    int cost = 0;
    int row_cnt[SIZE+1], col_cnt[SIZE+1];
    for (int i = 0; i < SIZE; i++) {
        memset(row_cnt, 0, sizeof row_cnt);
        memset(col_cnt, 0, sizeof col_cnt);
        for (int j = 0; j < SIZE; j++) {
            row_cnt[state->grid[i][j]]++;
            col_cnt[state->grid[j][i]]++;
        }
        for (int v = 1; v <= SIZE; v++) {
            if (row_cnt[v] > 1) cost += row_cnt[v] - 1;
            if (col_cnt[v] > 1) cost += col_cnt[v] - 1;
        }
    }
    return cost;
}


bool generate_neighbor_inplace(SudokuState *neighbor, const SudokuState *current) {
    memcpy(neighbor, current, sizeof *neighbor);
    int br = rand_range(0, BLOCK-1), bc = rand_range(0, BLOCK-1);
    int r1, c1, r2, c2;
    do {
        r1 = br*BLOCK + rand_range(0, BLOCK-1);
        c1 = bc*BLOCK + rand_range(0, BLOCK-1);
    } while (puzzle[r1][c1] != 0);
    do {
        r2 = br*BLOCK + rand_range(0, BLOCK-1);
        c2 = bc*BLOCK + rand_range(0, BLOCK-1);
    } while ((r1==r2 && c1==c2) || puzzle[r2][c2] != 0);
    int tmp = neighbor->grid[r1][c1];
    neighbor->grid[r1][c1] = neighbor->grid[r2][c2];
    neighbor->grid[r2][c2] = tmp;
    neighbor->cost = calculate_cost(neighbor);
    return true;
}

// wypisz
void print_state(const SudokuState *state) {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++)
            printf("%d ", state->grid[i][j]);
        printf("\n");
    }
}

// info o iteracji
void print_iteration(int iter, double T, const SudokuState *current, const SudokuState *best, bool accepted) {
    printf("Iteracja %5d, T=%.4f, curr_cost=%3d, best_cost=%3d: %s\n",
        iter, T, current->cost, best->cost,
        accepted ? "dobre" : "odrzucam");
}

// main
void do_sudoku_sa(double T_start, double T_end, double alpha, int max_iter) {
    srand((unsigned)time(NULL));
    SudokuState current, neighbor, best;
    entry_state(&current);
    best = current;

    int accepted_count = 0, rejected_count = 0;
    int found_iter = -1;

    double T = T_start;
    int iter;
    for (iter = 1; iter <= max_iter && T > T_end; iter++) {
        bool accepted = false;
        generate_neighbor_inplace(&neighbor, &current);
        int delta = neighbor.cost - current.cost;

        if (delta < 0 || ((double)rand()/RAND_MAX) < exp(-delta / T)) {
            current = neighbor;
            accepted = true;
            accepted_count++;
            if (current.cost < best.cost) {
                best = current;
                if (best.cost == 0 && found_iter < 0) {
                    found_iter = iter;
                }
            }
        } else {
            rejected_count++;
        }

        print_iteration(iter, T, &current, &best, accepted);

        if (best.cost == 0) break;
        T *= alpha;
    }

    // podsumowanie
    printf("\n\n\nWynik:\n");
    printf("Wykonane iteracje: %d\n", iter);
    printf("Dobre: %d\n", accepted_count);
    printf("Zle: %d\n", rejected_count);
    printf("Najlepszy koszt: %d\n", best.cost);
    if (found_iter > 0)
        printf("Rozwiazanie w %d iteracji\n", found_iter);
    else
        printf("Brak idealnego rozwiazania\n");
    printf("\nNajlepsze znalezione rozwiazanie:\n");
    print_state(&best);
}

int main(void) {
    printf("Wyzaranie Sudoku\n\n");
    do_sudoku_sa(T_START, T_END, ALPHA, MAX_ITER);
    return 0;
}
