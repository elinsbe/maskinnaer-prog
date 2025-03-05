#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <linux/input.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <poll.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <string.h>

// The game state can be used to detect what happens on the playfield
#define GAMEOVER 0
#define ACTIVE (1 << 0)
#define ROW_CLEAR (1 << 1)
#define TILE_ADDED (1 << 2)

/*
Grid is 8px*8px, and each pix is a 16bit word.
*/
#define GRID_SIZE (8 * 8 * sizeof(u_int16_t))

/*
File path for framebuffer and joystick.
*/
#define FILEPATH "/dev/fb"
#define JOYSTICK_PATH "/dev/input/event"

// If you extend this structure, either avoid pointers or adjust
// the game logic allocate/deallocate and reset the memory

/*
I am giving the tile struct an additional variable, being the color variable.
This is so it can be the same value throughou the game. This does not breach
the requirement, since it doesn't change game logic.
*/
typedef struct
{
    bool occupied;
    int color;
} tile;

typedef struct
{
    unsigned int x;
    unsigned int y;
} coord;

/*
Map will be a pointer to the memory mapped framebuffer. Framebuffer and
joystick are the file descripters for the different devices.
*/
u_int16_t *map;
int framebuffer;
int joystick;

typedef struct
{
    coord const grid;                     // playfield bounds
    unsigned long const uSecTickTime;     // tick rate
    unsigned long const rowsPerLevel;     // speed up after clearing rows
    unsigned long const initNextGameTick; // initial value of nextGameTick

    unsigned int tiles; // number of tiles played
    unsigned int rows;  // number of rows cleared
    unsigned int score; // game score
    unsigned int level; // game level

    tile *rawPlayfield; // pointer to raw memory of the playfield
    tile **playfield;   // This is the play field array
    unsigned int state;
    coord activeTile; // current tile

    unsigned long tick;         // incremeted at tickrate, wraps at nextGameTick
                                // when reached 0, next game state calculated
    unsigned long nextGameTick; // sets when tick is wrapping back to zero
                                // lowers with increasing level, never reaches 0
} gameConfig;

int colors[] = {0x8E60, 0xD8D0, 0x781F, 0x8000};
int conter_color = 0;

gameConfig game = {
    .grid = {8, 8},
    .uSecTickTime = 10000,
    .rowsPerLevel = 2,
    .initNextGameTick = 50,
};

// This function is called on the start of your application
// Here you can initialize what ever you need for your task
// return false if something fails, else true

/*
Will seeif it can open the joysticks path to enable reading the data.
*/
bool initialize_joystick(void)
{
    int found_input = 0;

    for (int i = 0; i < 32; i++)
    {
        char const add = i + '0';
        char path[32] = JOYSTICK_PATH;
        strncat(path, &add, 1);

        joystick = open(path, O_RDONLY);
        if (joystick == -1)
        {
            continue;
        }

        char buf[256];
        int plz = EVIOCGNAME(256);
        int result = ioctl(joystick, plz, buf);
        const char *test = buf;
        if (strcmp(test, "Raspberry Pi Sense HAT Joystick") != 0)
        {
            continue;
        }
        else
        {
            found_input = 1;
            break;
        }
    }

    return (found_input == 1);
}

bool initializeSenseHat()
{
    /*
    this stuct will save info about the device. Will be mainly used to check
    if the sames device corresponds with the wanted device. Will try to open
    the framebuffer using the path. If not found, return.
    */
    struct fb_fix_screeninfo info;
    int found_fb = 0;

    /*
    Since I cannot be sure of the location to the fb or input device I iterate
    though the file path by adding the num to the path. If it is correct then I
    will change found_fb (or input) to one to indicate this. If that is zero,
    then It is not connected. 32 is chosen as 31 fb31 seems to be the last
    framebuffer in the provided documentation
    */
    for (int i = 0; i < 32; i++)
    {
        char const add = i + '0';
        char path[32] = FILEPATH;
        strncat(path, &add, 1);

        framebuffer = open(path, O_RDWR);
        if (framebuffer == -1)
        {

            continue;
        }

        /*
        Uses to get info about the device connected to the framebuffer.
        FBIOGET_FSCREENINFO stores info in the struct which makes it possible to
        check the name. It is important to close file, even if it fails to ensure
        no memory leak.
        */
        if (ioctl(framebuffer, FBIOGET_FSCREENINFO, &info) == -1)
        {
            close(framebuffer);
            continue;
        }

        /*
        Checks if the devices ID matches.
        */
        if (strcmp("RPi-Sense FB", info.id) != 0)
        {
            close(framebuffer);
            continue;
        }
        else
        {
            found_fb = 1;
            break;
        }

        /*
        Here I am mapping the device memory to the virtual memory. NULL is used so
        the kernel chooses the address for mapping.
        */
    }
    if (found_fb == 1)
    {
        map = mmap(NULL, GRID_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, framebuffer, 0);
        if (map == MAP_FAILED)
        {
            close(framebuffer);
            fprintf(stderr, "ERROR: could not map sensehat memory\n");
            return false;
        }
        memset(map, 0, GRID_SIZE);
    }

    return (found_fb == 1);
}

