/***************************************************************************************************
 * DON'T REMOVE THE VARIABLES BELOW THIS COMMENT                                                   *
 **************************************************************************************************/
unsigned long long __attribute__((used)) VGAaddress = 0xc8000000; // Memory storing pixels
unsigned int __attribute__((used)) red = 0x0000F0F0;
unsigned int __attribute__((used)) green = 0x00000F0F;
unsigned int __attribute__((used)) blue = 0x000000FF;
unsigned int __attribute__((used)) white = 0x0000FFFF;
unsigned int __attribute__((used)) black = 0x0;

unsigned char n_cols = 10; // <- This variable might change depending on the size of the game. Supported value range: [1,18]

char *won = "You Won";       // DON'T TOUCH THIS - keep the string as is
char *lost = "You Lost";     // DON'T TOUCH THIS - keep the string as is
unsigned short height = 240; // DON'T TOUCH THIS - keep the value as is
unsigned short width = 320;  // DON'T TOUCH THIS - keep the value as is
char font8x8[128][8];        // DON'T TOUCH THIS - this is a forward declaration
/**************************************************************************************************/

/***
 * TODO: Define your variables below this comment
 */
unsigned int position_bar = 90;

/***
 * You might use and modify the struct/enum definitions below this comment
 */
typedef struct _block
{
    unsigned char destroyed;
    unsigned char deleted;
    unsigned int pos_x;
    unsigned int pos_y;
    unsigned int color;
} Block;

Block *list_of_stucts;

typedef enum _gameState
{
    Stopped = 0,
    Running = 1,
    Won = 2,
    Lost = 3,
    Exit = 4,
} GameState;
GameState currentState = Stopped;

typedef struct _ball
{
    unsigned int pos_x;
    unsigned int pos_y;
    int dir_x;
    int dir_y;
} Ball;

Ball ball = {.pos_x = 7, .pos_y = 112};

/***
 * Here follow the C declarations for our assembly functions
 */

// TODO: Add a C declaration for the ClearScreen assembly procedure
void SetPixel(unsigned int x_coord, unsigned int y_coord, unsigned int color);
void DrawBlock(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned int color);
void DrawBar(unsigned int y);
int ReadUart();
void WriteUart(char c);
void ClearScreen(void);

/***
 * Now follow the assembly implementations
 */

asm("ClearScreen: \n\t"
    "    PUSH {LR} \n\t"
    "    PUSH {R4, R5, R6, R7} \n\t"
    // TODO: Add ClearScreen implementation in assembly here
    "   LDR r4, =width \n\t"
    // width is a short which is 2 bytes, so must add ldrh.
    "   LDRh R4, [R4] \n\t"
    "   LDR r5, =height \n\t"
    "   LDRh r5, [r5] \n\t"

    // height is now stored in r4, width in r5
    "   MOV R0, #0 \n\t"
    "   MOV R1, #0 \n\t"
    "   LDR R2, =white \n\t"
    "   LDR R2, [R2] \n\t"
    // Uses r6 and r7 for coordinates, cannot just use setPixel as it changes the values.
    "   MOV r6, #0 \n\t"
    "   MOV r7, #0 \n\t"

    "   increase_height: \n\t"
    // resets the x coordinates
    "   mov r7, #0 \n\t"

    "   increase_width: \n\t"
    "   mov r1, r6 \n\t"
    "   mov r0, r7 \n\t"

    "   bl SetPixel \n\t"

    "   add r7, r7, #1 \n\t"
    "   cmp r7, r4 \n\t"
    // if the x coordinate is not the same as width it will ocntinue to move along.
    "   bne increase_width \n\t"

    "   add r6, r6, #1 \n\t"
    "   cmp r5, r6 \n\t"
    "   bne increase_height\n\t"

    "    POP {R4,R5, R6, R7}\n\t"
    "    POP {LR} \n\t"
    "    BX LR");

// assumes R0 = x-coord, R1 = y-coord, R2 = colorvalue
asm("SetPixel: \n\t"
    "LDR R3, =VGAaddress \n\t"
    "LDR R3, [R3] \n\t"
    "LSL R1, R1, #10 \n\t"
    "LSL R0, R0, #1 \n\t"
    "ADD R1, R0 \n\t"
    "STRH R2, [R3,R1] \n\t"
    "BX LR");

