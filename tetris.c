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

/* CELL_NONE also acts as a sentinel value signifying the end of the list of 
 * colors */
typedef enum CellColor {
        CELL_BLUE, 
        CELL_RED, 
        CELL_YELLOW, 
        CELL_GREEN, 
        CELL_GRAY,
        CELL_VIOLET,
        CELL_NONE
} CellColor;

typedef enum Direction {NONE, LEFT, RIGHT, UP, DOWN} Direction;

typedef enum GameState {MENU_STATE, PLAY_STATE, LOSE_STATE} GameState;

typedef struct Cell {
        Vector2 pos;
        Color color;
} Cell;

typedef struct TetraShape {
        int cells[TETRA_SHAPE][TETRA_SHAPE];
        CellColor cellColor;
} TetraShape;

typedef struct Tetramino {
        float x;
        float y;
        TetraShape shape;
} Tetramino;

typedef struct Text {
        int fontSize;
        Color fontColor;
        const char *cstr;
} Text;

typedef struct Button {
        Vector2 pos;
        float width;
        float height;
        int borderWidth;
        Color borderColor;
        Color defaultColor;
        Color color;
        Color hoverColor;
        Color activeColor;
        int padLeft;
        void (*HandleOnClickEvent) (void);
        Text text;
} Button;

void DrawCell(float x, float y, Color color);
void DrawPanels();
void DrawTextOnRPanel(const char *text, int x, int y, int fontSize, 
                Color color);
void DrawTextOnDPanel(const char *text, int x, int y, int fontSize, 
                Color color);
void DrawPeekTetraminoBlock(TetraShape nextTetramino);
void DrawScore(int score);
void DrawLevel(int level);
void DrawCommands();
void DrawCredits();
void DrawUI();
void DrawWell(CellColor well[PLAY_HEIGHT][PLAY_WIDTH]);
void DrawTetramino(Tetramino tetramino);
void DrawGameOver(Button restartBtn);
void DrawButton(Button btn);

bool isThereYCollision(Tetramino tetra, 
                CellColor well[PLAY_HEIGHT][PLAY_WIDTH]);
bool isThereXCollision(Tetramino tetra, CellColor well[PLAY_HEIGHT][PLAY_WIDTH], 
                Direction dir);

void MarkWell(Tetramino tetra, CellColor well[PLAY_HEIGHT][PLAY_WIDTH]);
void ResetTetraminoPos(Tetramino *current);
void GenerateRandomTetramino(TetraShape *next);
void MoveTetramino(Tetramino *tetra, CellColor well[PLAY_HEIGHT][PLAY_WIDTH]);
void HandleInput(GameState gameState);
void RotateTetraminoShape(int currentShape[TETRA_SHAPE][TETRA_SHAPE], 
                bool clockwise);
void CopyMatrix(int m, int n, int old[m][n], int new[m][n]);
void CheckWinningCondition(CellColor well[PLAY_HEIGHT][PLAY_WIDTH], int *score);
void CheckLosingCondition(CellColor well[PLAY_HEIGHT][PLAY_WIDTH], 
                GameState *gameState);
int GetWinningRow(CellColor well[PLAY_HEIGHT][PLAY_WIDTH]);
void ClearRow(int row, CellColor well[PLAY_HEIGHT][PLAY_WIDTH]);
void CopyRows(int n, int old[n], int new[n]);
void ShiftDownRows(int startRow, CellColor well[PLAY_HEIGHT][PLAY_WIDTH]);
void IncrementScore(int *score);
CellColor GetRandomCellColor();
Color CellColorToColor(CellColor cellColor);
void InitWell(CellColor well[PLAY_HEIGHT][PLAY_WIDTH]);
void SetupGame();

void MainDraw(CellColor well[PLAY_HEIGHT][PLAY_WIDTH]);
void MainUpdate();
void UpdatePlayGame();
void UpdateButton(Button *btn);
void UpdateAudio(Music *music, bool isPaused);

TetraShape nextTetramino = {
        .cells = {{0}},
        .cellColor = CELL_RED
};

Tetramino currentTetramino = {
        .x = CELL_SIZE * 3,
        .y = DEFAULT_Y,
        .shape = {
             .cells = {{0}},
             .cellColor = CELL_BLUE
        }
};

CellColor well[PLAY_HEIGHT][PLAY_WIDTH] = {0};

