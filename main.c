#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include "audio.h"
#include "path.h"


#define MAX_SNAKE_LENGTH 2000

AudioData* bgMusicData;

struct Point {
    int x;
    int y;
};

struct Snake {
    struct Point head;
    struct Point body[MAX_SNAKE_LENGTH];
    int dirX;
    int dirY;
    int length;
};

int randInt(int min, int max) {
    srand(time(NULL));

    int randomNumber = (rand() % (max - min + 1)) + min;

    return randomNumber;
}

const char WALL[] = "\u2B1C";
const char SNAKE[] = "\u2B1B";
const char FOOD[] = "\033[31m\u2BC1 \033[37m";
const char SPACE[] = "  ";
const char NEW_LINE[] = "\n\r";

int width = 40;
int height = 20;
const int ADDITIONAL_HEIGHT = 2;
const int GAME_BASE_SPEED_MS = 800;

bool isPlaying = false;
bool isGameOver = false;

int renderCount = 0;
bool shouldStop = false;

bool confirmEscape = false;

bool difficultyChoosen = false;
int difficulty = 1;

bool gridSizeChoosen = false;
int gridSize = 1;

int gameSpeedSubs = 0;
int minSpeed = 400;

int score = 0;

struct Snake snake;
struct Point food;

int screen_w, screen_h;
int offset_x;
int offset_y;
bool tick = false;

unsigned long long getMilliseconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000ULL + tv.tv_usec / 1000;
}

unsigned long long startTime;
unsigned long long endTime;

int clamp(int value, int min, int max) {
    if (value < min) {
        return min;
    } else if (value > max) {
        return max;
    } else {
        return value;
    }
}

bool getShouldContinue() {
    char ans;

    while(true) {
        printf("\n\r\033[36;1;4mDo you want try again? y/n:\033[0m ");
        scanf(" %c", &ans);

        if(tolower(ans) == 'n') {
            return false;
        } else if(tolower(ans) == 'y') {
            return true;
        }
    }
}

void getScreenOffset() {
    refresh();
    getmaxyx(stdscr, screen_h, screen_w);
    offset_x = (screen_w - (width * 2)) / 4;
    offset_y = (screen_h - (height + ADDITIONAL_HEIGHT)) / 2;
}

void getElapsedFormattedTime(char* result) {
    unsigned long long elapsedTime = (endTime - startTime) / 1000;
    unsigned long long minutes = elapsedTime / 60;
    unsigned long long seconds = elapsedTime % 60;

    snprintf(
        result,
        sizeof(result),
        "%llum %llus",
        minutes,
        seconds
    );
}

void printOffsetX(int w) {
    for (int k = 0; k < w; k++)
    {
        printf("%s", SPACE);
    }
}

void printOffsetWidth() {
    printOffsetX(offset_x);
}

void printOffsetY(int h) {
    for (int i = 0; i < h; i++)
    {
        printf("%s", NEW_LINE);
    }
}

void printOffsetHeight() {
    printOffsetY(offset_y);
}

void printNL() {
    printf("%s", NEW_LINE);
}

void renderGame(int i, int j) {
    if(
        i == 0 ||
        i == height - 1 ||
        j == 0 ||
        j == width - 1
    ) {

        printf("%s", WALL);
        return;
    }

    for (int k = 0; k < snake.length; k++)
    {
        if(i == snake.body[k].y && j == snake.body[k].x) {
            printf("%s", SNAKE);
            return;
        }
    }

    if(i == food.y && j == food.x) {
        printf("%s", FOOD);
        return;
    }

    printf("%s", SPACE);
}