// TODO: Implement the DrawBlock function in assembly. You need to accept 5 parameters, as outlined in the c declaration above (unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned int color)
asm("DrawBlock: \n\t"
    // TODO: Here goes your implementation
    "PUSH {LR} \n\t"

    // I am not using r4 in this, however I wanted to showcase that it is common to use the registers r4 and up
    // and have r0-r3 parameters. The reason why it is not used is just that I found simpler implementation.
    "PUSH {r4, r5, R6, R7, R8, R9}\n\t"
    // moves x coordinate into r5 and y into r6

    "mov r5, r0 \n\t"
    "mov r6, r1 \n\t"
    // r7 has the width
    "mov r7, r2 \n\t"
    "mov r8, #0 \n\t"
    "mov r9, r3\n\t"
    // since we have so many parameters, the fifth is on stack
    //  this is so we don't breach the arm conventions for register use.
    //  24 is calculated by (7 * 4) since I have pushed 6 registers
    "ldr r2, [sp, #28] \n\t"

    // DO NOT TOUCH R2

    // Here I use the same principles as in the the clear screen, however now by width.
    "y_axis: \n\t"
    "mov r8, #0 \n\t"
    "x_axis: \n\t"

    // load r5 (new x coor) to the first parameter
    "mov r0, r5 \n\t"
    // load r6 (new y coor) to the second parameter
    "mov r1, r6 \n\t"
    "bl SetPixel \n\t"

    // add one to the counter for width
    "add r8, r8, #1 \n\t"
    // add one to the x coordinate
    "add r5, r5, #1 \n\t"

    // if counter not equal to the width, coninue
    "cmp r8, r7 \n\t"
    "bne x_axis \n\t "

    // removes the added x coordinates
    "sub r5, r5, r7 \n\t"
    "add r6, r6, #1 \n\t"

    "sub r9, r9, #1 \n\t"

    "cmp r9, #0 \n\t"
    "bne y_axis \n\t"

    "POP { r4, R5, R6, R7, R8, R9} \n\t"
    "POP {LR} \n\t"
    "BX LR");

// TODO: Impelement the DrawBar function in assembly. You need to accept the parameter as outlined in the c declaration above (unsigned int y)
asm("DrawBar: \n\t"
    "PUSH {LR} \n\t"
    "PUSH {r4}\n\t"

    // position 7
    "mov r1, r0\n\t"
    // x coordinate 0
    "mov r0, #0\n\t"
    // width
    "mov r2, #7 \n\t"
    // height
    "mov r3, #45\n\t"

    "LDR R4, =blue \n\t"
    "LDR R4, [R4] \n\t"
    "PUSH {r4}\n\t"

    "bl DrawBlock \n\t"
    // will be a bit shorter

    "POP {r4} \n\t"
    "POP {R4} \n\t"
    "POP {LR} \n\t"

    "BX LR");

asm("ReadUart:\n\t"
    "LDR R1, =0xFF201000 \n\t"
    "LDR R0, [R1]\n\t"
    "BX LR");

// TODO: Add the WriteUart assembly procedure here that respects the WriteUart C declaration on line 46
asm("WriteUart:\n\t"
    "PUSH {LR} \n\t"
    "PUSH {R4} \n\t"
    // Address for Uart
    "LDR R4, =0xFF201000 \n\t"
    "str r0, [r4] \n\t"

    "POP {R4} \n\t"
    "POP {LR} \n\t"
    "BX LR");

// TODO: Implement the C functions below
void draw_ball()
{
    DrawBlock(ball.pos_x, ball.pos_y, 7, 7, black);
}

/*
    Used to initialize the blocks. Adds every block to the list of structs
    Also, Since 16 (the number of blocks that fit height-wise) % 3 != 0,
    we know two identical colors will never be next to each other.
*/
void draw_playing_field()
{
    unsigned int color_list[] = {red, green, blue};
    int counter = 0;
    int start_block = width - (n_cols * 15);
    // list_of_stucts[n_cols * 16];
    for (int i = start_block; i < width; i += 15)
    {
        for (int j = 0; j < height; j += 15)
        {
            unsigned int color = color_list[counter % 3];
            DrawBlock(i, j, 15, 15, color);
            Block new_block = {.color = color, .pos_x = i, .pos_y = j};
            list_of_stucts[counter] = new_block;
            counter = counter + 1;
        }
    }
}

void check_collision_edges(void)
{
    if (ball.pos_y < 7)
    {
        ball.dir_y = 1;
    }
    else if (ball.pos_y > height - 7)
    {
        ball.dir_y = -1;
    }
    if (ball.pos_x < 7)
    {
        currentState = Lost;
    }
    else if (ball.pos_x > width - 7)
    {
        currentState = Won;
    }
}

