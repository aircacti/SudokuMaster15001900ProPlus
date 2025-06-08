#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>

#define SIZE        9      // rozmiar planszy
#define BLOCK       3      // rozmiar bloku 3x3

// parametry GA
#define POP_SIZE    200    // liczebnosc populacji
#define MAX_GEN     5000   // maksymalna liczba generacji
#define TOURN_SIZE  3      // liczba w turnieju
#define CROSS_RATE  0.05    // prawdopodobienstwo krzyzowania
#define MUT_RATE    0.05    // prawdopodobienstwo mutacji

#define REPORT_GEN 50      // raport co

// dane sudoku, 0 = puste pole
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

// struktura jednego osobnika
typedef struct {
    int grid[SIZE][SIZE]; // wartosci w komorkach
    int fitness;          // ocena dopasowania
} Individual;

// deklaracje funkcji
void entry_individual(Individual *ind);
int  evaluate_fitness(const Individual *ind);
Individual* tournament_selection(Individual pop[]);
void crossover(const Individual *p1, const Individual *p2,
               Individual *c1, Individual *c2, int generation);
void mutate(Individual *ind, int generation);
void do_sudoku_ga(void);
void print_individual(const Individual *ind);
void print_iteration(int gen, const Individual *best);

// losowa liczba z przedzialu [a,b]
static int rand_range(int a, int b) {
    return a + rand() % (b - a + 1);
}

// inicjalizacja osobnika: uzupelnienie pustych pol w blokach 3x3
void entry_individual(Individual *ind) {
    bool fixed[SIZE][SIZE] = {false};
    // skopiuj stale wart
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            ind->grid[i][j] = puzzle[i][j];
            fixed[i][j] = (puzzle[i][j] != 0);
        }
    }
    // wypełnij kazdy blok
    for (int br = 0; br < BLOCK; br++) {
        for (int bc = 0; bc < BLOCK; bc++) {
            bool present[SIZE+1] = {false};
            // zaznacz juz obecne wartosci
            for (int di = 0; di < BLOCK; di++)
            for (int dj = 0; dj < BLOCK; dj++) {
                int v = ind->grid[br*BLOCK+di][bc*BLOCK+dj];
                if (v) present[v] = true;
            }
            int missing[SIZE], m = 0;
            // zbierz brakujace
            for (int v = 1; v <= SIZE; v++)
                if (!present[v]) missing[m++] = v;
            // losowo przemieszaj brakujace
            for (int x = m-1; x > 0; x--) {
                int y = rand() % (x+1);
                int tmp = missing[x]; missing[x] = missing[y]; missing[y] = tmp;
            }
            // wstaw w puste miejsca
            int idx = 0;
            for (int di = 0; di < BLOCK; di++)
            for (int dj = 0; dj < BLOCK; dj++) {
                int r = br*BLOCK+di, c = bc*BLOCK+dj;
                if (!fixed[r][c]) ind->grid[r][c] = missing[idx++];
            }
        }
    }
    // oblicz fitness poczatkowy
    ind->fitness = evaluate_fitness(ind);
}

// ocena ile roznych liczb w kazdym wierszu i kolumnie
int evaluate_fitness(const Individual *ind) {
    int score = 0;
    int row_cnt[SIZE+1], col_cnt[SIZE+1];
    for (int i = 0; i < SIZE; i++) {
        memset(row_cnt, 0, sizeof row_cnt);
        memset(col_cnt, 0, sizeof col_cnt);
        for (int j = 0; j < SIZE; j++) {
            row_cnt[ind->grid[i][j]]++;
            col_cnt[ind->grid[j][i]]++;
        }
        for (int v = 1; v <= SIZE; v++) {
            if (row_cnt[v] > 0) score++;
            if (col_cnt[v] > 0) score++;
        }
    }
    return score;
}

// selekcja turniejowa, najlepsze z TOURN_SIZE osobnikow
Individual* tournament_selection(Individual pop[]) {
    Individual *best = NULL;
    for (int i = 0; i < TOURN_SIZE; i++) {
        Individual *cand = &pop[rand() % POP_SIZE];
        if (!best || cand->fitness > best->fitness) best = cand;
    }
    return best;
}