int score = 0;
int level = 1;

GameState gameState = LOSE_STATE;

Button restartBtn = {
        .pos = {
                .x = 348,
                .y = 195
        },
        .padLeft = 18,
        .width = 160,
        .height = 50,
        .borderWidth = 2,
        .borderColor = BLACK,
        .defaultColor = GRAY,
        .color = GRAY,
        .hoverColor = BLUE,
        .activeColor = RED,
        .text = {
                .cstr = "Restart",
                .fontSize = 30,
                .fontColor = BLACK
        },
        .HandleOnClickEvent = SetupGame,
};

int main(void)
{
        InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Tetris Game");

        InitAudioDevice();

        SetTargetFPS(TARGET_FPS);

        Music bgMusic = LoadMusicStream("assets/audio/stardust_renderer.mp3");
        float volume = 0.8f;
        bool isPaused = false;

        SetMusicVolume(bgMusic, volume);

        PlayMusicStream(bgMusic);

        SetupGame();

        while(!WindowShouldClose()) {
                BeginDrawing();
                ClearBackground(GRAY);
                MainDraw(well);
                MainUpdate();
                UpdateAudio(&bgMusic, isPaused);
                EndDrawing();
        }

        CloseAudioDevice();

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

        HandleInput(gameState);

        if (frameCounter >= UPDATE_DELAY) {
                if (gameState == PLAY_STATE) {
                        UpdatePlayGame();
                }
                frameCounter = 0;
        }
}

void UpdateAudio(Music *music, bool isPaused)
{
        UpdateMusicStream(*music);

        if (isPaused) PauseMusicStream(*music);
        else ResumeMusicStream(*music);
}

void UpdateButton(Button *btn)
{
        Vector2 mousePos = GetMousePosition();
        int right = btn->pos.x;
        int left = right + btn->width;
        int top = btn->pos.y;
        int bottom = top + btn->height;

        if (mousePos.x >= right && mousePos.x <= left 
                        && mousePos.y >= top && mousePos.y <= bottom) {

                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                        btn->HandleOnClickEvent();

                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                        btn->color = btn->activeColor;
                } else
                        btn->color = btn->hoverColor;
        } else {
                btn->color = btn->defaultColor;
        }
}

void UpdatePlayGame() {
        if (!isThereYCollision(currentTetramino, well))
                currentTetramino.y += CELL_SIZE;
        else {
                // update well
                MarkWell(currentTetramino, well);

                ResetTetraminoPos(&currentTetramino);
                currentTetramino.shape = nextTetramino;

                CheckWinningCondition(well, &score);
                CheckLosingCondition(well, &gameState);

                GenerateRandomTetramino(&nextTetramino);
        }
}

/*
 * ============================
 */

/*
 * ===== HELPER FUNCTIONS =====
 */

void SetupGame()
{
        score = 0;
        level = 0;
        InitWell(well);
        GenerateRandomTetramino(&currentTetramino.shape);
        ResetTetraminoPos(&currentTetramino);
        GenerateRandomTetramino(&nextTetramino);
        gameState = PLAY_STATE;
}

void InitWell(CellColor well[PLAY_HEIGHT][PLAY_WIDTH])
{
        for (int i = 0; i < PLAY_HEIGHT; i++) {
                for (int j = 0; j < PLAY_WIDTH; j++) {
                        well[i][j] = CELL_NONE;
                }
        }
}

CellColor GetRandomCellColor()
{
        srand(time(NULL));
        static int prev = -1;

        int color;
        do {
                color = rand() % CELL_NONE;
        } while (color == prev);
        prev = color;

        return (CellColor) color;
}

bool isThereYCollision(Tetramino tetra, CellColor well[PLAY_HEIGHT][PLAY_WIDTH])
{
        for (int i = 0; i < TETRA_SHAPE; i++) {
                for (int j = 0; j < TETRA_SHAPE; j++) {
                        float x = j * CELL_SIZE + tetra.x;
                        float y = i * CELL_SIZE + tetra.y;
                        if (tetra.shape.cells[i][j] == 1) {
                                int row = y / CELL_SIZE;
                                int col = x / CELL_SIZE;
                                if(row + 1 >= PLAY_HEIGHT)
                                        return true;
                                if (row >= 0 && 
                                        well[row + 1][col] != CELL_NONE) {
                                        return true;
                                }
                        }
                }
        }
        return false;
}