void bar_collision_check(void)
{
    // check if the ball touches, then changes directions as given in the handout
    if ((ball.pos_x <= 7) && (ball.pos_y >= position_bar) && (ball.pos_y < position_bar + 45))
    {
        if ((position_bar <= ball.pos_y + 3) && (ball.pos_y + 3 < position_bar + 15))
        {
            ball.dir_x = 1;
            ball.dir_y = -1;
        }
        else if ((ball.pos_y + 3 >= position_bar + 15) && (ball.pos_y + 3 < position_bar + 30))
        {
            ball.dir_x = 1;
            ball.dir_y = 0;
        }
        else
        {
            ball.dir_x = 1;
            ball.dir_y = 1;
        }
    }
}

void block_collision(void)
{
    if (ball.pos_x >= width - ((n_cols + 1) * 15))
    {
        for (int i = 0; i < n_cols * 16; i++)
        {
            Block *check = &list_of_stucts[i];
            // these two check if we are along the right column or row.
            int y_match = (check->pos_y <= ball.pos_y && check->pos_y + 15 > ball.pos_y) || (check->pos_y <= ball.pos_y + 7 && check->pos_y + 15 > ball.pos_y + 7);
            int x_match = (check->pos_x <= ball.pos_x && check->pos_x + 15 > ball.pos_x) || (check->pos_x <= ball.pos_x + 7 && check->pos_x + 15 > ball.pos_x + 7);
            if (check->destroyed == 0)
            {
                if (y_match)
                {
                    // checks collision in right ball
                    if (check->pos_x - ball.pos_x < 7)
                    {
                        check->destroyed = 1;
                        check->color = white;
                        ball.dir_x = -1;
                        DrawBlock(check->pos_x, check->pos_y, 15, 15, white);
                    }

                    // checks collision in leftmost pixel in ball
                    else if (check->pos_x + 15 - ball.pos_x < 7)
                    {
                        check->destroyed = 1;
                        check->color = white;
                        ball.dir_x = 1;
                        DrawBlock(check->pos_x, check->pos_y, 15, 15, white);
                        write("left\n");
                    }
                }
                if (x_match)
                {
                    // checks collision in top pixel of ball
                    if (check->pos_y + 15 - ball.pos_y < 7)
                    {
                        check->destroyed = 1;
                        check->color = white;
                        ball.dir_y = 1;
                        DrawBlock(check->pos_x, check->pos_y, 15, 15, white);
                    }
                    // checks collision in bottom pixel of ball
                    else if (check->pos_y - ball.pos_y < 7)
                    {
                        check->destroyed = 1;
                        check->color = white;
                        ball.dir_y = -1;
                        DrawBlock(check->pos_x, check->pos_y, 15, 15, white);
                    }
                }
            }
        }
    }
}
void update_game_state()
{
    if (currentState != Running)
    {
        return;
    }

    // TODO: Check: game won? game lost?

    // TODO: Update balls position and direction

    check_collision_edges();
    bar_collision_check();
    if (currentState == lost)
    {
        return;
    }
    block_collision();
    DrawBlock(ball.pos_x, ball.pos_y, 7, 7, white);

    ball.pos_x = ball.pos_x + (ball.dir_x * 7);
    ball.pos_y = ball.pos_y + (ball.dir_y * 7);
    draw_ball();

    // TODO: Hit Check with Blocks
    // HINT: try to only do this check when we potentially have a hit, as it is relatively expensive and can slow down game play a lot
}

/*
    Bar moves by 15
*/
void update_bar_state()
{
    int remaining = 0;
    int char_read = ReadUart();
    // must shift the bytes to get values
    int first_byte = (char_read & 0xFF);
    int second_byte = ((char_read >> 8) & 0xFF);
    // All decimals nums are the nums converted from hex.
    if (second_byte == 128)
    {
        if ((first_byte == 115) && (position_bar < height - 45))
        {
            position_bar += (15);
            DrawBar(position_bar);
            DrawBlock(0, position_bar - 15, 7, 15, white);
        }
        if ((first_byte == 119) && (position_bar > 15))
        {
            position_bar -= (15);
            DrawBar(position_bar);
            DrawBlock(0, position_bar + 45, 7, 15, white);
        }
        if (first_byte == '\n' || first_byte == '\r')
        {
            currentState = Exit;
        }
    }
    // TODO: Read all chars in the UART Buffer and apply the respective bar position updates
    // HINT: w == 77, s == 73
    // HINT Format: 0x00 'Remaining Chars':2 'Ready 0x80':2 'Char 0xXX':2, sample: 0x00018077 (1 remaining character, buffer is ready, current character is 'w')
}

