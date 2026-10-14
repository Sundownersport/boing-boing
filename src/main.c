#include "gt/gametank.h"
#include "gt/input.h"
#include "gt/gfx/draw_queue.h"
#include "gt/gfx/draw_direct.h"
#include "gt/feature/text/text.h"
#include "gt/banking.h"
#include "gen/bank_nums.h"

#define COL(h, s, l) (((h) << 5) | ((s) << 3) | (l))

#define C_BG         COL(5, 1, 1)
#define C_STONE      COL(0, 0, 4)
#define C_BRICK      COL(1, 2, 3)
#define C_BRICK_DARK COL(1, 2, 1)
#define C_SPRING     COL(7, 3, 4)
#define C_ARROW      COL(6, 3, 4)
#define C_SPIKE      COL(2, 3, 4)
#define C_HEART      COL(3, 3, 5)
#define C_WHITE      7

#define MAP_W 16
#define MAP_H 13
#define PF_Y 16
#define NUM_LEVELS 10

#define BALL_SIZE 6
#define SUB 6
#define MAX_BX ((128 - BALL_SIZE) << SUB)
#define GRAVITY 5
#define MAX_FALL 176
#define BOUNCE 125
#define SUPER_BOUNCE 197
#define ACCEL 5
#define FRICTION 2
#define MAX_VX 53
#define DASH_SPEED 106
#define DASH_FRAMES 44

#define LEVEL_SLOT 7

#define STATE_TITLE 0
#define STATE_PLAY 1
#define STATE_DEAD 2
#define STATE_CLEAR 3
#define STATE_WIN 4

#pragma rodata-name (push, "PROG0")
static const char levels[NUM_LEVELS][MAP_H][MAP_W + 1] = {
    {
        "................",
        "................",
        "................",
        "................",
        "................",
        "................",
        "................",
        "................",
        "..h.........h...",
        "................",
        ".S......h.......",
        "................",
        "################",
    },
    {
        "................",
        "................",
        "................",
        "................",
        "................",
        "................",
        "................",
        "................",
        ".....h.....h....",
        "................",
        ".S............h.",
        "...#...#.#...#..",
        "###.....#.....##",
    },
    {
        "................",
        "................",
        "................",
        "................",
        "................",
        "................",
        "................",
        "................",
        "..h....h....h...",
        "................",
        ".S..............",
        "##bbbbbbbbbbbb##",
        "xxxxxxxxxxxxxxxx",
    },
    {
        "................",
        "................",
        "................",
        ".h............h.",
        "####........####",
        "................",
        "................",
        "................",
        ".......h........",
        "................",
        ".S....^...^.....",
        "################",
        "################",
    },
    {
        "................",
        "................",
        "................",
        "................",
        "................",
        "................",
        "................",
        ".......#........",
        "S...............",
        ".....#........h.",
        "................",
        "#..#....^....###",
        "xxxxxxxxxxxxxxxx",
    },
        {
        "................",
        "................",
        "................",
        "................",
        "................",
        "................",
        "................",
        "................",
        "................",
        ".h............h.",
        ".S..^.......^...",
        "####.........###",
        "xxxxxxxxxxxxxxxx",
    },
    {
        "................",
        "................",
        "..h..........h..",
        ".###........###.",
        "................",
        "................",
        ".......hh.......",
        "......bbbb......",
        "................",
        "................",
        ".S..^......^....",
        "######xxxx######",
        "######xxxx######",
    },
        {
        "................",
        "................",
        "S...............",
        "................",
        "...............h",
        "................",
        "................",
        "................",
        "................",
        "................",
        ".......h........",
        "................",
        "^xxxxxxx#^xxxxxx",
    },    {
        "................",
        ".h.#............",
        "...#..#.........",
        "####............",
        "....#........#..",
        ".......#.....#.^",
        "..........#..#..",
        ".............#..",
        ".............#.x",
        ".............#.#",
        "S..............#",
        ".....>.........#",
        "##xxxxxxxxxxxx^.",
    },    {
        "...............S",
        "................",
        ".#####xx###x####",
        "................",
        "................",
        "######xx###x###.",
        "h...............",
        "................",
        "......x..x.xh###",
        "......#..#.#....",
        "......#.x#......",
        "^.......#.^.....",
        "##xxx#######xxxx",
    },
};
#pragma rodata-name (pop)

