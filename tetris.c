/*
 * tetris.c - A super simple implementation of the classic Tetris game written
 *            in c using Raylib.
 *
 * author: mellowboyxd
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "raylib.h"

#define TARGET_FPS 60
#define UPDATE_DELAY (TARGET_FPS * 1) /* 1 second */
#define CELL_SIZE 32
#define DEFAULT_Y -32
#define WALL_OFFSET 5
#define TETRA_SHAPE 3
#define PLAY_WIDTH 10
#define PLAY_HEIGHT 20
#define WELL_WIDTH (PLAY_WIDTH * CELL_SIZE)
#define WELL_HEIGHT (PLAY_HEIGHT * CELL_SIZE)
#define RPANEL_WIDTH 218
#define DPANEL_HEIGHT 32
#define SCREEN_WIDTH (WELL_WIDTH + WALL_OFFSET + RPANEL_WIDTH)
#define SCREEN_HEIGHT (WELL_HEIGHT + WALL_OFFSET + DPANEL_HEIGHT)

enum CellColors {CELL_NONE, CELL_BLUE, CELL_RED, CELL_YELLOW, CELL_GREEN};

typedef enum Direction {NONE, LEFT, RIGHT, UP, DOWN} Direction;

typedef enum GameState {MENU_STATE, PLAY_STATE, LOSE_STATE} GameState;

typedef struct Cell {
        Vector2 pos;
        Color color;
} Cell;

typedef struct Tetramino {
        float x;
        float y;
        int shape[TETRA_SHAPE][TETRA_SHAPE];
        Color color;
} Tetramino;

void DrawCell(float x, float y, Color color);
void DrawCellCell(Cell cell);
void DrawCellCellInsideWell(Cell cell);
void DrawPanels();
void DrawTextOnRPanel(const char *text, int x, int y, int fontSize, 
                Color color);
void DrawTextOnDPanel(const char *text, int x, int y, int fontSize, 
                Color color);
void DrawPeekTetraminoBlock(int nextTetramino[TETRA_SHAPE][TETRA_SHAPE]);
void DrawScore(int score);
void DrawLevel(int level);
void DrawCommands();
void DrawCredits();
void DrawUI();
void DrawWell(int well[PLAY_HEIGHT][PLAY_WIDTH]);
void DrawTetramino(Tetramino tetramino);

bool isThereYCollision(Tetramino tetra, int well[PLAY_HEIGHT][PLAY_WIDTH]);
bool isThereXCollision(Tetramino tetra, int well[PLAY_HEIGHT][PLAY_WIDTH], 
                Direction dir);

void MarkWell(Tetramino tetra, int well[PLAY_HEIGHT][PLAY_WIDTH]);
void ResetTetramino(Tetramino *current, int next[TETRA_SHAPE][TETRA_SHAPE]);
void GenerateRandomTetramino(int next[TETRA_SHAPE][TETRA_SHAPE]);
void HandleInput(Tetramino *tetra, int well[PLAY_HEIGHT][PLAY_WIDTH]);
void RotateTetraminoShape(int currentShape[TETRA_SHAPE][TETRA_SHAPE], 
                bool clockwise);
void CopyMatrix(int m, int n, int old[m][n], int new[m][n]);
void CheckWinningCondition(int well[PLAY_HEIGHT][PLAY_WIDTH], int *score);
void CheckLosingCondition(int well[PLAY_HEIGHT][PLAY_WIDTH], 
                GameState *gameState);
int GetWinningRow(int well[PLAY_HEIGHT][PLAY_WIDTH]);
void ClearRow(int row, int well[PLAY_HEIGHT][PLAY_WIDTH]);
void CopyRows(int n, int old[n], int new[n]);
void ShiftDownRows(int startRow, int well[PLAY_HEIGHT][PLAY_WIDTH]);
void IncrementScore(int *score);

void MainDraw(int well[PLAY_HEIGHT][PLAY_WIDTH]);
void MainUpdate();
void UpdatePlayGame();

int nextTetramino[TETRA_SHAPE][TETRA_SHAPE];