// This function is called when the application exits
// Here you can free up everything that you might have opened/allocated

/*
Here I am unmapping and closing every file that I opened.
*/
void freeSenseHat()
{

    int error = munmap(map, GRID_SIZE);
    if (error != 0)
    {
        fprintf(stderr, "ERROR: could not unmap sensehat\n");
    }
    close(joystick);
    close(framebuffer);
}

// This function should return the key that corresponds to the joystick press
// KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, with the respective direction
// and KEY_ENTER, when the the joystick is pressed
// !!! when nothing was pressed you MUST return 0 !!!
int readSenseHatJoystick()
{
    struct pollfd file = {.fd = joystick, .events = POLLIN};
    /*
    The benefit of using poll is that if there is no input, it will return this
    as opposed to just wait for an input. This would lead to the game only moving
    when the player moved the joystick. 1 means number of file descriptors,
    which is just 1. POLLIN means that there is data to read.
    */
    int result = poll(&file, 1, 0);
    if (result == 0)
    {
        return 0;
    }

    struct input_event event;
    read(joystick, &event, sizeof(event));
    /*
    After reading from the file, the read data will be placed in the input_event
    struct. Here, event.value means which type of event. It is important to
    have this if statement since without it the block would move twice for each
    movement (1 for release and 1 for keypress). 2 is when it is held
    continuously.
    */
    if (event.value == 2 || event.value == 0)
    {
        return event.code;
    }
    return 0;
}

// This function should render the gamefield on the LED matrix. It is called
// every game tick. The parameter playfieldChanged signals whether the game logic
// has changed the playfield
void renderSenseHatMatrix(bool const playfieldChanged)
{

    if (!playfieldChanged)
    {
        return;
    }
    for (int i = 0; i < game.grid.y; i++)
    {
        for (int j = 0; j < game.grid.x; j++)
        {
            if (game.playfield[i][j].occupied)
            {
                /*
                Will use the memory mapped address to ensure that the color
                shows up on sensehat.
                */
                *(map + ((i * game.grid.x) + j)) = game.playfield[i][j].color;
            }
            else
            {
                /*
                used to the tile doesn't leave a trail
                */
                *(map + ((i * game.grid.x) + j)) = 0x0;
            }
        }
    }
}

// The game logic uses only the following functions to interact with the playfield.
// if you choose to change the playfield or the tile structure, you might need to
// adjust this game logic <> playfield interface

static inline void newTile(coord const target)
{
    game.playfield[target.y][target.x].occupied = true;

    /*
    By ading this the tile will have the same color throughout the entire game!
    */
    game.playfield[target.y][target.x].color = colors[conter_color];
    conter_color = (conter_color + 1) % (sizeof(colors) / sizeof(colors[0]));
}

static inline void copyTile(coord const to, coord const from)
{
    memcpy((void *)&game.playfield[to.y][to.x], (void *)&game.playfield[from.y][from.x], sizeof(tile));
}

static inline void copyRow(unsigned int const to, unsigned int const from)
{
    memcpy((void *)&game.playfield[to][0], (void *)&game.playfield[from][0], sizeof(tile) * game.grid.x);
}

static inline void resetTile(coord const target)
{
    memset((void *)&game.playfield[target.y][target.x], 0, sizeof(tile));
}

static inline void resetRow(unsigned int const target)
{
    memset((void *)&game.playfield[target][0], 0, sizeof(tile) * game.grid.x);
}

static inline bool tileOccupied(coord const target)
{
    return game.playfield[target.y][target.x].occupied;
}

static inline bool rowOccupied(unsigned int const target)
{
    for (unsigned int x = 0; x < game.grid.x; x++)
    {
        coord const checkTile = {x, target};
        if (!tileOccupied(checkTile))
        {
            return false;
        }
    }
    return true;
}

static inline void resetPlayfield()
{
    for (unsigned int y = 0; y < game.grid.y; y++)
    {
        resetRow(y);
    }
}