bool isThereXCollision(Tetramino tetra, CellColor well[PLAY_HEIGHT][PLAY_WIDTH], 
                Direction dir)
{
        for (int i = 0; i < TETRA_SHAPE; i++) {
                for (int j = 0; j < TETRA_SHAPE; j++) {
                        float x = j * CELL_SIZE + tetra.x;
                        float y = i * CELL_SIZE + tetra.y;
                        if (tetra.shape.cells[i][j] == 1) {
                                int row = y / CELL_SIZE;
                                int col = x / CELL_SIZE;

                                if (dir == LEFT)
                                        col -= 1;
                                else if (dir == RIGHT)
                                        col += 1;

                                if (col < 0 || col >= PLAY_WIDTH)
                                        return true;

                                if (row >= 0 && well[row][col] != CELL_NONE) {
                                        return true;
                                }
                        }
                }
        }
        return false;
}

void MarkWell(Tetramino tetra, CellColor well[PLAY_HEIGHT][PLAY_WIDTH])
{
        for (int i = 0; i < TETRA_SHAPE; i++) {
                for (int j = 0; j < TETRA_SHAPE; j++) {
                        float x = j * CELL_SIZE + tetra.x;
                        float y = i * CELL_SIZE + tetra.y;
                        if (tetra.shape.cells[i][j] != 0) {
                                int row = y / CELL_SIZE;
                                int col = x / CELL_SIZE;
                                well[row][col] = tetra.shape.cellColor;
                        }
                }
        }
}

void ResetTetraminoPos(Tetramino *current)
{
        current->x = CELL_SIZE * 3;
        current->y = DEFAULT_Y;
}

void GenerateRandomTetramino(TetraShape *next)
{
        srand(time(NULL));
        static int prev = -1;
        int type;
        do {
                type = (rand() % 6) + 1;
        } while (type == prev);
        prev = type;

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

        CopyMatrix(TETRA_SHAPE, TETRA_SHAPE, *nextType, next->cells);
        next->cellColor = GetRandomCellColor();
}

void HandleInput(GameState gameState)
{
        if (gameState == PLAY_STATE) {
                MoveTetramino(&currentTetramino, well);
        }

        UpdateButton(&restartBtn);
}