void renderStats() {
    printNL();
    
    printOffsetWidth();

    char scoreText[20];
    snprintf(
        scoreText,
        sizeof(scoreText),
        "Your score: %d",
        score
    );

    char scorePadded[20];
    snprintf(
        scorePadded,
        sizeof(scorePadded),
        "%s%*c",
        scoreText,
        (int)(sizeof(scorePadded) - strlen(scoreText)),
        ' '
    );

    char formattedTime[10];

    getElapsedFormattedTime(formattedTime);

    char timeText[30];
    snprintf(
        timeText,
        sizeof(timeText),
        "Your time: %s",
        formattedTime
    );

    char timePadded[30];
    snprintf(
        timePadded,
        sizeof(timePadded),
        "%*c%s",
        (int)(sizeof(timePadded) - strlen(timeText) - 1),
        ' ',
        timeText
    );
  
    char offset[(int)((float)width * 2)];

    int desiredOffset = (width * 2 - (sizeof(scoreText) + sizeof(timeText)) + 2);

    for (int i = 0; i < desiredOffset; i++) {
        offset[i] = ' ';
    }

    offset[desiredOffset] = '\0';

    printf("\033[1m%s\033[42m%s\033[1m%s\033[0m%s", scorePadded, offset, timePadded, NEW_LINE);
}

void render() {
    printf("%s", "\033[42m");
    printOffsetHeight();

    for (int i = 0; i < height; i++)
    {
        printOffsetWidth();

        for (int j = 0; j < width; j++)
        {
            renderGame(i, j);
        }

        printNL();
    }

    renderStats();
}

void printToCenter(int size, int offset, const char* format, ...) {
    va_list args;
    va_start(args, format);

    char text[size];

    vsnprintf(text, sizeof(text), format, args);
    int offsetToCenter = ((width - (strlen(text)  / 2)) / 2) - offset;
    printOffsetWidth();
    printOffsetX(offsetToCenter);
    printf("%s", text);
    va_end(args);
}

bool shouldRestart = false;

void stopGame() {
    shouldStop = true;
}

bool checkSelfCollision() {
    for (int i = 1; i < snake.length; i++) {
        if (snake.body[i].x == snake.head.x && snake.body[i].y == snake.head.y) {
            return true;
        }
    }

    return false;
}

void updateSnake() {
    snake.head.x += snake.dirX;
    snake.head.y += snake.dirY;

    if(snake.head.x >= width - 1) {
        snake.head.x = 1;
    }

    if(snake.head.y >= height - 1) {
        snake.head.y = 1;
    }

    if(snake.head.x < 1) {
        snake.head.x = width - 2;
    }

    if(snake.head.y < 1) {
        snake.head.y = height - 2;
    }

    struct Point prevPoint = snake.head;
    struct Point currentPoint;
    for (int i = 0; i < snake.length; i++) {
        currentPoint = snake.body[i];
        snake.body[i] = prevPoint;
        prevPoint = currentPoint;
    }
}

void incSnake() {
    if (snake.length >= MAX_SNAKE_LENGTH - 1) {
        return;
    }

    struct Point newPoint = {
        .x = snake.head.x - snake.dirX,
        .y = snake.head.y - snake.dirY
    };

    for (int i = snake.length; i > 1; i--) {
        snake.body[i] = snake.body[i - 1];
    }

    snake.body[1] = newPoint;

    snake.length++;
}

void initSnake() {
    snake.head.x = width / 2;
    snake.head.y = height / 2;
    snake.length = 0;
    snake.dirX = 1;
    snake.dirY = 0;

    for (int i = 0; i < 3; i++) {
        snake.body[i].x = snake.head.x - i;
        snake.body[i].y = snake.head.y;
    }

    snake.length = 3;
}

bool chekIsFoodCollidesWithSnake(int x, int y) {
    for (int i = 0; i < snake.length; i++)
    {
        if(snake.body[i].x == x && snake.body[i].y == y) {
            return true;
        }
    }
    
    return false;
}

bool DoesBoardHaveFreeSpace() {
    return snake.length < (width - 2) * (height - 2);
}

void getFreeSpace(int *x, int *y) {
    for (int i = 1; i < height - 1; i++)
    {
        for (int j = 1; j < width - 1; j++)
        {
            bool isOccupied = false;

            for (int k = 0; k < snake.length; k++)
            {
                if(snake.body[k].x == j && snake.body[k].y == i) {
                    isOccupied = true;
                    break;
                }
            }

            if(!isOccupied) {
                *x = j;
                *y = i;
                return;
            }
        }
    }
}