// Below here comes the game logic. Keep in mind: You are not allowed to change how the game works!
// that means no changes are necessary below this line! And if you choose to change something
// keep it compatible with what was provided to you!

bool addNewTile()
{
    game.activeTile.y = 0;
    game.activeTile.x = (game.grid.x - 1) / 2;
    if (tileOccupied(game.activeTile))
        return false;
    newTile(game.activeTile);
    return true;
}

bool moveRight()
{
    coord const newTile = {game.activeTile.x + 1, game.activeTile.y};
    if (game.activeTile.x < (game.grid.x - 1) && !tileOccupied(newTile))
    {
        copyTile(newTile, game.activeTile);
        resetTile(game.activeTile);
        game.activeTile = newTile;
        return true;
    }
    return false;
}

bool moveLeft()
{
    coord const newTile = {game.activeTile.x - 1, game.activeTile.y};
    if (game.activeTile.x > 0 && !tileOccupied(newTile))
    {
        copyTile(newTile, game.activeTile);
        resetTile(game.activeTile);
        game.activeTile = newTile;
        return true;
    }
    return false;
}

bool moveDown()
{
    coord const newTile = {game.activeTile.x, game.activeTile.y + 1};
    if (game.activeTile.y < (game.grid.y - 1) && !tileOccupied(newTile))
    {
        copyTile(newTile, game.activeTile);
        resetTile(game.activeTile);
        game.activeTile = newTile;
        return true;
    }
    return false;
}

bool clearRow()
{
    if (rowOccupied(game.grid.y - 1))
    {
        for (unsigned int y = game.grid.y - 1; y > 0; y--)
        {
            copyRow(y, y - 1);
        }
        resetRow(0);
        return true;
    }
    return false;
}

void advanceLevel()
{
    game.level++;
    switch (game.nextGameTick)
    {
    case 1:
        break;
    case 2 ... 10:
        game.nextGameTick--;
        break;
    case 11 ... 20:
        game.nextGameTick -= 2;
        break;
    default:
        game.nextGameTick -= 10;
    }
}

void newGame()
{
    game.state = ACTIVE;
    game.tiles = 0;
    game.rows = 0;
    game.score = 0;
    game.tick = 0;
    game.level = 0;
    resetPlayfield();
}

void gameOver()
{
    game.state = GAMEOVER;
    game.nextGameTick = game.initNextGameTick;
}

bool sTetris(int const key)
{
    bool playfieldChanged = false;

    if (game.state & ACTIVE)
    {
        // Move the current tile
        if (key)
        {
            playfieldChanged = true;
            switch (key)
            {
            case KEY_LEFT:
                moveLeft();
                break;
            case KEY_RIGHT:
                moveRight();
                break;
            case KEY_DOWN:
                while (moveDown())
                {
                };
                game.tick = 0;
                break;
            default:
                playfieldChanged = false;
            }
        }

        // If we have reached a tick to update the game
        if (game.tick == 0)
        {
            // We communicate the row clear and tile add over the game state
            // clear these bits if they were set before
            game.state &= ~(ROW_CLEAR | TILE_ADDED);

            playfieldChanged = true;
            // Clear row if possible
            if (clearRow())
            {
                game.state |= ROW_CLEAR;
                game.rows++;
                game.score += game.level + 1;
                if ((game.rows % game.rowsPerLevel) == 0)
                {
                    advanceLevel();
                }
            }

            // if there is no current tile or we cannot move it down,
            // add a new one. If not possible, game over.
            if (!tileOccupied(game.activeTile) || !moveDown())
            {
                if (addNewTile())
                {
                    game.state |= TILE_ADDED;
                    game.tiles++;
                }
                else
                {
                    gameOver();
                }
            }
        }
    }

    // Press any key to start a new game
    if ((game.state == GAMEOVER) && key)
    {
        playfieldChanged = true;
        newGame();
        addNewTile();
        game.state |= TILE_ADDED;
        game.tiles++;
    }

    return playfieldChanged;
}

int readKeyboard()
{
    struct pollfd pollStdin = {
        .fd = STDIN_FILENO,
        .events = POLLIN};
    int lkey = 0;

    if (poll(&pollStdin, 1, 0))
    {
        lkey = fgetc(stdin);
        if (lkey != 27)
            goto exit;
        lkey = fgetc(stdin);
        if (lkey != 91)
            goto exit;
        lkey = fgetc(stdin);
    }
exit:
    switch (lkey)
    {
    case 10:
        return KEY_ENTER;
    case 65:
        return KEY_UP;
    case 66:
        return KEY_DOWN;
    case 67:
        return KEY_RIGHT;
    case 68:
        return KEY_LEFT;
    }
    return 0;
}