Tetramino currentTetramino = {
        .x = CELL_SIZE * 3,
        .y = DEFAULT_Y,
        .shape = {0},
        .color = BLUE
};

int well[PLAY_HEIGHT][PLAY_WIDTH] = {0};

int score = 0;
int level = 1;

GameState gameState = PLAY_STATE;

int main(void)
{
        InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Tetris Game");
        SetTargetFPS(TARGET_FPS);

        GenerateRandomTetramino(currentTetramino.shape);
        GenerateRandomTetramino(nextTetramino);

        while(!WindowShouldClose()) {
                BeginDrawing();
                ClearBackground(GRAY);
                MainDraw(well);
                MainUpdate();
                EndDrawing();
        }
        CloseWindow();
        return 0;
}

/*
 * ===== UPDATE FUNCTIONS =====
 */

int frameCounter = 0;
void MainUpdate()
{
        frameCounter++;

        HandleInput(&currentTetramino, well);

        if (frameCounter >= UPDATE_DELAY) {
                if (gameState == PLAY_STATE) {
                        UpdatePlayGame();
                }
                frameCounter = 0;
        }
}

void UpdatePlayGame() {
        if (!isThereYCollision(currentTetramino, well))
                currentTetramino.y += CELL_SIZE;
        else {
                // update well
                MarkWell(currentTetramino, well);

                ResetTetramino(&currentTetramino, nextTetramino);

                CheckWinningCondition(well, &score);
                CheckLosingCondition(well, &gameState);

                GenerateRandomTetramino(nextTetramino);
        }
}

/*
 * ============================
 */

/*
 * ===== HELPER FUNCTIONS =====
 */

bool isThereYCollision(Tetramino tetra, int well[PLAY_HEIGHT][PLAY_WIDTH])
{
        for (int i = 0; i < TETRA_SHAPE; i++) {
                for (int j = 0; j < TETRA_SHAPE; j++) {
                        float x = j * CELL_SIZE + tetra.x;
                        float y = i * CELL_SIZE + tetra.y;
                        if (tetra.shape[i][j] == 1) {
                                int row = y / CELL_SIZE;
                                int col = x / CELL_SIZE;
                                if(row + 1 >= PLAY_HEIGHT)
                                        return true;
                                if (row >= 0 && well[row + 1][col] != 0) {
                                        return true;
                                }
                        }
                }
        }
        return false;
}

bool isThereXCollision(Tetramino tetra, int well[PLAY_HEIGHT][PLAY_WIDTH], 
                Direction dir)
{
        for (int i = 0; i < TETRA_SHAPE; i++) {
                for (int j = 0; j < TETRA_SHAPE; j++) {
                        float x = j * CELL_SIZE + tetra.x;
                        float y = i * CELL_SIZE + tetra.y;
                        if (tetra.shape[i][j] == 1) {
                                int row = y / CELL_SIZE;
                                int col = x / CELL_SIZE;

                                if (dir == LEFT)
                                        col -= 1;
                                else if (dir == RIGHT)
                                        col += 1;

                                if (col < 0 || col >= PLAY_WIDTH)
                                        return true;

                                if (row >= 0 && well[row][col] != 0) {
                                        return true;
                                }
                        }
                }
        }
        return false;
}

void MarkWell(Tetramino tetra, int well[PLAY_HEIGHT][PLAY_WIDTH])
{
        for (int i = 0; i < TETRA_SHAPE; i++) {
                for (int j = 0; j < TETRA_SHAPE; j++) {
                        float x = j * CELL_SIZE + tetra.x;
                        float y = i * CELL_SIZE + tetra.y;
                        if (tetra.shape[i][j] == 1) {
                                int row = y / CELL_SIZE;
                                int col = x / CELL_SIZE;
                                well[row][col] = tetra.shape[i][j];
                        }
                }
        }
}

void ResetTetramino(Tetramino *current, int next[TETRA_SHAPE][TETRA_SHAPE])
{
        for (int i = 0; i < TETRA_SHAPE; i++) {
                for (int j = 0; j < TETRA_SHAPE; j++) {
                        current->x -= CELL_SIZE;
                        current->shape[i][j] = next[i][j];
                }
        }

        // TODO: generate a random color

        current->x = CELL_SIZE * 3;
        current->y = DEFAULT_Y;
}

