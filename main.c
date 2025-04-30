#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <time.h>


int selectedBoardSize = 0;
int selectedDifficulty = 0;
time_t gameStartTime;


const char* difficultyNames[] = { "latwy", "sredni", "trudny" };
const int boardSizes[] = { 4, 9, 16 };
float difficultyRatios[] = { 0.7f, 0.5f, 0.2f };  // latwy, sredni, trudny

void saveGame(const char *filename,
              time_t gameStartTime,
              int selectedBoardSize,
              int selectedDifficulty,
              int board[selectedBoardSize][selectedBoardSize],
              int userBoard[selectedBoardSize][selectedBoardSize]) {

    // ile trzeba miejsca
    int total = selectedBoardSize * selectedBoardSize;
    int bufLen = total * 6 + 1;
    char *bufSolved = malloc(bufLen);
    char *bufUser   = malloc(bufLen);
    if (!bufSolved || !bufUser) {
        perror("malloc");
        free(bufSolved);
        free(bufUser);
        return;
    }


    // Serializujemy stringa, ktory trzyma dane z boarda
    char *p = bufSolved;
    for (int i = 0; i < selectedBoardSize; i++) {
        for (int j = 0; j < selectedBoardSize; j++) {
            p += sprintf(p, "%d", board[i][j]);
            if (i * selectedBoardSize + j + 1 < total) {
                *p++ = ';';
            }
        }
    }
    *p = '\0';

    // Serializujemy stringa, ktory trzyma dane z user boarda
    p = bufUser;
    for (int i = 0; i < selectedBoardSize; i++) {
        for (int j = 0; j < selectedBoardSize; j++) {
            p += sprintf(p, "%d", userBoard[i][j]);
            if (i * selectedBoardSize + j + 1 < total) {
                *p++ = ';';
            }
        }
    }
    *p = '\0';


    // writing do pliku
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("fopen");
        free(bufSolved);
        free(bufUser);
        return;
    }

    // Wszystko wlatuje do osobnej lini pokolei do pliku
    fprintf(f, "%ld\n",  (long)gameStartTime);
    fprintf(f, "%d\n",   selectedBoardSize);
    fprintf(f, "%d\n",   selectedDifficulty);
    fprintf(f, "%s\n",   bufSolved);
    fprintf(f, "%s\n",   bufUser);
    fclose(f);

    free(bufSolved);
    free(bufUser);
    printf("Zapisano do %s\n", filename);
}

typedef struct {
    time_t startTime;
    int boardSize;
    int difficulty;
    int **board;
    int **userBoard;
} GameSave;



GameSave LoadOldGame(const char *filename) {
    GameSave gs = {0,0,0,NULL,NULL};
    FILE *f = fopen(filename, "r");
    if (!f) return gs;

    // odczytujemy linie
    if (fscanf(f, "%ld\n%d\n%d\n", (long*)&gs.startTime,
                                      &gs.boardSize,
                                      &gs.difficulty) != 3) {
        fclose(f);
        return gs;
    }


    if (gs.boardSize < 1) {
        fclose(f);
        return gs;
    }

    // Wczytanie tablic
    gs.board     = malloc(gs.boardSize * sizeof(int*));
    gs.userBoard = malloc(gs.boardSize * sizeof(int*));
    for (int i = 0; i < gs.boardSize; i++) {
        gs.board[i]     = malloc(gs.boardSize * sizeof(int));
        gs.userBoard[i] = malloc(gs.boardSize * sizeof(int));
    }


    size_t bufLen = gs.boardSize * gs.boardSize * 6 + 1;
    char *bufS = malloc(bufLen), *bufU = malloc(bufLen);
    if (!bufS || !bufU) goto fail;


    if (!fgets(bufS, bufLen, f) ||
        !fgets(bufU, bufLen, f)) {
        goto fail;
    }
    bufS[strcspn(bufS, "\r\n")] = '\0';
    bufU[strcspn(bufU, "\r\n")] = '\0';
    fclose(f);


    // wypelnienie tablic wartosciami
    char *tok;
    int total = gs.boardSize * gs.boardSize;
    tok = strtok(bufS, ";");
    for (int idx = 0; idx < total; idx++) {
        if (!tok) goto fail;
        int r = idx / gs.boardSize, c = idx % gs.boardSize;
        gs.board[r][c] = atoi(tok);
        tok = strtok(NULL, ";");
    }
    tok = strtok(bufU, ";");
    for (int idx = 0; idx < total; idx++) {
        if (!tok) goto fail;
        int r = idx / gs.boardSize, c = idx % gs.boardSize;
        gs.userBoard[r][c] = atoi(tok);
        tok = strtok(NULL, ";");
    }

    free(bufS);
    free(bufU);
    return gs;

fail:

    fclose(f);
    free(bufS); free(bufU);
    for (int i = 0; i < gs.boardSize; i++) {
        free(gs.board[i]);
        free(gs.userBoard[i]);
    }
    free(gs.board);
    free(gs.userBoard);
    gs.boardSize = 0;
    return gs;
}