// krzyzowanie swapuj losowo bloki 3x3 z CROSS_RATE %
void crossover(const Individual *p1, const Individual *p2,
               Individual *c1, Individual *c2, int generation) {
    // skopiuj rodzicow
    memcpy(c1->grid, p1->grid, sizeof p1->grid);
    memcpy(c2->grid, p2->grid, sizeof p2->grid);
    if ((double)rand()/RAND_MAX < CROSS_RATE) {
        for (int br = 0; br < BLOCK; br++) {
            for (int bc = 0; bc < BLOCK; bc++) {
                if (rand()%2) {
                    // zamien blok
                    for (int di = 0; di < BLOCK; di++)
                    for (int dj = 0; dj < BLOCK; dj++) {
                        int r = br*BLOCK+di, c = bc*BLOCK+dj;
                        int tmp = c1->grid[r][c];
                        c1->grid[r][c] = c2->grid[r][c];
                        c2->grid[r][c] = tmp;
                    }
                }
            }
        }
    }
    c1->fitness = evaluate_fitness(c1);
    c2->fitness = evaluate_fitness(c2);
}

// mutacja = w jednym bloku zamienia dwie losowe puste komorki
void mutate(Individual *ind, int generation) {
    if ((double)rand()/RAND_MAX >= MUT_RATE) return;
    int br = rand_range(0, BLOCK-1), bc = rand_range(0, BLOCK-1);
    int idxs[SIZE], m = 0;
    for (int di = 0; di < BLOCK; di++)
    for (int dj = 0; dj < BLOCK; dj++) {
        int r = br*BLOCK+di, c = bc*BLOCK+dj;
        if (puzzle[r][c] == 0) idxs[m++] = r*SIZE + c;
    }
    if (m < 2) return;
    int a = idxs[rand()%m], b = idxs[rand()%m];
    if (a != b) {
        int r1 = a/SIZE, c1 = a%SIZE;
        int r2 = b/SIZE, c2 = b%SIZE;
        int tmp = ind->grid[r1][c1];
        ind->grid[r1][c1] = ind->grid[r2][c2];
        ind->grid[r2][c2] = tmp;
        ind->fitness = evaluate_fitness(ind);
    }
}

// glowna petla
void do_sudoku_ga(void) {
    srand((unsigned)time(NULL));
    Individual pop[POP_SIZE], newpop[POP_SIZE];
    // inicjalizacja populacji
    for (int i = 0; i < POP_SIZE; i++) entry_individual(&pop[i]);
    Individual best = pop[0];

    for (int gen = 1; gen <= MAX_GEN; gen++) {
        // aktualizacja najlepszego
        for (int i = 0; i < POP_SIZE; i++)
            if (pop[i].fitness > best.fitness) best = pop[i];
        // raport co mniej jesli wystarczy
        if (gen % REPORT_GEN == 0) print_iteration(gen, &best);
        // tworzenie nastepnej populacji
        for (int i = 0; i < POP_SIZE; i += 2) {
            Individual *p1 = tournament_selection(pop);
            Individual *p2 = tournament_selection(pop);
            crossover(p1, p2, &newpop[i], &newpop[i+1], gen);
            mutate(&newpop[i],   gen);
            mutate(&newpop[i+1], gen);
        }
        memcpy(pop, newpop, sizeof pop);
        // znaleziono idealne rozwiazanie
        if (best.fitness == 2 * SIZE * SIZE) break;
    }
    // wynik
    printf("\nFinal best fitness %d/%d\n", best.fitness, 2*SIZE*SIZE);
    print_individual(&best);
}

// wyswietlenie
void print_individual(const Individual *ind) {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++)
            printf("%d ", ind->grid[i][j]);
        printf("\n");
    }
}

// raport
void print_iteration(int gen, const Individual *best) {
    printf("Gen %4d, best fitness=%3d\n", gen, best->fitness);
}

int main(void) {
    printf("GA Sudoku solver\n");
    do_sudoku_ga();
    return 0;
}