static char map[MAP_H][MAP_W];
static unsigned char hearts_left;
static unsigned char level;
static unsigned char state;
static unsigned char timer;
static int bx, by, vx, vy;
static unsigned char dash_timer;

static SpriteSlot font;
static char num_buf[3];

static void gram_begin() {
    direct_prepare_sprite_ram_array_mode(LEVEL_SLOT);
}

static void gram_end() {
    flagsMirror |= DMA_ENABLE;
    *dma_flags = flagsMirror;
}

static void gram_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h, unsigned char c) {
    unsigned char i;
    unsigned char* p = vram + ((unsigned int)y << 7) + x;
    while (h--) {
        for (i = 0; i < w; ++i) p[i] = c;
        p += 128;
    }
}

static void draw_tile(unsigned char r, unsigned char c) {
    unsigned char x = c << 3, y = r << 3;
    switch (map[r][c]) {
    case '#':
        gram_rect(x, y, (c < MAP_W - 1 && map[r][c + 1] == '#') ? 8 : 7, 7, C_STONE);
        break;
    case 'x':
        gram_rect(x + 3, y, 1, 7, C_SPIKE);
        gram_rect(x, y + 3, 7, 1, C_SPIKE);
        gram_rect(x + 2, y + 2, 3, 3, C_SPIKE);
        gram_rect(x + 1, y + 1, 1, 1, C_SPIKE);
        gram_rect(x + 5, y + 1, 1, 1, C_SPIKE);
        gram_rect(x + 1, y + 5, 1, 1, C_SPIKE);
        gram_rect(x + 5, y + 5, 1, 1, C_SPIKE);
        break;
    case 'b':
        gram_rect(x, y, 7, 7, C_BRICK);
        gram_rect(x + 1, y + 3, 5, 1, C_BRICK_DARK);
        break;
    case '^':
        gram_rect(x, y, 7, 7, C_SPRING);
        gram_rect(x + 2, y + 1, 3, 2, C_WHITE);
        break;
    case '>':
        gram_rect(x, y, 7, 7, C_ARROW);
        gram_rect(x + 4, y + 2, 2, 3, C_WHITE);
        break;
    case '<':
        gram_rect(x, y, 7, 7, C_ARROW);
        gram_rect(x + 1, y + 2, 2, 3, C_WHITE);
        break;
    case 'h':
        gram_rect(x + 1, y + 2, 5, 2, C_HEART);
        gram_rect(x + 2, y + 4, 3, 1, C_HEART);
        gram_rect(x + 3, y + 5, 1, 1, C_HEART);
        break;
    }
}

static void clear_tile(unsigned char r, unsigned char c) {
    map[r][c] = '.';
    gram_begin();
    gram_rect(c << 3, r << 3, 8, 8, C_BG);
    gram_end();
}

static void load_level() {
    unsigned char r, c;
    char t;
    hearts_left = 0;
    vx = 0;
    vy = 0;
    dash_timer = 0;
    push_rom_bank();
    change_rom_bank(BANK_PROG0);
    for (r = 0; r < MAP_H; ++r) {
        for (c = 0; c < MAP_W; ++c) {
            t = levels[level][r][c];
            if (t == 'S') {
                bx = (int)((c << 3) + 1) << SUB;
                by = (int)((r << 3) + 2) << SUB;
                t = '.';
            } else if (t == 'h') {
                ++hearts_left;
            }
            map[r][c] = t;
        }
    }
    pop_rom_bank();
    gram_begin();
    for (r = 0; r < MAP_H; ++r) {
        gram_rect(0, r << 3, 128, 8, C_BG);
        for (c = 0; c < MAP_W; ++c) {
            draw_tile(r, c);
        }
    }
    gram_end();
}

static char tile_at(unsigned char px, unsigned char py) {
    py >>= 3;
    if (py >= MAP_H) return '.';
    return map[py][px >> 3];
}

static bool is_solid(char t) {
    return t == '#' || t == 'b' || t == '^' || t == '<' || t == '>';
}