void clearBuffer() {
    while (getchar() != '\n');
}

void printWelcomeMenu() {
    printf("\n   ***  Witaj w SudokuMaster15001900Pro+!  ***\n");
    printf("====================================================\n");
    printf("Aby utworzyc nowa gre, wybierz rozmiar planszy...\n");
    printf("1. 4x4\n");
    printf("2. 9x9\n");
    printf("3. 16x16\n");
    printf("====================================================\n");
    printf("lub...\n");
    printf("4. Wczytaj poprzednia gre\n");
    printf("5. Instrukcja\n");
    printf("6. Wyjdz\n");
    printf("Wpisz numer(1-6): ");
}

void printDifficultyPrompt() {
    printf("\nWybrano plansze %dx%d. Wybierz poziom trudnosci...\n", selectedBoardSize, selectedBoardSize);
    printf("1. latwy\n");
    printf("2. sredni\n");
    printf("3. trudny\n");
    printf("Wpisz numer(1-3): ");
}

void printNewStartMessage() {
    printf("\nUruchamiam nowa gre: %dx%d (%s)\n", selectedBoardSize, selectedBoardSize, difficultyNames[selectedDifficulty - 1]);
}

void printOldStartMessage(time_t gameStartTime) {
    char timeStr[64];
    struct tm *tm_info = localtime(&gameStartTime);
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm_info);

    printf("\nUruchamiam gre z %s: %dx%d (%s)\n", timeStr, selectedBoardSize, selectedBoardSize, difficultyNames[selectedDifficulty - 1]);
}



void generateEmptyBoard(int board[selectedBoardSize][selectedBoardSize]) {

    for (int i = 0; i < selectedBoardSize; i++) {
        for (int j = 0; j < selectedBoardSize; j++) {
            board[i][j] = 0;
        }
    }
}

void printBoard(int board[selectedBoardSize][selectedBoardSize]) {
    int blockSize = (int)sqrt(selectedBoardSize);
    int maxVal = selectedBoardSize;


    int cellWidth = (maxVal < 10) ? 2 : 3;


    printf("%*s", cellWidth + 1, " ");
    for (int j = 0; j < selectedBoardSize; j++) {
        if (j > 0 && j % blockSize == 0)
            printf("|");

        printf(" %*c", cellWidth - 1, 'A' + j);
    }
    printf("\n");

    for (int i = 0; i < selectedBoardSize; i++) {

        if (i > 0 && i % blockSize == 0) {
            printf("%*s", cellWidth + 1, " ");
            for (int k = 0; k < selectedBoardSize; k++) {
                if (k > 0 && k % blockSize == 0)
                    printf("+");
                for (int d = 0; d < cellWidth; d++) printf("-");
            }
            printf("\n");
        }


        printf("%*d", cellWidth, i + 1);
        printf(" ");

        for (int j = 0; j < selectedBoardSize; j++) {
            if (j > 0 && j % blockSize == 0)
                printf("|");

            if (board[i][j] == 0)
                printf(" %*s", cellWidth - 1, ".");
            else
                printf(" %*d", cellWidth - 1, board[i][j]);
        }

        printf("\n");
    }
}



int isSafe(int board[selectedBoardSize][selectedBoardSize], int row, int col, int num) {
    int boxSize = (int)sqrt(selectedBoardSize);


    for (int i = 0; i < selectedBoardSize; i++) {
        if (board[row][i] == num || board[i][col] == num)
            return 0;
    }


    int startRow = row - row % boxSize;
    int startCol = col - col % boxSize;

    for (int i = 0; i < boxSize; i++) {
        for (int j = 0; j < boxSize; j++) {
            if (board[startRow + i][startCol + j] == num)
                return 0;
        }
    }

    return 1;
}