void updateFoodPos() {
    const int MAX_ITERATION = 20;
    int x, y, i = 0;

    do
    {
        i++;

        if(i > MAX_ITERATION) {
            if(DoesBoardHaveFreeSpace()) {
                getFreeSpace(&x, &y);
            }
            
            break;
        }

        x = randInt(1, width - 2);
        y = randInt(1, height - 2);
    } while (chekIsFoodCollidesWithSnake(x, y));
    
    food.x = x;
    food.y = y;
}

void initFood() {
    updateFoodPos();
}

void initGame() {
    startTime = getMilliseconds();
    endTime = getMilliseconds();

    score = 0;
    gameSpeedSubs = 0;
    isGameOver = false;
    isPlaying = true;

    switch (difficulty)
    {
    case 1:
        minSpeed = 600;
        break;

    case 2:
        minSpeed = 300;
        gameSpeedSubs = 100;
        break;

    case 3:
        minSpeed = 100;
        gameSpeedSubs = 200;
        break;
    
    default:
        minSpeed = 300;
        break;
    }

    difficultyChoosen = false;

    switch (gridSize)
    {
    case 1:
        width = 30;
        height = 10;
        break;

    case 2:
        width = 40;
        height = 20;
        break;

    case 3:
        width = 60;
        height = 30;
        break;
    
    default:
        width = 40;
        height = 20;
        break;
    }

    gridSizeChoosen = false;

    initSnake();
    initFood();
}

void getSnakeInput() {
    if(!tick) return;

    int ch = getch();

    if (ch == ERR) return;

    if(ch == KEY_LEFT && snake.dirX != 1) {
        snake.dirX = -1;
        snake.dirY = 0;
    };

    if(ch == KEY_RIGHT && snake.dirX != -1) {
        snake.dirX = 1;
        snake.dirY = 0;
    };

    if(ch == KEY_UP && snake.dirY != 1) {
        snake.dirY = -1;
        snake.dirX = 0;
    };

    if(ch == KEY_DOWN && snake.dirY != -1) {
        snake.dirY = 1;
        snake.dirX = 0;
    };

    if(ch == 27) {
        // Escape key was pressed
        isPlaying = false;
        isGameOver = true;
    }

    tick = false;
}

bool getMainPromptResult() {
    int ch = getch();

    if (ch == ERR) return false;

    if(ch == KEY_LEFT) {
        if (! difficultyChoosen) {
            difficulty = clamp(difficulty - 1, 1, 3);
            return true;
        }

        if (! gridSizeChoosen) {
            gridSize = clamp(gridSize - 1, 1, 3);
            return true;
        }
    };

    if(ch == KEY_RIGHT) {
        if (! difficultyChoosen) {
            difficulty = clamp(difficulty + 1, 1, 3);
            return true;
        }

        if (! gridSizeChoosen) {
            gridSize = clamp(gridSize + 1, 1, 3);
            return true;
        }
    };

    if(ch == '\n') {
        if(confirmEscape) {
            confirmEscape = false;
            return true;
        }

        if (! difficultyChoosen) {
            difficultyChoosen = true;
            return true;
        }

        if (! gridSizeChoosen) {
            gridSizeChoosen = true;
            return true;
        }
    }

    if(ch == 27) {
        // Escape key was pressed
        if(gridSizeChoosen) {
            gridSizeChoosen = false;
            return true;
        }

        if(difficultyChoosen) {
            difficultyChoosen = false;
            return true;
        }

        if(confirmEscape) {
            stopGame();
            return true;
        }

        confirmEscape = true;

    }

    if(ch == '\n') {
        initGame();
    }

    return true;
}

bool checkDoesSnakeAteFood() {
    if(food.x == snake.head.x && food.y == snake.head.y) {
        return true;
    }

    return false;
}

void restartGame() {
    gridSizeChoosen = false;
    difficultyChoosen = false;
    isGameOver = false;
    isPlaying = false;
}

bool getGameOverInput() {
    int ch = getch();

    if (ch == ERR) return false;

    if(ch == KEY_LEFT) shouldRestart = true;
    if(ch == KEY_RIGHT) shouldRestart = false;

    if(ch == '\n') {
        if(shouldRestart) {
            restartGame();
        } else {
            stopGame();
        }
    }
    return true;
}

