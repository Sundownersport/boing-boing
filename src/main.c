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
#define C_KEY        COL(1, 3, 6)
#define C_PAD        COL(5, 3, 4)
#define C_CRUMBLE    COL(1, 1, 4)
#define C_CRACK      COL(1, 1, 2)
#define C_PLAT_SLOW  COL(6, 1, 3)
#define C_PLAT_MID   COL(6, 1, 4)
#define C_PLAT_FAST  COL(6, 1, 6)
#define C_WHITE      7
#define C_PAIR       1

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
#define LOW_BOUNCE 88
#define SHAKE_FRAMES 16
#define MAX_MOVERS 4
#define MAX_PLATFORMS 8
#define NO_PLATFORM 255
#define GLYPH_Y 104

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
        "...........LLL..",
        "..h........LhL..",
        "...........LLL..",
        ".S......k.......",
        "................",
        "################",
    },
    {
        "................",
        "1...............",
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
        "###.....1.....##",
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
        "##cbcbcbcbcbcb##",
        "xxxxxxxxxxxxxxxx",
    },
    {
        "................",
        "................",
        "................",
        ".h............h.",
        "####m.......####",
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
        "...#..b.........",
        "####..x.........",
        "....bb.......#..",
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

static char map[MAP_H][MAP_W];
static unsigned char hearts_left;
static unsigned char keys_left, keys_total;
static unsigned char level;
static unsigned char state;
static unsigned char timer;
static int bx, by, vx, vy;
static unsigned char dash_timer;
static unsigned char shake_r, shake_c, shake_timer;
static unsigned char tele_cooldown, tele_touched;
static unsigned char movers;
static unsigned char mover_x[MAX_MOVERS], mover_y[MAX_MOVERS];
static signed char mover_dir[MAX_MOVERS];
static unsigned char platforms;
static unsigned char plat_x[MAX_PLATFORMS], plat_y[MAX_PLATFORMS], plat_speed[MAX_PLATFORMS];
static unsigned char plat_tr[MAX_PLATFORMS], plat_tc[MAX_PLATFORMS], plat_pr[MAX_PLATFORMS], plat_pc[MAX_PLATFORMS];

static SpriteSlot font;
static char num_buf[3];

static const char glyph_chars[] = "xhkb^><Lvc";
#define GLYPH_C 10
#define GLYPH_TELE 11
#define GLYPH_PLAT 12

static const unsigned char glyph_start[] = { 0, 7, 14, 21, 23, 25, 27, 29, 31, 33, 38, 43, 52, 54 };

static const unsigned char glyph_rects[] = {
    3, 0, 1, 7, C_SPIKE,
    0, 3, 7, 1, C_SPIKE,
    2, 2, 3, 3, C_SPIKE,
    1, 1, 1, 1, C_SPIKE,
    5, 1, 1, 1, C_SPIKE,
    1, 5, 1, 1, C_SPIKE,
    5, 5, 1, 1, C_SPIKE,

    1, 1, 2, 1, C_HEART,
    4, 1, 2, 1, C_HEART,
    0, 2, 7, 2, C_HEART,
    1, 4, 5, 1, C_HEART,
    2, 5, 3, 1, C_HEART,
    3, 6, 1, 1, C_HEART,
    1, 2, 1, 1, C_WHITE,

    1, 2, 2, 1, C_KEY,
    0, 3, 1, 2, C_KEY,
    3, 3, 1, 2, C_KEY,
    1, 5, 2, 1, C_KEY,
    3, 3, 4, 1, C_KEY,
    4, 4, 1, 1, C_KEY,
    6, 4, 1, 1, C_KEY,

    0, 0, 7, 7, C_BRICK,
    1, 3, 5, 1, C_BRICK_DARK,

    0, 0, 7, 7, C_SPRING,
    2, 1, 3, 2, C_WHITE,

    0, 0, 7, 7, C_ARROW,
    4, 2, 2, 3, C_WHITE,

    0, 0, 7, 7, C_ARROW,
    1, 2, 2, 3, C_WHITE,

    0, 0, 7, 7, C_KEY,
    3, 1, 1, 4, C_BG,

    0, 0, 7, 7, C_PAD,
    1, 5, 5, 1, C_WHITE,

    0, 0, 7, 7, C_CRUMBLE,
    1, 1, 1, 1, C_CRACK,
    4, 2, 1, 1, C_CRACK,
    2, 4, 1, 1, C_CRACK,
    5, 5, 1, 1, C_CRACK,

    0, 0, 7, 7, C_CRUMBLE,
    3, 0, 1, 2, C_CRACK,
    2, 2, 1, 2, C_CRACK,
    3, 4, 1, 1, C_CRACK,
    4, 5, 1, 2, C_CRACK,

    2, 0, 3, 1, C_PAIR,
    2, 6, 3, 1, C_PAIR,
    0, 2, 1, 3, C_PAIR,
    6, 2, 1, 3, C_PAIR,
    1, 1, 1, 1, C_PAIR,
    5, 1, 1, 1, C_PAIR,
    1, 5, 1, 1, C_PAIR,
    5, 5, 1, 1, C_PAIR,
    3, 3, 1, 1, C_WHITE,

    0, 0, 7, 7, C_PAIR,
    1, 1, 5, 1, C_WHITE,
};

static const unsigned char pair_colors[] = { COL(4, 3, 5), COL(6, 3, 6), COL(3, 3, 6), COL(0, 3, 7) };
static const unsigned char plat_colors[] = { C_PLAT_SLOW, C_PLAT_MID, C_PLAT_FAST };
static const char plat_chars[] = "poP";
#pragma rodata-name (pop)

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

#pragma code-name (push, "PROG0")
#pragma rodata-name (push, "PROG0")

static void draw_glyph(unsigned char g, unsigned char x, unsigned char y, unsigned char pair) {
    const unsigned char* p = glyph_rects + glyph_start[g] * 5;
    unsigned char n = glyph_start[g + 1] - glyph_start[g];
    while (n--) {
        gram_rect(x + p[0], y + p[1], p[2], p[3], p[4] == C_PAIR ? pair : p[4]);
        p += 5;
    }
}

static void draw_tile(unsigned char r, unsigned char c, unsigned char x) {
    char t = map[r][c];
    unsigned char y = r << 3, g;
    if (t == '#') {
        gram_rect(x, y, (c < MAP_W - 1 && map[r][c + 1] == '#') ? 8 : 7, 7, C_STONE);
    } else if (t == 'C') {
        draw_glyph(GLYPH_C, x, y, 0);
    } else if (t >= '1' && t <= '4') {
        draw_glyph(GLYPH_TELE, x, y, pair_colors[t - '1']);
    } else {
        for (g = 0; glyph_chars[g]; ++g) {
            if (glyph_chars[g] == t) {
                draw_glyph(g, x, y, 0);
                break;
            }
        }
    }
}

static void plat_next(unsigned char i);
static unsigned char plat_hit();

static void load_level_banked() {
    unsigned char r, c, g;
    char t;
    hearts_left = 0;
    keys_total = 0;
    movers = 0;
    vx = 0;
    vy = 0;
    dash_timer = 0;
    shake_timer = 0;
    tele_cooldown = 0;
    platforms = 0;
    for (r = 0; r < MAP_H; ++r) {
        for (c = 0; c < MAP_W; ++c) {
            t = levels[level][r][c];
            if (t == 'S') {
                bx = (int)((c << 3) + 1) << SUB;
                by = (int)((r << 3) + 2) << SUB;
                t = '.';
            } else if (t == 'h') {
                ++hearts_left;
            } else if (t == 'k') {
                ++keys_total;
            } else if (t == 'm') {
                if (movers < MAX_MOVERS) {
                    mover_x[movers] = c << 3;
                    mover_y[movers] = r << 3;
                    mover_dir[movers] = 1;
                    ++movers;
                }
                t = '.';
            } else {
                for (g = 0; plat_chars[g]; ++g) {
                    if (plat_chars[g] == t && platforms < MAX_PLATFORMS) {
                        plat_x[platforms] = c << 3;
                        plat_y[platforms] = r << 3;
                        plat_tr[platforms] = plat_pr[platforms] = r;
                        plat_tc[platforms] = plat_pc[platforms] = c;
                        plat_speed[platforms] = g;
                        ++platforms;
                        t = '=';
                    }
                }
            }
            map[r][c] = t;
        }
    }
    keys_left = keys_total;
    gram_begin();
    for (r = 0; r < MAP_H; ++r) {
        gram_rect(0, r << 3, 128, 8, C_BG);
        for (c = 0; c < MAP_W; ++c) {
            draw_tile(r, c, c << 3);
        }
    }
    gram_rect(0, GLYPH_Y, 48, 8, 0);
    draw_glyph(0, 0, GLYPH_Y, 0);
    draw_glyph(1, 8, GLYPH_Y, 0);
    draw_glyph(2, 16, GLYPH_Y, 0);
    for (g = 0; g < 3; ++g) draw_glyph(GLYPH_PLAT, 24 + (g << 3), GLYPH_Y, plat_colors[g]);
    gram_end();
    for (g = 0; g < platforms; ++g) plat_next(g);
}

#pragma code-name (pop)
#pragma rodata-name (pop)

static void redraw_tile(unsigned char r, unsigned char c, unsigned char dx) {
    push_rom_bank();
    change_rom_bank(BANK_PROG0);
    gram_begin();
    gram_rect(c << 3, r << 3, 8, 8, C_BG);
    draw_tile(r, c, (c << 3) + dx);
    gram_end();
    pop_rom_bank();
}

static void clear_tile(unsigned char r, unsigned char c) {
    map[r][c] = '.';
    redraw_tile(r, c, 0);
}

static void load_level() {
    push_rom_bank();
    change_rom_bank(BANK_PROG0);
    load_level_banked();
    pop_rom_bank();
}

static char tile_at(unsigned char px, unsigned char py) {
    py >>= 3;
    if (py >= MAP_H) return '.';
    return map[py][px >> 3];
}

static bool is_solid(char t) {
    return t == '#' || t == 'b' || t == '^' || t == '<' || t == '>' || t == 'L' || t == 'v' || t == 'c' || t == 'C';
}

static void bounce_on(unsigned char px, unsigned char py) {
    unsigned char r = py >> 3, c = px >> 3;
    vy = -BOUNCE;
    switch (map[r][c]) {
    case '^':
        vy = -SUPER_BOUNCE;
        break;
    case 'v':
        vy = -LOW_BOUNCE;
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
        break;
    case 'c':
        if (shake_timer) redraw_tile(shake_r, shake_c, 0);
        map[r][c] = 'C';
        shake_r = r;
        shake_c = c;
        shake_timer = SHAKE_FRAMES;
        break;
    case 'C':
        if (shake_timer && shake_r == r && shake_c == c) shake_timer = 0;
        clear_tile(r, c);
        break;
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
    l = plat_hit();
    if (l != NO_PLATFORM) {
        bx = (int)(vx > 0 ? plat_x[l] - BALL_SIZE : plat_x[l] + 8) << SUB;
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
    l = plat_hit();
    if (l != NO_PLATFORM) {
        if (vy > 0) {
            by = (int)(plat_y[l] - BALL_SIZE) << SUB;
            vy = -BOUNCE;
        } else {
            by = (int)(plat_y[l] + 8) << SUB;
            vy = 0;
        }
    }
}

static void die() {
    state = STATE_DEAD;
    timer = 0;
}

static void open_locks() {
    unsigned char r, c;
    for (r = 0; r < MAP_H; ++r) {
        for (c = 0; c < MAP_W; ++c) {
            if (map[r][c] == 'L') clear_tile(r, c);
        }
    }
}

static void teleport(char t, unsigned char r0, unsigned char c0) {
    unsigned char r, c;
    for (r = 0; r < MAP_H; ++r) {
        for (c = 0; c < MAP_W; ++c) {
            if (map[r][c] == t && (r != r0 || c != c0)) {
                bx = (int)((c << 3) + 1) << SUB;
                by = (int)((r << 3) + 1) << SUB;
                tele_cooldown = 1;
                return;
            }
        }
    }
}

static void touch(unsigned char px, unsigned char py) {
    unsigned char r = py >> 3, c = px >> 3;
    char t;
    if (r >= MAP_H) return;
    t = map[r][c];
    if (t == 'h') {
        clear_tile(r, c);
        --hearts_left;
    } else if (t == 'k') {
        clear_tile(r, c);
        if (--keys_left == 0) open_locks();
    } else if (t == 'x') {
        die();
    } else if (t >= '1' && t <= '4') {
        tele_touched = 1;
        if (!tele_cooldown) teleport(t, r, c);
    }
}

static void check_touch() {
    unsigned char l, r, t, b;
    l = (bx >> SUB) + 1;
    r = l + BALL_SIZE - 3;
    t = (by >> SUB) + 1;
    b = t + BALL_SIZE - 3;
    tele_touched = 0;
    touch(l, t);
    touch(r, t);
    touch(l, b);
    touch(r, b);
    tele_cooldown = tele_touched;
}

static void update_movers() {
    unsigned char i, x, y, l, t;
    l = bx >> SUB;
    t = by >> SUB;
    for (i = 0; i < movers; ++i) {
        x = mover_x[i];
        y = mover_y[i];
        if (timer & 1) {
            if (mover_dir[i] > 0) {
                if (x >= 120 || is_solid(tile_at(x + 8, y + 3))) mover_dir[i] = -1;
                else ++x;
            } else {
                if (x == 0 || is_solid(tile_at(x - 1, y + 3))) mover_dir[i] = 1;
                else --x;
            }
            mover_x[i] = x;
        }
        if (l + BALL_SIZE > x + 1 && l < x + 6 && t + BALL_SIZE > y + 1 && t < y + 6) die();
    }
}

static const signed char step_r[] = { -1, 1, 0, 0 };
static const signed char step_c[] = { 0, 0, -1, 1 };

static void plat_next(unsigned char i) {
    unsigned char r = plat_tr[i], c = plat_tc[i], k, nr, nc;
    for (k = 0; k < 4; ++k) {
        nr = r + step_r[k];
        nc = c + step_c[k];
        if (nr < MAP_H && nc < MAP_W && map[nr][nc] == '=' && (nr != plat_pr[i] || nc != plat_pc[i])) {
            plat_pr[i] = r;
            plat_pc[i] = c;
            plat_tr[i] = nr;
            plat_tc[i] = nc;
            return;
        }
    }
    plat_tr[i] = plat_pr[i];
    plat_tc[i] = plat_pc[i];
    plat_pr[i] = r;
    plat_pc[i] = c;
}

static unsigned char plat_hit() {
    unsigned char i, l = bx >> SUB, t = by >> SUB;
    for (i = 0; i < platforms; ++i) {
        if (l < plat_x[i] + 8 && l + BALL_SIZE > plat_x[i] && t < plat_y[i] + 8 && t + BALL_SIZE > plat_y[i]) return i;
    }
    return NO_PLATFORM;
}

static void move_platforms() {
    unsigned char i, steps, x, y, tx, ty;
    signed char dx, dy;
    for (i = 0; i < platforms; ++i) {
        x = plat_x[i];
        y = plat_y[i];
        dx = dy = 0;
        steps = plat_speed[i] ? plat_speed[i] : (timer & 1);
        while (steps--) {
            tx = plat_tc[i] << 3;
            ty = plat_tr[i] << 3;
            if (x == tx && y == ty) {
                plat_next(i);
                tx = plat_tc[i] << 3;
                ty = plat_tr[i] << 3;
            }
            dx = tx > x ? 1 : (tx < x ? -1 : 0);
            dy = ty > y ? 1 : (ty < y ? -1 : 0);
            x += dx;
            y += dy;
        }
        plat_x[i] = x;
        plat_y[i] = y;
        if (plat_hit() == i) {
            if (dy < 0) {
                by = (int)(y - BALL_SIZE) << SUB;
                vy = -BOUNCE;
            } else if (dy > 0) {
                by = (int)(y + 8) << SUB;
                if (vy < 0) vy = 0;
            } else if (dx > 0) {
                bx = (int)(x + 8) << SUB;
            } else if (dx < 0) {
                bx = (int)(x - BALL_SIZE) << SUB;
            }
        }
    }
}

static void update_shake() {
    if (!shake_timer) return;
    --shake_timer;
    redraw_tile(shake_r, shake_c, (shake_timer & 2) ? 1 : 0);
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
    move_platforms();
    move_x();
    move_y();
    if ((by >> SUB) >= (MAP_H << 3) - 4) {
        state = STATE_DEAD;
        timer = 0;
        return;
    }
    check_touch();
    update_movers();
    update_shake();
    if (state == STATE_PLAY && hearts_left == 0 && keys_left == 0) {
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
    unsigned char i;
    queue_draw_box(1, 7, SCREEN_WIDTH - 2, PF_Y - 7, C_BG);
    queue_draw_sprite(0, PF_Y, 64, MAP_H << 3, 0, 0, LEVEL_SLOT);
    queue_draw_sprite(64, PF_Y, 64, MAP_H << 3, 64, 0, LEVEL_SLOT);
    draw_text(4, 8, "LEVEL");
    draw_text(52, 8, two_digits(level + 1));
    if (keys_total) {
        queue_draw_sprite(70, 7, 8, 8, 16, GLYPH_Y, LEVEL_SLOT);
        draw_text(78, 8, two_digits(keys_left));
    }
    queue_draw_sprite(97, 8, 8, 8, 8, GLYPH_Y, LEVEL_SLOT);
    draw_text(108, 8, two_digits(hearts_left));
    for (i = 0; i < movers; ++i) {
        queue_draw_sprite(mover_x[i], PF_Y + mover_y[i], 8, 8, 0, GLYPH_Y, LEVEL_SLOT);
    }
    for (i = 0; i < platforms; ++i) {
        queue_draw_sprite(plat_x[i], PF_Y + plat_y[i], 8, 8, 24 + (plat_speed[i] << 3), GLYPH_Y, LEVEL_SLOT);
    }
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