void GenerateRandomTetramino(int next[TETRA_SHAPE][TETRA_SHAPE])
{
        srand(time(NULL));

        int type = (rand() % 6) + 1;
        int (*nextType)[TETRA_SHAPE][TETRA_SHAPE]; 

        switch(type) {
                case 1:
                        nextType = &((int [TETRA_SHAPE][TETRA_SHAPE]) {
                                {1, 1, 1},
                                {0, 1, 0},
                                {0, 0, 0}
                        });
                        break;

                case 2:
                        nextType = &((int [TETRA_SHAPE][TETRA_SHAPE]) {
                                {1, 1, 0},
                                {0, 1, 1},
                                {0, 0, 0}
                        });
                        break;

                case 3:
                        nextType = &((int [TETRA_SHAPE][TETRA_SHAPE]){
                                {1, 0, 0},
                                {1, 1, 0},
                                {0, 0, 0}
                        });
                        break;

                case 4:
                        nextType = &((int [TETRA_SHAPE][TETRA_SHAPE]){
                                {1, 1, 0},
                                {1, 1, 0},
                                {0, 0, 0}
                        });
                        break;

                case 5:
                        nextType = &((int [TETRA_SHAPE][TETRA_SHAPE]){
                                {0, 0, 1},
                                {1, 1, 1},
                                {0, 0, 0}
                        });
                        break;

                case 6:
                        nextType = &((int [TETRA_SHAPE][TETRA_SHAPE]){
                                {0, 0, 0},
                                {1, 1, 1},
                                {0, 0, 0}
                        });
                        break;
        }

        for (int i = 0; i < TETRA_SHAPE; i++) {
                for (int j = 0; j < TETRA_SHAPE; j++) {
                        next[i][j] = (*nextType)[i][j];
                }
        }
}

void HandleInput(Tetramino *tetra, int well[PLAY_HEIGHT][PLAY_WIDTH])
{
        int oldShape[TETRA_SHAPE][TETRA_SHAPE] = {0};
        int key = GetKeyPressed();
        switch (key) {
                case KEY_RIGHT:
                        if (!isThereXCollision(*tetra, well, RIGHT))
                                tetra->x += CELL_SIZE;
                        break;
                case KEY_LEFT:
                        if (!isThereXCollision(*tetra, well, LEFT))
                                tetra->x -= CELL_SIZE;
                        break;
                case KEY_DOWN:
                        if (!isThereYCollision(*tetra, well))
                                tetra->y += CELL_SIZE;
                        break;
                case KEY_UP:
                        // TODO: rotate tetramino 90 deg anti-clockwise
                        break;
                case KEY_SPACE:
                        CopyMatrix(TETRA_SHAPE, TETRA_SHAPE, tetra->shape, 
                                        oldShape);
                        RotateTetraminoShape(tetra->shape, true);
                        if (isThereYCollision(*tetra, well) || 
                                isThereXCollision(*tetra, well, NONE)) {
                                // reset
                                CopyMatrix(TETRA_SHAPE, TETRA_SHAPE, oldShape, 
                                                tetra->shape);
                        }
                        break;
        }
}

void TransposeMatrix(int m, int n, int matrix[m][n])
{
        // turn rows into columns
        for (int i = 0; i < m; i++) {
                for (int j = i + 1; j < n; j++) {
                        int tmp = matrix[i][j];
                        matrix[i][j] = matrix[j][i];
                        matrix[j][i] = tmp;
                }
        }
}

void ReverseArray(int arr[], int n)
{
        int l = 0; 
        int r = n - 1;
        for(; l < r; l++, r--) {
                int tmp = arr[r];
                arr[r] = arr[l];
                arr[l] = tmp;
        }
}

void CopyMatrix(int m, int n, int old[m][n], int new[m][n])
{
        for (int i = 0; i < m; i++) {
                for (int j = 0; j < n; j++) {
                        new[i][j] = old[i][j];
                }
        }
}