void renderGameOver() {
    printf("%s", "\033[42;1m");
    printOffsetHeight();

    printNL();
    printToCenter(30, -3, "\033[41;1m GAME OVER! \033[42;1m");
    printNL();
    printNL();

    printNL();
    printToCenter(20, 1, "Your score: %d", score);
    printNL();

    char time[10];
    getElapsedFormattedTime(time);
    printNL();
    printToCenter(20, 0, "Your time: %s ", time);
    printNL();

    char message[70];

    if (shouldRestart) {
        snprintf(
            message,
            sizeof(message),
            "\033[44;1;37m Do you want to play again? \033[4m y \033[0m\033[44;1;37m/ n \033[42;0m"
        );
    } else {
        snprintf(
            message,
            sizeof(message),
            "\033[44;1;37m Do you want to play again?  y /\033[44;4;37m n \033[0m\033[44;1;37m\033[42;0m"
        );
    }

    printNL();
    printNL();
    printToCenter(80, -8, message, time);
    printNL();
}

void printGoodBye() {
    printf("%s", "\033[42;37m");
    printOffsetHeight();
    printOffsetY(height / 2);

    printNL();
    printToCenter(35, -3, "\033[1m GOOD BYE :) \033[0m");
    printNL();
    printNL();
}

void printGreeting() {
    const char text[] = "Snake Game :)";

    char toPrint[sizeof(text) * 2];
    toPrint[0] = '\0';
    int counter = 0;

    while(true) {
        clear();
        refresh();
        getScreenOffset();
        printf("%s", "\033[42;1m");
        printNL();
        printOffsetHeight();
        printOffsetY(height / 2);


        printNL();
        printToCenter(sizeof(toPrint), -1, " %s ", toPrint);
        printNL();

        for (int i = 0; i < counter; i++)
        {
            toPrint[i] = text[i];
        }

        toPrint[counter] = '\0';

        counter++;
        
        usleep(400000);
        if(counter >= sizeof(text) + 1) {
            char absolute_path[PATH_MAX];
            relToAbsPath("sounds/coin.wav", absolute_path, sizeof(absolute_path));

            playSoundAsyncOnce(absolute_path);
            usleep(2000000);
            break;
        }
    }

}

void chooseTheDifficulty() {
    printToCenter(23, 0, "Choose the difficulty:");
    printNL();
    printNL();

    switch (difficulty)
    {
    case 1:
        printToCenter(50, -3, "\033[42;4m easy \033[0m\033[42;1m medium  hard");
        break;

    case 2:
        printToCenter(50, -3, " easy \033[42;4m medium \033[0m\033[42;1m hard");
        break;

    case 3:
        printToCenter(50, -3, " easy  medium \033[42;4m hard \033[0m\033[42m");
        break;
    
    default:
        printToCenter(19, 0, "easy  medium  hard");
        break;
    }
    printNL();
}

void chooseTheGridSize() {
    printOffsetY(4);
    printToCenter(22, 1, "Choose the grid size:");
    printOffsetY(2);


    switch (gridSize)
    {
    case 1:
        printToCenter(50, -4, "\033[42;4m small \033[0m\033[42;1m medium  large");
        break;

    case 2:
        printToCenter(50, -4, " small \033[42;4m medium \033[0m\033[42;1m large");
        break;

    case 3:
        printToCenter(50, -4, " small  medium \033[42;4m large \033[0m\033[42m");
        break;
    
    default:
        printToCenter(21, 0, "small  medium  large");
        break;
    }
    printNL();
}

void renderMainPrompt() {
    printNL();
    printOffsetHeight();
    printNL();
    printf("%s", "\033[42;1m");

    if(confirmEscape) {
        printNL();
        printToCenter(55, 0, "Press again ESC to exit or press ENTER to cancel exit!");
        printNL();
        return;
    }

    chooseTheDifficulty();


    if(difficultyChoosen) {
        printf("%s", "\033[42;1m");
        chooseTheGridSize();
    }

    if(difficultyChoosen && gridSizeChoosen) {
        printf("%s", "\033[42;1m");
        printNL();
        printNL();
        printToCenter(55, 0, "Press ENTER to start or ESC to choose another options!");
        printNL();
    }
}

void gameOver() {
    isPlaying = false;
    isGameOver = true;
    renderCount = 0;
}