void renderConsole(bool const playfieldChanged)
{
    if (!playfieldChanged)
        return;

    // Goto beginning of console
    fprintf(stdout, "\033[%d;%dH", 0, 0);
    for (unsigned int x = 0; x < game.grid.x + 2; x++)
    {
        fprintf(stdout, "-");
    }
    fprintf(stdout, "\n");
    for (unsigned int y = 0; y < game.grid.y; y++)
    {
        fprintf(stdout, "|");
        for (unsigned int x = 0; x < game.grid.x; x++)
        {
            coord const checkTile = {x, y};
            fprintf(stdout, "%c", (tileOccupied(checkTile)) ? '#' : ' ');
        }
        switch (y)
        {
        case 0:
            fprintf(stdout, "| Tiles: %10u\n", game.tiles);
            break;
        case 1:
            fprintf(stdout, "| Rows:  %10u\n", game.rows);
            break;
        case 2:
            fprintf(stdout, "| Score: %10u\n", game.score);
            break;
        case 4:
            fprintf(stdout, "| Level: %10u\n", game.level);
            break;
        case 7:
            fprintf(stdout, "| %17s\n", (game.state == GAMEOVER) ? "Game Over" : "");
            break;
        default:
            fprintf(stdout, "|\n");
        }
    }
    for (unsigned int x = 0; x < game.grid.x + 2; x++)
    {
        fprintf(stdout, "-");
    }
    fflush(stdout);
}

inline unsigned long uSecFromTimespec(struct timespec const ts)
{
    return ((ts.tv_sec * 1000000) + (ts.tv_nsec / 1000));
}

void check_color(void)
{
    for (int i = 0; i < 64; i++)
    {
        //*(mem_start + i) = 0xFA7B;
    }
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    // This sets the stdin in a special state where each
    // keyboard press is directly flushed to the stdin and additionally
    // not outputted to the stdout
    {
        struct termios ttystate;
        tcgetattr(STDIN_FILENO, &ttystate);
        ttystate.c_lflag &= ~(ICANON | ECHO);
        ttystate.c_cc[VMIN] = 1;
        tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
    }

    // Allocate the playing field structure
    game.rawPlayfield = (tile *)malloc(game.grid.x * game.grid.y * sizeof(tile));
    game.playfield = (tile **)malloc(game.grid.y * sizeof(tile *));
    if (!game.playfield || !game.rawPlayfield)
    {
        fprintf(stderr, "ERROR: could not allocate playfield\n");
        return 1;
    }
    for (unsigned int y = 0; y < game.grid.y; y++)
    {
        game.playfield[y] = &(game.rawPlayfield[y * game.grid.x]);
    }

    // Reset playfield to make it empty
    resetPlayfield();
    // Start with gameOver
    gameOver();

    if (!initializeSenseHat())
    {
        fprintf(stderr, "ERROR: could not initilize sense hat\n");
        return 1;
    }

    if (!initialize_joystick())
    {
        fprintf(stderr, "ERROR: could not initilize joystick\n");
        return 1;
    }

    // Clear console, render first time
    fprintf(stdout, "\033[H\033[J");
    renderConsole(true);
    renderSenseHatMatrix(true);
    check_color();

    while (true)
    {
        struct timeval sTv, eTv;
        gettimeofday(&sTv, NULL);

        int key = readSenseHatJoystick();
        if (!key)
        {
            // NOTE: Uncomment the next line if you want to test your implementation with
            // reading the inputs from stdin. However, we expect you to read the inputs directly
            // from the input device and not from stdin (you should implement the readSenseHatJoystick
            // method).
            // key = readKeyboard();
        }
        if (key == KEY_ENTER)
            break;

        bool playfieldChanged = sTetris(key);
        renderConsole(playfieldChanged);
        renderSenseHatMatrix(playfieldChanged);

        // Wait for next tick
        gettimeofday(&eTv, NULL);
        unsigned long const uSecProcessTime = ((eTv.tv_sec * 1000000) + eTv.tv_usec) - ((sTv.tv_sec * 1000000 + sTv.tv_usec));
        if (uSecProcessTime < game.uSecTickTime)
        {
            usleep(game.uSecTickTime - uSecProcessTime);
        }
        game.tick = (game.tick + 1) % game.nextGameTick;
    }

    freeSenseHat();
    free(game.playfield);
    free(game.rawPlayfield);

    return 0;
}