void write(char *str)
{
    // TODO: Use WriteUart to write the string to JTAG UART
    char *copy = str;
    for (;;)
    {
        char sent = *copy;
        WriteUart(sent);

        copy += 1;
        if (*copy == '\0')
        {
            break;
        }
    }
}

void play()
{
    // Need to have tick to slow down
    int tick = 0;
    // HINT: This is the main game loop

    while (1)
    {
        if (tick % 100000 == 0)
        {
            update_game_state();
            update_bar_state();
            if (currentState != Running)
            {
                break;
            }
            // DrawBar(position_bar); // TODO: replace the constant value with the current position of the bar
        }
        tick++;
    }
    if (currentState == Won)
    {
        write(won);
        return;
    }
    else if (currentState == Lost)
    {
        write(lost);
        return;
    }
    else if (currentState == Exit)
    {
        return;
    }
    currentState = Stopped;
}

void reset()
{
    // Hint: This is draining the UART buffer
    int remaining = 0;
    do
    {
        unsigned long long out = ReadUart();
        if (!(out & 0x8000))
        {
            // not valid - abort reading
            break;
        }
        remaining = (out & 0xFF0000) >> 4;
    } while (remaining > 0);
    ClearScreen();
    ball.pos_x = 7;
    ball.pos_y = 112;
    draw_ball();
    draw_playing_field();
    DrawBar(90);

    // TODO: You might want to reset other state in here
}