void CopyRows(int n, int old[n], int new[n])
{
        for (int i = 0; i < n; i++) {
                new[i] = old[i];
        }
}

void RotateTetraminoShape(int currentShape[TETRA_SHAPE][TETRA_SHAPE], 
                bool clockwise)
{
        TransposeMatrix(TETRA_SHAPE, TETRA_SHAPE, currentShape);

        if (clockwise) {
                for (int i = 0; i < TETRA_SHAPE; i++) {
                        ReverseArray(currentShape[i], TETRA_SHAPE);
                }
        }
}

void IncrementScore(int *score)
{
        *score += 20;
}

void CheckWinningCondition(int well[PLAY_HEIGHT][PLAY_WIDTH], int *score)
{
        int winningRow;
        while ((winningRow = GetWinningRow(well)) != -1) {
                ClearRow(winningRow, well);
                ShiftDownRows(winningRow, well);
                IncrementScore(score);
        }
}

void CheckLosingCondition(int well[PLAY_HEIGHT][PLAY_WIDTH], 
                GameState *gameState)
{
        int start = PLAY_WIDTH / 2 - TETRA_SHAPE / 2;

        for (int i = start; i < (start + TETRA_SHAPE); i++) {
                if (well[0][i] != 0) {
                        *gameState = LOSE_STATE;
                        return;
                }
        }
}

void ClearRow(int row, int well[PLAY_HEIGHT][PLAY_WIDTH])
{
        for (int j = 0; j < PLAY_WIDTH; j++) {
                well[row][j] = 0;
        }
}

void ShiftDownRows(int startRow, int well[PLAY_HEIGHT][PLAY_WIDTH])
{
        for (int i = startRow; i > 1; i--) {
                CopyRows(PLAY_WIDTH, well[i - 1], well[i]);
        }
}

/* returns the first winning row from bottom to top */
int GetWinningRow(int well[PLAY_HEIGHT][PLAY_WIDTH])
{
        for (int i = PLAY_HEIGHT - 1; i >= 0; i--) {
                bool isComplete = true;
                for (int j = 0; j < PLAY_WIDTH; j++) {
                        if (well[i][j] == 0) {
                                isComplete = false;
                        }
                }

                if (isComplete)
                        return i;
        }

        return -1;
}

/*
 * ============================
 */

/*
 * ===== DRAW FUNCTIONS =====
 */

void MainDraw(int well[PLAY_HEIGHT][PLAY_WIDTH])
{
        DrawUI();
        DrawWell(well);
        DrawTetramino(currentTetramino);

}

void DrawWell(int well[PLAY_HEIGHT][PLAY_WIDTH])
{
        for (int row = 0; row < PLAY_HEIGHT; row++) {
                for (int col = 0; col < PLAY_WIDTH; col++) {
                        float x = col * CELL_SIZE + WALL_OFFSET;
                        float y = row * CELL_SIZE + WALL_OFFSET;

                        switch(well[row][col]) {
                                case CELL_BLUE:
                                        DrawCell(x, y, BLUE);
                                        break;
                                case CELL_RED:
                                        DrawCell(x, y, RED);
                                        break;
                                default:
                                        DrawRectangle(x, y, CELL_SIZE, 
                                                        CELL_SIZE, BLACK);
                        }
                }
        }
}

void DrawUI()
{
        DrawTextOnRPanel("Tetris", 20, 20, 48, BLACK);
        if (gameState == PLAY_STATE) {
                DrawPeekTetraminoBlock(nextTetramino);
        } else if (gameState == LOSE_STATE) {
                DrawTextOnRPanel("GAME OVER", 20, 128, 30, BLACK);
        }
        DrawScore(score);
        DrawLevel(level);
        DrawCommands();
        DrawCredits();
}

void DrawCellCell(Cell cell)
{
        const int borderWidth = 2;

        DrawRectangle(cell.pos.x, cell.pos.y, CELL_SIZE, CELL_SIZE, RAYWHITE);
        DrawRectangle(
                        cell.pos.x + borderWidth, 
                        cell.pos.y + borderWidth, 
                        CELL_SIZE - borderWidth * 2, 
                        CELL_SIZE - borderWidth * 2, 
                        cell.color
        );
}