static void bounce_on(unsigned char px, unsigned char py) {
    unsigned char r = py >> 3, c = px >> 3;
    switch (map[r][c]) {
    case '^':
        vy = -SUPER_BOUNCE;
        break;
    case '>':
        vy = 0;
        vx = DASH_SPEED;
        dash_timer = DASH_FRAMES;
        break;
    case '<':
        vy = 0;
        vx = -DASH_SPEED;
        dash_timer = DASH_FRAMES;
        break;
    case 'b':
        clear_tile(r, c);
        vy = -BOUNCE;
        break;
    default:
        vy = -BOUNCE;
    }
}

static void move_x() {
    unsigned char l, r, t, b;
    bx += vx;
    if (bx < 0) {
        bx = 0;
        vx = 0;
        dash_timer = 0;
    } else if (bx > MAX_BX) {
        bx = MAX_BX;
        vx = 0;
        dash_timer = 0;
    }
    l = bx >> SUB;
    r = l + BALL_SIZE - 1;
    t = by >> SUB;
    b = t + BALL_SIZE - 1;
    if (vx > 0 && (is_solid(tile_at(r, t)) || is_solid(tile_at(r, b)))) {
        bx = (int)((r & 0xF8) - BALL_SIZE) << SUB;
        vx = 0;
        dash_timer = 0;
    } else if (vx < 0 && (is_solid(tile_at(l, t)) || is_solid(tile_at(l, b)))) {
        bx = (int)((l & 0xF8) + 8) << SUB;
        vx = 0;
        dash_timer = 0;
    }
}

static void move_y() {
    unsigned char l, r, t, b, mid;
    if (dash_timer) {
        --dash_timer;
        vy = 0;
    } else {
        vy += GRAVITY;
        if (vy > MAX_FALL) vy = MAX_FALL;
    }
    by += vy;
    if (by < 0) {
        by = 0;
        if (vy < 0) vy = 0;
    }
    l = bx >> SUB;
    r = l + BALL_SIZE - 1;
    t = by >> SUB;
    b = t + BALL_SIZE - 1;
    if (vy > 0) {
        if (is_solid(tile_at(l, b)) || is_solid(tile_at(r, b))) {
            by = (int)((b & 0xF8) - BALL_SIZE) << SUB;
            mid = l + (BALL_SIZE >> 1);
            if (is_solid(tile_at(mid, b))) bounce_on(mid, b);
            else if (is_solid(tile_at(l, b))) bounce_on(l, b);
            else bounce_on(r, b);
        }
    } else if (vy < 0) {
        if (is_solid(tile_at(l, t)) || is_solid(tile_at(r, t))) {
            by = (int)((t & 0xF8) + 8) << SUB;
            vy = 0;
        }
    }
}

static void touch(unsigned char px, unsigned char py) {
    unsigned char r = py >> 3, c = px >> 3;
    if (r >= MAP_H) return;
    if (map[r][c] == 'h') {
        clear_tile(r, c);
        --hearts_left;
    } else if (map[r][c] == 'x') {
        state = STATE_DEAD;
        timer = 0;
    }
}

static void check_touch() {
    unsigned char l, r, t, b;
    l = (bx >> SUB) + 1;
    r = l + BALL_SIZE - 3;
    t = (by >> SUB) + 1;
    b = t + BALL_SIZE - 3;
    touch(l, t);
    touch(r, t);
    touch(l, b);
    touch(r, b);
}

static void steer() {
    if (dash_timer) return;
    if (player1_buttons & INPUT_MASK_LEFT) {
        vx -= ACCEL;
        if (vx < -MAX_VX) vx = -MAX_VX;
    } else if (player1_buttons & INPUT_MASK_RIGHT) {
        vx += ACCEL;
        if (vx > MAX_VX) vx = MAX_VX;
    } else if (vx > FRICTION) {
        vx -= FRICTION;
    } else if (vx < -FRICTION) {
        vx += FRICTION;
    } else {
        vx = 0;
    }
}

static void update_play() {
    if (player1_new_buttons & INPUT_MASK_START) {
        load_level();
        return;
    }
    steer();
    move_x();
    move_y();
    if ((by >> SUB) >= (MAP_H << 3) - 4) {
        state = STATE_DEAD;
        timer = 0;
        return;
    }
    check_touch();
    if (state == STATE_PLAY && hearts_left == 0) {
        state = STATE_CLEAR;
        timer = 0;
    }
}