void wait_for_start()
{
    while (1 == 1)
    {
        int char_read = ReadUart();
        int first_byte = (char_read & 0xFF);
        int second_byte = ((char_read >> 8) & 0xFF);

        if (second_byte == 128)
        {
            // 115 is s and w is 119.
            if (first_byte == 115 || first_byte == 119)
            {
                currentState = Running;
                break;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    if (n_cols > 18 || n_cols < 1)
    {
        write("Not playable configuration! Must change n_cols\n");
        while (1 == 1)
        {
            int i = 0;
        }
    }
    Block block_array[n_cols * 16];
    list_of_stucts = block_array;

    ClearScreen();

    // initial direction for ball
    ball.dir_x = 1;

    DrawBar(position_bar);
    draw_playing_field();
    draw_ball();

    // HINT: This loop allows the user to restart the game after loosing/winning the previous game
    while (1)
    {
        wait_for_start();
        play();
        reset();
        if (currentState == Exit)
        {
            break;
        }
        else
        {
            currentState = Running;
            ball.dir_x = 1;
        }
    }
    return 0;
}

// THIS IS FOR THE OPTIONAL TASKS ONLY

// HINT: How to access the correct bitmask
// sample: to get character a's bitmask, use
// font8x8['a']
char font8x8[128][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0000 (nul)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0001
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0002
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0003
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0004
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0005
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0006
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0007
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0008
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0009
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000A
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000B
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000C
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000D
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000E
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000F
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0010
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0011
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0012
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0013
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0014
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0015
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0016
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0017
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0018
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0019
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001A
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001B
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001C
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001D
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001E
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001F
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0020 (space)
    {0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00}, // U+0021 (!)
    {0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0022 (")
    {0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00}, // U+0023 (#)
    {0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00}, // U+0024 ($)
    {0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00}, // U+0025 (%)
    {0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00}, // U+0026 (&)
    {0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0027 (')
    {0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00}, // U+0028 (()
    {0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00}, // U+0029 ())
    {0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00}, // U+002A (*)
    {0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00}, // U+002B (+)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x06}, // U+002C (,)
    {0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00}, // U+002D (-)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00}, // U+002E (.)
    {0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00}, // U+002F (/)
    {0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00}, // U+0030 (0)
    {0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00}, // U+0031 (1)
    {0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00}, // U+0032 (2)
    {0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00}, // U+0033 (3)
    {0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00}, // U+0034 (4)
    {0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00}, // U+0035 (5)
    {0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00}, // U+0036 (6)
    {0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00}, // U+0037 (7)
    {0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00}, // U+0038 (8)
    {0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00}, // U+0039 (9)
    {0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00}, // U+003A (:)
    {0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x06}, // U+003B (;)
    {0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00}, // U+003C (<)
    {0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00}, // U+003D (=)
    {0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00}, // U+003E (>)
    {0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00}, // U+003F (?)
    {0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00}, // U+0040 (@)
    {0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00}, // U+0041 (A)
    {0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00}, // U+0042 (B)
    {0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00}, // U+0043 (C)
    {0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00}, // U+0044 (D)
    {0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00}, // U+0045 (E)
    {0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00}, // U+0046 (F)
    {0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00}, // U+0047 (G)
    {0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00}, // U+0048 (H)
    {0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // U+0049 (I)
    {0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00}, // U+004A (J)
    {0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00}, // U+004B (K)
    {0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00}, // U+004C (L)
    {0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00}, // U+004D (M)
    {0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00}, // U+004E (N)
    {0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00}, // U+004F (O)
    {0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00}, // U+0050 (P)
    {0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00}, // U+0051 (Q)
    {0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00}, // U+0052 (R)
    {0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00}, // U+0053 (S)
    {0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // U+0054 (T)
    {0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00}, // U+0055 (U)
    {0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00}, // U+0056 (V)
    {0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00}, // U+0057 (W)
    {0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00}, // U+0058 (X)
    {0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00}, // U+0059 (Y)
    {0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00}, // U+005A (Z)
    {0x1E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x1E, 0x00}, // U+005B ([)
    {0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00}, // U+005C (\)
    {0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00}, // U+005D (])
    {0x08, 0x1C, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00}, // U+005E (^)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF}, // U+005F (_)
    {0x0C, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0060 (`)
    {0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x6E, 0x00}, // U+0061 (a)
    {0x07, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3B, 0x00}, // U+0062 (b)
    {0x00, 0x00, 0x1E, 0x33, 0x03, 0x33, 0x1E, 0x00}, // U+0063 (c)
    {0x38, 0x30, 0x30, 0x3e, 0x33, 0x33, 0x6E, 0x00}, // U+0064 (d)
    {0x00, 0x00, 0x1E, 0x33, 0x3f, 0x03, 0x1E, 0x00}, // U+0065 (e)
    {0x1C, 0x36, 0x06, 0x0f, 0x06, 0x06, 0x0F, 0x00}, // U+0066 (f)
    {0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x1F}, // U+0067 (g)
    {0x07, 0x06, 0x36, 0x6E, 0x66, 0x66, 0x67, 0x00}, // U+0068 (h)
    {0x0C, 0x00, 0x0E, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // U+0069 (i)
    {0x30, 0x00, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E}, // U+006A (j)
    {0x07, 0x06, 0x66, 0x36, 0x1E, 0x36, 0x67, 0x00}, // U+006B (k)
    {0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // U+006C (l)
    {0x00, 0x00, 0x33, 0x7F, 0x7F, 0x6B, 0x63, 0x00}, // U+006D (m)
    {0x00, 0x00, 0x1F, 0x33, 0x33, 0x33, 0x33, 0x00}, // U+006E (n)
    {0x00, 0x00, 0x1E, 0x33, 0x33, 0x33, 0x1E, 0x00}, // U+006F (o)
    {0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x0F}, // U+0070 (p)
    {0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x78}, // U+0071 (q)
    {0x00, 0x00, 0x3B, 0x6E, 0x66, 0x06, 0x0F, 0x00}, // U+0072 (r)
    {0x00, 0x00, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x00}, // U+0073 (s)
    {0x08, 0x0C, 0x3E, 0x0C, 0x0C, 0x2C, 0x18, 0x00}, // U+0074 (t)
    {0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00}, // U+0075 (u)
    {0x00, 0x00, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00}, // U+0076 (v)
    {0x00, 0x00, 0x63, 0x6B, 0x7F, 0x7F, 0x36, 0x00}, // U+0077 (w)
    {0x00, 0x00, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x00}, // U+0078 (x)
    {0x00, 0x00, 0x33, 0x33, 0x33, 0x3E, 0x30, 0x1F}, // U+0079 (y)
    {0x00, 0x00, 0x3F, 0x19, 0x0C, 0x26, 0x3F, 0x00}, // U+007A (z)
    {0x38, 0x0C, 0x0C, 0x07, 0x0C, 0x0C, 0x38, 0x00}, // U+007B ({)
    {0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00}, // U+007C (|)
    {0x07, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0x07, 0x00}, // U+007D (})
    {0x6E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+007E (~)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  // U+007F
};