void DrawCellCellInsideWell(Cell cell)
{
        float x = (cell.pos.x < WELL_WIDTH - CELL_SIZE) ? 
                cell.pos.x + WALL_OFFSET : cell.pos.x;
        float y = (cell.pos.y < WELL_HEIGHT - CELL_SIZE) ?
                cell.pos.y + WALL_OFFSET : cell.pos.y - WALL_OFFSET;

        DrawCell(x, y, cell.color);
}

void DrawCell(float x, float y, Color color)
{
        Cell cell = {
                .pos = (Vector2) {x, y},
                .color = color
        };
        DrawCellCell(cell);
}

void DrawTextOnRPanel(const char *text, int x, int y, int fontSize, Color color) 
{
        float posX = WELL_WIDTH + WALL_OFFSET + x;
        float posY = WALL_OFFSET + y;
        DrawText(text, posX, posY, fontSize, color);
}

void DrawTextOnDPanel(const char *text, int x, int y, int fontSize, Color color) 
{
        float posX = WALL_OFFSET + x;
        float posY = WELL_HEIGHT + WALL_OFFSET + y;
        DrawText(text, posX, posY, fontSize, color);
}

void DrawPeekTetraminoBlock(int nextTetramino[TETRA_SHAPE][TETRA_SHAPE])
{
        int offsetX = 16 + WELL_WIDTH + WALL_OFFSET;
        float y = WALL_OFFSET + 80;
        const float cell_size = CELL_SIZE;
        DrawRectangle(offsetX, y, TETRA_SHAPE * cell_size, 
                        (TETRA_SHAPE + 1) * cell_size, BLACK);
        DrawTextOnRPanel("Next", 52, y + 6, 24, RAYWHITE);

        for(int row = 0; row < TETRA_SHAPE; row++) {
                for (int col = 0; col < TETRA_SHAPE; col++) {
                        float cellX = col * cell_size + offsetX;
                        float cellY = row * cell_size + (y + cell_size);
                        switch(nextTetramino[row][col]) {
                                case 1:
                                        DrawCell(cellX, cellY, BLUE);
                                        break;
                        }
                }
        }
}

void DrawScore(int score)
{
        float x = 20;
        float y = 258;
        char scoreText[5] = {0};
        sprintf(scoreText, "%04d", score);
        DrawTextOnRPanel("Score:", x, y, 32, BLACK);
        DrawTextOnRPanel(scoreText, x, y + 36, 32, BLACK);
}

void DrawLevel(int level)
{
        float x = 20;
        float y = 346;
        char levelText[4] = {0};
        sprintf(levelText, "%03d", level);
        DrawTextOnRPanel("Level:", x, y, 32, BLACK);
        DrawTextOnRPanel(levelText, x, y + 36, 32, BLACK);
}

void DrawCommands() 
{
        float x = 10;
        float y = 420;
        DrawTextOnRPanel("<space> - Rotate", x, y, 20, BLACK);
        y += 22;
        DrawTextOnRPanel("Arrow keys - Move", x, y, 20, BLACK);
}

void DrawCredits()
{
        float x = 20;
        float y = 12;

        DrawTextOnDPanel("by mellowboyxd", x, y, 16, BLACK);
}

void DrawTetramino(Tetramino tetramino)
{
        float x = tetramino.x + WALL_OFFSET;
        float y = tetramino.y + WALL_OFFSET;

        for (int i = 0; i < TETRA_SHAPE; i++) {
                for (int j = 0; j < TETRA_SHAPE; j++) {
                        float cellX = j * CELL_SIZE + x;
                        float cellY = i * CELL_SIZE + y;

                        /* skip drawing if not inside of well. */
                        if (cellX > WELL_WIDTH || cellX < 0 
                                || cellY > WELL_HEIGHT || cellY < 0) {
                                continue;
                        }

                        if (tetramino.shape[i][j] != 0) {
                                DrawCell(cellX, cellY, tetramino.color);
                        }
                }
        }
}

/*
 * ===========================
 */