static void draw_text(unsigned char x, unsigned char y, const char* s) {
    unsigned char ch;
    while ((ch = *s++)) {
        if (ch != ' ') {
            ch += TEXT_COLOR_WHITE;
            queue_draw_sprite(x, y, 8, 8, (ch & 0x0F) << 3, (ch & 0xF0) >> 1, font);
        }
        x += 8;
    }
}

static char* two_digits(unsigned char n) {
    num_buf[0] = '0' + n / 10;
    num_buf[1] = '0' + n % 10;
    num_buf[2] = 0;
    return num_buf;
}

static void draw_ball(unsigned char x, unsigned char y) {
    queue_draw_box(x + 1, y, BALL_SIZE - 2, BALL_SIZE, C_WHITE);
    queue_draw_box(x, y + 1, BALL_SIZE, BALL_SIZE - 2, C_WHITE);
}

static void draw_level() {
    queue_draw_box(1, 7, SCREEN_WIDTH - 2, PF_Y - 7, C_BG);
    queue_draw_sprite(0, PF_Y, 64, MAP_H << 3, 0, 0, LEVEL_SLOT);
    queue_draw_sprite(64, PF_Y, 64, MAP_H << 3, 64, 0, LEVEL_SLOT);
    draw_text(4, 8, "LEVEL");
    draw_text(52, 8, two_digits(level + 1));
    queue_draw_box(97, 10, 5, 2, C_HEART);
    queue_draw_box(98, 12, 3, 1, C_HEART);
    queue_draw_box(99, 13, 1, 1, C_HEART);
    draw_text(108, 8, two_digits(hearts_left));
}

static int title_y, title_vy;

static void draw_title() {
    title_vy += GRAVITY;
    title_y += title_vy;
    if (title_y > (96 << SUB)) {
        title_y = 96 << SUB;
        title_vy = -BOUNCE;
    }
    draw_text(16, 32, "BOING BOING!");
    queue_draw_box(40, 102, 48, 7, C_STONE);
    draw_ball(61, title_y >> SUB);
    if (timer & 32) draw_text(20, 64, "PRESS START");
}

void main() {
    font = text_load_font();
    state = STATE_TITLE;
    title_y = 60 << SUB;
    title_vy = 0;
    timer = 0;

    while (1) {
        update_inputs();
        ++timer;

        switch (state) {
        case STATE_TITLE:
            if (player1_new_buttons & (INPUT_MASK_START | INPUT_MASK_A)) {
                level = 0;
                load_level();
                state = STATE_PLAY;
            }
            break;
        case STATE_PLAY:
            update_play();
            break;
        case STATE_DEAD:
            if (timer == 45) {
                load_level();
                state = STATE_PLAY;
            }
            break;
        case STATE_CLEAR:
            if (timer == 60) {
                ++level;
                if (level == NUM_LEVELS) {
                    state = STATE_WIN;
                } else {
                    load_level();
                    state = STATE_PLAY;
                }
            }
            break;
        case STATE_WIN:
            if (player1_new_buttons & INPUT_MASK_START) {
                state = STATE_TITLE;
            }
            break;
        }

        switch (state) {
        case STATE_TITLE:
            queue_clear_screen(C_BG);
            draw_title();
            break;
        case STATE_PLAY:
            draw_level();
            draw_ball(bx >> SUB, PF_Y + (by >> SUB));
            break;
        case STATE_DEAD:
            if (timer < 4) queue_clear_screen(C_SPIKE);
            else draw_level();
            if (timer & 4) draw_ball(bx >> SUB, PF_Y + (by >> SUB));
            break;
        case STATE_CLEAR:
            draw_level();
            draw_ball(bx >> SUB, PF_Y + (by >> SUB));
            draw_text(40, 56, "CLEAR!");
            break;
        case STATE_WIN:
            queue_clear_screen(C_BG);
            draw_text(28, 48, "YOU WIN!");
            draw_text(20, 72, "PRESS START");
            break;
        }
        queue_clear_border(0);

        await_draw_queue();
        await_vsync(1);
        flip_pages();
    }
}