int fillBoard(int board[selectedBoardSize][selectedBoardSize], int row, int col) {
    if (row == selectedBoardSize) return 1;

    int nextRow = (col == selectedBoardSize - 1) ? row + 1 : row;
    int nextCol = (col + 1) % selectedBoardSize;


    int numbers[selectedBoardSize];
    for (int i = 0; i < selectedBoardSize; i++) numbers[i] = i + 1;


    for (int i = selectedBoardSize - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = numbers[i];
        numbers[i] = numbers[j];
        numbers[j] = tmp;
    }

    for (int i = 0; i < selectedBoardSize; i++) {
        int num = numbers[i];

        if (isSafe(board, row, col, num)) {
            board[row][col] = num;
            if (fillBoard(board, nextRow, nextCol)) return 1;
            board[row][col] = 0;
        }
    }

    return 0;
}


void generateUserBoard(int solvedBoard[selectedBoardSize][selectedBoardSize],
                       int userBoard[selectedBoardSize][selectedBoardSize]) {

    int totalCells = selectedBoardSize * selectedBoardSize;
    float visibleRatio = difficultyRatios[selectedDifficulty - 1];
    int visibleCells = (int)(totalCells * visibleRatio);


    for (int i = 0; i < selectedBoardSize; i++) {
        for (int j = 0; j < selectedBoardSize; j++) {
            userBoard[i][j] = 0;
        }
    }


    int placed = 0;
    while (placed < visibleCells) {
        int row = rand() % selectedBoardSize;
        int col = rand() % selectedBoardSize;

        if (userBoard[row][col] == 0) {
            userBoard[row][col] = solvedBoard[row][col];
            placed++;
        }
    }
}

int parseInput(char input[10], int *row, int *col, int *num) {

    char colChar = toupper(input[0]);
    int rowNum = atoi(&input[1]);


    if (colChar < 'A' || colChar > 'A' + selectedBoardSize - 1 || rowNum < 1 || rowNum > selectedBoardSize) {
        return 0;
    }

    *col = colChar - 'A';
    *row = rowNum - 1;


    printf("Podaj liczbe do wpisania (1-%d): ", selectedBoardSize);
    if (scanf("%d", num) != 1 || *num < 1 || *num > selectedBoardSize) {
        clearBuffer();
        return 0;
    }

    return 1;
}


bool areAllFieldsFilled(int userBoard[][selectedBoardSize], int size) {
    for (int row = 0; row < size; row++) {
        for (int col = 0; col < size; col++) {
            if (userBoard[row][col] == 0)
                return false;
        }
    }
    return true;
}

bool checkIfDoneGood(int board[][selectedBoardSize],
                     int userBoard[][selectedBoardSize],
                     int size) {
    for (int row = 0; row < size; row++) {
        for (int col = 0; col < size; col++) {
            if (board[row][col] != userBoard[row][col])
                return false;
        }
    }
    return true;
}



int updateUserBoard(int userBoard[selectedBoardSize][selectedBoardSize]) {
    char input[10];
    int row, col, num;

    printf("\nS - zapisz, R - rozwiazanie, E - wyjscie lub");
    printf("\npodaj pole aby wpisac liczbe (np. A4): ");
    if (scanf("%s", input) != 1) {
        clearBuffer();
        return 0;
    }


    if (toupper(input[0]) == 'E' && input[1] == '\0') {
        return 2;
    }

    if (toupper(input[0]) == 'S' && input[1] == '\0') {
        return 3;
    }

    if (toupper(input[0]) == 'R' && input[1] == '\0') {
        return 4;
    }


    if (!parseInput(input, &row, &col, &num)) {
        printf("Blad: Niepoprawny adres lub liczba. Sprobuj ponownie.\n");
        return 0;
    }

    if (userBoard[row][col] != 0) {
        printf("Nadpisano %d na %d w %s.\n", userBoard[row][col], num, input);
    } else {
        printf("Wpisano %d w pole %s.\n", num, input);
    }

    userBoard[row][col] = num;

    printf("\nTwoja plansza:\n");
    printBoard(userBoard);

    return 0;
}