void update() {
    endTime = getMilliseconds();
    updateSnake();

    if(checkDoesSnakeAteFood()) {
        score++;
        updateFoodPos();
        incSnake();

        char absolute_path[PATH_MAX];
        relToAbsPath("sounds/food.wav", absolute_path, sizeof(absolute_path));
        playSoundAsyncOnce(absolute_path);

        if (gameSpeedSubs <= GAME_BASE_SPEED_MS) {
            gameSpeedSubs += 25;
        }
    }

    if(checkSelfCollision()) {
        char absolute_path[PATH_MAX];
        relToAbsPath("sounds/hit.wav", absolute_path, sizeof(absolute_path));
        playSoundAsyncOnce(absolute_path);
        gameOver();
    }
}

void initTerminal() {
    initscr();
    cbreak();
    noecho();
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_GREEN);
    bkgd(COLOR_PAIR(1));
    curs_set(0);
    nodelay(stdscr, TRUE); 
    keypad(stdscr, TRUE);
}

void cleanupTerminal() {
    endwin();
}

int main() {
    char *dir = getCurrentDir();
    sprintf(currentDir, "%s", dir);

    if(initAudio()) {
        return 1;
    }

    initTerminal();

    clear();
    refresh();

    unsigned long long lastTick = getMilliseconds();

    printGreeting();

    int currentState = 0;

    while (true)
    {
        if(isGameOver && !isPlaying) {
            gameSpeedSubs = 800;

            bool res = getGameOverInput();

            if(res) {
                renderCount = 0;
            }

            if(!res && renderCount > 10)
                continue;
        }

        if(!isGameOver && isPlaying) {
           getSnakeInput();
        }

        if(!isPlaying && !isGameOver) {
            gameSpeedSubs = 800;

            bool res = getMainPromptResult();

            if(res) {
                renderCount = 0;
            }

            if(!res && renderCount > 10)
                continue;
        }

        if(shouldStop) {
            clear();
            getScreenOffset();
            if(bgMusicData != NULL)
                stopAudio(bgMusicData);

            char absolute_path[PATH_MAX];
            relToAbsPath("sounds/coin.wav", absolute_path, sizeof(absolute_path));

            playSoundAsyncOnce(absolute_path);

            printGoodBye();
            usleep(1000000);
            break;
        }

        unsigned long long currentTick = getMilliseconds();
        unsigned long long elapsed = currentTick - lastTick;
        
        if (!(elapsed >= clamp(GAME_BASE_SPEED_MS - gameSpeedSubs, minSpeed, 1000))) {
            continue;
        }

        lastTick = currentTick;
        
        if(renderCount < 100) renderCount++;

        clear();
        refresh();
        getScreenOffset();
        
        if(isPlaying && !isGameOver) {
            if (currentState != 2) {
                if(bgMusicData != NULL)
                    stopAudio(bgMusicData);
                
                char absolute_path[PATH_MAX];
                relToAbsPath("musics/game.mp3", absolute_path, sizeof(absolute_path));
                playBackgroundMusicAsync(absolute_path, 64, &bgMusicData);
                currentState = 2;
            }

            update();
            render();
            tick = true;
        }

        if(isGameOver && !isPlaying) {
            if (currentState != 3) {
                if(bgMusicData != NULL)
                    stopAudio(bgMusicData);
                
                char absolute_path[PATH_MAX];
                relToAbsPath("musics/menu.mp3", absolute_path, sizeof(absolute_path));
                playBackgroundMusicAsync(absolute_path, 64, &bgMusicData);
                currentState = 3;
            }

            renderGameOver();
        }

        if(!isPlaying && !isGameOver) {
            if (currentState != 1) {
                if(bgMusicData != NULL)
                    stopAudio(bgMusicData);
                
                char absolute_path[PATH_MAX];
                relToAbsPath("musics/menu.mp3", absolute_path, sizeof(absolute_path));
                playBackgroundMusicAsync(absolute_path, 64, &bgMusicData);
                currentState = 1;
            }

            renderMainPrompt();
        }
        
    }

    if(bgMusicData != NULL)
        stopAudio(bgMusicData);

    cleanupTerminal();
    cleanupAudio();
    return 0;
}