void MoveTetramino(Tetramino *tetra, CellColor well[PLAY_HEIGHT][PLAY_WIDTH])
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
                        CopyMatrix(TETRA_SHAPE, TETRA_SHAPE, tetra->shape.cells, 
                                        oldShape);
                        RotateTetraminoShape(tetra->shape.cells, true);
                        if (isThereYCollision(*tetra, well) || 
                                isThereXCollision(*tetra, well, NONE)) {
                                // reset
                                CopyMatrix(TETRA_SHAPE, TETRA_SHAPE, oldShape, 
                                                tetra->shape.cells);
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

void CheckWinningCondition(CellColor well[PLAY_HEIGHT][PLAY_WIDTH], int *score)
{
        int winningRow;
        while ((winningRow = GetWinningRow(well)) != -1) {
                ClearRow(winningRow, well);
                ShiftDownRows(winningRow, well);
                IncrementScore(score);
        }
}

void CheckLosingCondition(CellColor well[PLAY_HEIGHT][PLAY_WIDTH], 
                GameState *gameState)
{
        int start = PLAY_WIDTH / 2 - TETRA_SHAPE / 2;

        for (int i = start; i < (start + TETRA_SHAPE); i++) {
                if (well[0][i] != CELL_NONE) {
                        *gameState = LOSE_STATE;
                        return;
                }
        }
}

void ClearRow(int row, CellColor well[PLAY_HEIGHT][PLAY_WIDTH])
{
        for (int j = 0; j < PLAY_WIDTH; j++) {
                well[row][j] = CELL_NONE;
        }
}

void ShiftDownRows(int startRow, CellColor well[PLAY_HEIGHT][PLAY_WIDTH])
{
        for (int i = startRow; i > 1; i--) {
                CopyRows(PLAY_WIDTH, (int *)well[i - 1], (int *)well[i]);
        }
}

/* returns the first winning row from bottom to top */
int GetWinningRow(CellColor well[PLAY_HEIGHT][PLAY_WIDTH])
{
        for (int i = PLAY_HEIGHT - 1; i >= 0; i--) {
                bool isComplete = true;
                for (int j = 0; j < PLAY_WIDTH; j++) {
                        if (well[i][j] == CELL_NONE) {
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

void MainDraw(CellColor well[PLAY_HEIGHT][PLAY_WIDTH])
{
        DrawUI();
        DrawWell(well);
        DrawTetramino(currentTetramino);

}

void DrawWell(CellColor well[PLAY_HEIGHT][PLAY_WIDTH])
{
        for (int row = 0; row < PLAY_HEIGHT; row++) {
                for (int col = 0; col < PLAY_WIDTH; col++) {
                        float x = col * CELL_SIZE + WALL_OFFSET;
                        float y = row * CELL_SIZE + WALL_OFFSET;

                        if (well[row][col] == CELL_NONE) {
                                DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, 
                                                BLACK);
                        } else {
                                Color color = CellColorToColor(well[row][col]);
                                DrawCell(x, y, color);
                        }
                }
        }
}

void DrawButton(Button btn)
{
        int x = btn.pos.x;
        int y = btn.pos.y; 
        int w = btn.width;
        int h = btn.height;
        DrawRectangle(x, y, w, h, btn.borderColor);

        x += btn.borderWidth;
        y += btn.borderWidth;
        w -= btn.borderWidth * 2;
        h -= btn.borderWidth * 2;
        DrawRectangle(x, y, w, h, btn.color);

        int cy = (h / 2 + y) - (btn.text.fontSize / 2);
        DrawText(btn.text.cstr, x + btn.padLeft, cy, btn.text.fontSize, 
                        btn.text.fontColor);
}

void DrawGameOver(Button restartBtn)
{
        DrawTextOnRPanel("GAME OVER", 20, 128, 30, BLACK);
        DrawButton(restartBtn);
}

void DrawUI()
{
        DrawTextOnRPanel("Tetris", 20, 20, 48, BLACK);
        if (gameState == PLAY_STATE) {
                DrawPeekTetraminoBlock(nextTetramino);
        } else if (gameState == LOSE_STATE) {
                DrawGameOver(restartBtn);
        }
        DrawScore(score);
        DrawLevel(level);
        DrawCommands();
        DrawCredits();
}

void DrawCell(float x, float y, Color color)
{
        const int borderWidth = 2;
        DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, RAYWHITE);
        DrawRectangle(
                        x + borderWidth, 
                        y + borderWidth, 
                        CELL_SIZE - borderWidth * 2, 
                        CELL_SIZE - borderWidth * 2, 
                        color
                     );
}

Color CellColorToColor(CellColor cellColor)
{
        switch(cellColor) {
                case CELL_NONE:
                        return BLACK;
                case CELL_RED:
                        return RED;
                case CELL_BLUE:
                        return BLUE;
                case CELL_YELLOW:
                        return YELLOW;
                case CELL_GREEN:
                        return GREEN;
                case CELL_GRAY:
                        return GRAY;
                case CELL_VIOLET:
                        return VIOLET;
                default:
                        fprintf(stderr, "[ERROR]: Unknown cell color\n");
                        return BLACK;
        }
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

void DrawPeekTetraminoBlock(TetraShape next)
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
                        if (next.cells[row][col] != 0) {
                                Color color = CellColorToColor(next.cellColor);
                                DrawCell(cellX, cellY, color);
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

        DrawTextOnDPanel("by mellowboyxd", x, y, 18, BLACK);
}

void DrawTetramino(Tetramino tetra)
{
        float x = tetra.x + WALL_OFFSET;
        float y = tetra.y + WALL_OFFSET;

        for (int i = 0; i < TETRA_SHAPE; i++) {
                for (int j = 0; j < TETRA_SHAPE; j++) {
                        float cellX = j * CELL_SIZE + x;
                        float cellY = i * CELL_SIZE + y;

                        /* skip drawing if not inside of well. */
                        if (cellX > WELL_WIDTH || cellX < 0 
                                || cellY > WELL_HEIGHT || cellY < 0) {
                                continue;
                        }

                        if (tetra.shape.cells[i][j] != 0) {
                                CellColor cellColor = tetra.shape.cellColor;
                                Color color = CellColorToColor(cellColor);
                                DrawCell(cellX, cellY, color);
                        }
                }
        }
}

/*
 * ===========================
 */