int main() {
    srand(time(NULL));

    while (1) {

        int boardChoice = 0;
        int difficultyChoice = 0;

        printWelcomeMenu();
        if (scanf("%d", &boardChoice) != 1 || (boardChoice < 1 || boardChoice > 6)) {
            clearBuffer();
            printf("Blad: Niepoprawny wybor. Sprobuj ponownie!\n");
            continue;
        }


        if (boardChoice == 5) {
            printf("\n========== INSTRUKCJA ==========\n");
            printf("Sudoku to lamiglowka liczbowa, ktorej celem jest\n");
            printf("wypelnienie kwadratowej planszy o wymiarach n x n\n");
            printf("(najczesciej 9x9) cyframi od 1 do n tak, aby:\n\n");
            printf("- W kazdym wierszu kazda cyfra wystepowala dokladnie raz\n");
            printf("- W kazdej kolumnie kazda cyfra wystepowala dokladnie raz\n");
            printf("- W kazdym z mniejszych kwadratow kazda cyfra wystepowala raz\n\n");
            printf("W trakcie gry:\n");
            printf("- Wybierasz pole podajac np. A4, a nastepnie wpisujesz liczbe\n");
            printf("- Mozesz wpisac 'S' aby zapisac stan gry\n");
            printf("- Mozesz wpisac 'R' aby podejrzec rozwiazana plansze\n");
            printf("- Mozesz wpisac 'E' aby wyjsc z gry i wrocic do menu\n");
            printf("Gra automatycznie poinformuje Cie, gdy poprawnie wypelnisz sudoku i wyswietli czas.\n");
            printf("================================\n\n");
            printf("Kliknij ENTER, aby wrocic do menu wejsciowego...\n");

            clearBuffer();
            getchar();
            continue;

        }

        if (boardChoice == 6) {
            printf("Do zobaczenia!\n");
            exit(1);
        }

        bool loading = false;

        if (boardChoice == 4) {
            loading = true;

        }

        GameSave gm = LoadOldGame("sudoku_save.json");

        selectedBoardSize = (loading) ? gm.boardSize : boardSizes[boardChoice - 1];

        if (!loading) {
            printDifficultyPrompt();

            if (scanf("%d", &difficultyChoice) != 1 || (difficultyChoice < 1 || difficultyChoice > 3)) {
                clearBuffer();
                printf("Blad: Niepoprawny poziom trudnosci. Sprobuj ponownie!\n");
                continue;
            }
        }

        selectedDifficulty = (loading) ? gm.difficulty : difficultyChoice;

        int board[selectedBoardSize][selectedBoardSize];
        int userBoard[selectedBoardSize][selectedBoardSize];

        if (loading) {
            gameStartTime = gm.startTime;
            for (int i = 0; i < gm.boardSize; i++) {
                for (int j = 0; j < gm.boardSize; j++) {
                    board[i][j] = gm.board[i][j];
                }
            }
            for (int i = 0; i < gm.boardSize; i++) {
                for (int j = 0; j < gm.boardSize; j++) {
                    userBoard[i][j] = gm.userBoard[i][j];
                }
            }
            printOldStartMessage(gameStartTime);
        } else {
            printNewStartMessage();
            time(&gameStartTime);
            generateEmptyBoard(board);
            fillBoard(board, 0, 0);
            generateUserBoard(board, userBoard);
        }


        printf("\nTwoja plansza:\n");
        printBoard(userBoard);


        while (1) {
            int result = updateUserBoard(userBoard);

            if (result == 2) {
                printf("Wychodzisz do menu wejsciowego! Nie zapisane zmiany zostaly utracone.\n");
                break;
            } else if (result == 3) {
                saveGame("sudoku_save.json", gameStartTime, selectedBoardSize, selectedDifficulty, board, userBoard);
                printf("Zapisano stan gry! Wychodzisz do menu wejsciowego. Wznow przez wczytanie poprzedniej gry.\n");
                break;
            } else if (result == 4) {
                printf("Rozwiazana plansza:\n");
                printBoard(board);
                printf("\n");
            } else {
                if (areAllFieldsFilled(userBoard, selectedBoardSize)) {
                    if (checkIfDoneGood(board, userBoard, selectedBoardSize)) {
                        printf("\n\nGratulacje! Poprawnie wypelniono plansze!");
                        printf("\nStatystyki: ");
                        time_t currentTime = time(NULL);
                        double elapsedTime = difftime(currentTime, gameStartTime);
                        printf("\n- Start: %s", ctime(&gameStartTime));
                        printf("\n- Uplynelo: %.0f sekundy\n", elapsedTime);
                    } else {
                        printf("Blednie wypisane sudoku! Znajdz i popraw bledy.");
                    }
                }
            }


        }



        clearBuffer();
    }

    return 0;
}
