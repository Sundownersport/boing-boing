#include "gt/gametank.h"
#include "gt/input.h"
#include "gt/gfx/draw_queue.h"
#include "gt/gfx/draw_direct.h"
#include "gt/feature/text/text.h"
#include "gt/feature/persist/persist.h"
#include "gt/banking.h"
#include "gen/bank_nums.h"
#include "gt/audio/audio_coprocessor.h"
#include "sounds.h"

#define COL(h, s, l) (((h) << 5) | ((s) << 3) | (l))

#define C_BG         COL(5, 1, 1)
#define C_HUD        COL(0, 0, 0)
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

#define TILE 6
#define MAP_W 21
#define MAP_H 17
#define PF_W (MAP_W * TILE)
#define PF_H (MAP_H * TILE)
#define PF_X 1
#define PF_Y 16

#define BALL_SIZE 4
#define SUB 6
#define MAX_BX ((PF_W - BALL_SIZE) << SUB)
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
#define MAX_MOVERS 8
#define MAX_PLATFORMS 20
#define NO_PLATFORM 255
#define GLYPH_Y 104

#define LEVEL_SLOT 7

#define SFX(name, channel, priority) play_sound(SND_##name, channel, priority)

#define STATE_TITLE 0
#define STATE_PLAY 1
#define STATE_DEAD 2
#define STATE_CLEAR 3
#define STATE_WIN 4

/*
    To add a level, copy this block (from { to },) into the list below,
    where you want the level to appear. The level count updates itself.

    {
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        "..S...............h..",
        ".....................",
        "#####################",
    },
*/

#pragma rodata-name (push, "PROG1")
static const char levels[][MAP_H][MAP_W + 1] = {
    // Level 1
    {
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        "...........k.........",
        "...............LLLL..",
        "...h...........LhhL..",
        "...............LhhL..",
        "..........LLL..LLLL..",
        "..S.......LhL........",
        "..........LLL........",
        "#####################",
    },
    // Level 2
    {
        "...................hh",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        "...................^.",
        ".....................",
        ".....................",
        ".....................",
        ".......h.......h.....",
        ".....................",
        ".....................",
        "..S................h.",
        "....##............#..",
        "####.......#.......#^",
    },
    // Level 3
    {
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        "......h....h......h..",
        ".....................",
        ".....................",
        ".....................",
        "..S..................",
        "bbbcbbbbccbbbbcbbbbcb",
        "xxxxxxxxxxxxxxxxxxxxx",
    },
    // Level 4
    {
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        "..h................h.",
        ".....................",
        "######m.........#####",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        "..........h..........",
        ".....................",
        ".....................",
        "..S.....^.....^......",
        ".....................",
        "#####################",
    },
    // Level 5
    {
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        "..........#..........",
        "S....................",
        ".......#...........h.",
        ".......#.............",
        ".....................",
        "##..##.....^.........",
        "xxxxxxxxxxxxxxxxxxxxx",
    },
    // Level 6
    {
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        "................LL...",
        "................LL...",
        "...............kLL...",
        "................LL...",
        "................LL...",
        "................LL...",
        "................LL.h.",
        "................LL...",
        "..S.............LL...",
        "######^.........L^...",
        "xxxxxxxxxxxxxxxxxxxxx",
    },
    // Level 7
    {
        ".....................",
        ".....................",
        ".....................",
        "...h..............h..",
        "..####..........####.",
        ".....................",
        ".....................",
        ".....................",
        "..........hh.........",
        ".....................",
        ".......oooo====......",
        ".....................",
        ".....................",
        ".....................",
        "..S....^......^......",
        ".......xxxxxxxx......",
        "########xxxxxx#######",
    },
    // Level 8
    {
        ".....................",
        ".....................",
        ".....................",
        "S....................",
        ".....................",
        ".....................",
        "....................h",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        "..........h..........",
        ".....................",
        "^^xxxxxxxxx#^^xxxxxxx",
    },  
    // Level 9
    {
        "S....................",
        ".....................",
        ".....................",
        ".....................",
        "ccc=============ppp..",
        "ccc..................",
        "..m..................",
        "m...........k.......h",
        "...m...............##",
        ".....................",
        ".....................",
        "...L.................",
        "...L.................",
        "...L.................",
        "...L.................",
        "h..b...ccc..ccc...^..",
        "xxxxxxxxxxxxxxxxxxxxx",
    },
    // Level 10
    {
        "..................h..",
        ".....................",
        ".....................",
        "...================..",
        "...=.................",
        "...=.................",
        "...=.................",
        "...====pp==========..",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        "....pppp=========....",
        ".....................",
        "S..#.................",
        "...#.................",
        "####...............^^",
    }, 
    // Level 11
    {
        ".....................",
        ".....................",
        "..h.#................",
        "....#...bb...........",
        "#####...xx...........",
        "........xx...........",
        ".....bbb..........b..",
        "..........b.......b.^",
        "..............b...b..",
        "..............#...b..",
        "..................b..",
        "..................b.x",
        "..................b.#",
        "..................b.#",
        "S...................#",
        ".......>............#",
        "###xxxxxxxxxxxxxxxx^.",
    },    
    // Level 12
    {
        "....................S",
        ".....................",
        ".....................",
        "..######m.m####x#####",
        ".....................",
        ".....................",
        ".....................",
        "########m.m####x####.",
        "h....................",
        ".....................",
        ".....................",
        "........xx..xx.xh.###",
        "..m.....x#..##.#.....",
        "........x#..##.#.....",
        "........##.x##.......",
        "^^.........#..^......",
        "###xxxx#########xxxxx",
    },
    // Level 13
    {
        "....................h",
        ".....................",
        "................bbbbb",
        "...............b.....",
        "bb..bb....bbbb.......",
        ".....................",
        ".....................",
        "..c...............bb.",
        ".....................",
        ".....................",
        "....bb..bb..bb..bb...",
        ".....................",
        "S....................",
        ".....................",
        ".....................",
        ".....................",
        "###x##xx###xxx#x##xx^",
    },
    // Level 14
    {
        "..S...........h......",
        ".....................",
        ".....................",
        ".....................",
        "....xxx#xxx#xx.......",
        "####xxx#xxx#xx######.",
        ".....................",
        "........m............",
        ".....................",
        ".....................",
        "bbbv..vxvvxvxxx######",
        "....vv..............h",
        ".....................",
        ".....................",
        ".....................",
        "..................b..",
        "pppp============.....",
    },
    // Level 15
    {
        "S..13..2............h",
        ".....................",
        ".......k.............",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        ".....................",
        "....................L",
        "....................L",
        "....m...............^",
        ".....................",
        ".....................",
        ".....................",
        "====================p",
        ".....................",
        "bb13bb2bb............",
    },
    // Level 16
    {
        "h.L....m............k",
        "..Lm.................",
        "bLL.......m..........",
        ".....................",
        ".....................",
        "....................c",
        "..##################x",
        ".....................",
        ".....................",
        ".....................",
        "##vvvvvvvvvvvvvvvvv..",
        "m.hhh.hmh.hhh.hhh.hhm",
        ".....................",
        "..S..................",
        ".....................",
        ".....................",
        "########vvv#########^",
    },
    // Level 17
    {
        "...h.......h........h",
        ".....................",
        "....>................",
        "................====p",
        ".....................",
        ".....................",
        ".............m.......",
        "..^..................",
        ".....................",
        ".....................",
        "......b.....^^.......",
        ".....................",
        ".....................",
        ".....................",
        "..S..................",
        ".....................",
        "bbbbbbbbbbbbbbbbbbbb^",
    },
    // Level 18
    {
        "....................h",
        ".....................",
        ".....................",
        "=======oooo==========",
        ".....................",
        ".....................",
        "bb...................",
        ".....................",
        ".....................",
        ".....................",
        "================ppppp",
        ".....................",
        "....................b",
        "....................b",
        "..S...............h..",
        ".....................",
        "ppppppp==============",
    },
    // Level 19
    {
        "m...h.........h.....S",
        ".....................",
        ".....................",
        "1....................",
        ".....................",
        "#####################",
        "1.........h..........",
        "m...............h....",
        "....m................",
        ".....................",
        "....................2",
        "#####################",
        "h...m...m...........2",
        ".....................",
        ".....................",
        ".....................",
        "bbbbbbbbbbbbbbbbbbbb#",
    },
    // Level 20
    {
        "S....................",
        ".....................",
        ".....................",
        ">>x.........###bbbbb#",
        "....m..m.............",
        ".....................",
        "...m......m..........",
        "......m.m............",
        "..............xxxxxxx",
        "..............xxxxxxx",
        ".....................",
        "......k..............",
        ".....................",
        ".....................",
        ".....................",
        "ooo============....LL",
        "xxxxxxxxxxxxxxxxxxxxh",
    },
    // Level 21
    {
        ".....................",
        ".....................",
        "k...............=pppp",
        "..x.....#......x.....",
        "..x.....x===ppp......",
        "b.......#......==pp==",
        "........#......x.....",
        "....pp===......x.....",
        "...............x===pp",
        "===pp..........x.....",
        "...............x.....",
        "...............xpp===",
        "...............x.....",
        "...............x.LLLL",
        "..S.x..x...x...x.Lh..",
        "....x..x...x^..x.L...",
        "#####################",
    },
    // Level 22
    {
        "h...................h",
        "..m..................",
        ".....................",
        ".........S.......m..b",
        ".....................",
        "........###..........",
        "^....................",
        "ooo==================",
        ".....................",
        "......b.....b........",
        ".....................",
        ".....................",
        ".........b...........",
        ".........#...........",
        "..h...............h..",
        ".....................",
        "x##x##x##x##x##x##x##",
    },
    // Level 23
    {
        "k....................",
        ".....................",
        ".....................",
        ".....................",
        "b....................",
        ".....................",
        "..b....b.....b.......",
        "m....................",
        "..............m......",
        "....P...m.P.....Pb...",
        "2...P.....P.....P....",
        "....P.....P.....P....",
        "....=.....=.....P....",
        "S...=.....=.....P.LLL",
        "....P.....P.....=.Lh.",
        "....P.....P.....=.L..",
        "####x#####x#####2####",
    },
    // Level 24

    {
        "....#.........#.....k",
        "....#h........#..>...",
        "bbbb#.........#......",
        "bbbb#....#....#......",
        "bxbb#....#m...L.m....",
        "b#bb#....#....L....m.",
        "b.bbx=oo=x....L......",
        "b.xb#....#m...L......",
        "b.#x#....#.L..L......",
        "b..##....#LhL.#^....v",
        "bbb.x==ppx....#......",
        "bbb.#....#....#......",
        "bxbb#....#...m#......",
        "b.bb.....#....S......",
        "b.bb.pp==#...........",
        "b.bb.....#..........^",
        "##xcccxxx############",
    },
};

#pragma rodata-name (pop)

#pragma rodata-name (push, "SAVE")
static const unsigned char saved_best = 0;
#pragma rodata-name (pop)

#define NUM_LEVELS (sizeof(levels) / sizeof(levels[0]))

static char map[MAP_H][MAP_W];
static unsigned char hearts_left;
static unsigned char keys_left, keys_total;
static unsigned char level;
static unsigned char start_level, best_level, repeat_timer, cheat_pos;
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

#pragma rodata-name (push, "PROG0")

static SpriteSlot font;
static char num_buf[3];

static const char glyph_chars[] = "xhkb^><Lvc";
#define GLYPH_C 10
#define GLYPH_TELE 11
#define GLYPH_PLAT 12

static const unsigned char glyph_start[] = { 0, 7, 13, 18, 20, 22, 24, 26, 28, 30, 35, 40, 45, 47 };

static const unsigned char glyph_rects[] = {
    2, 0, 1, 5, C_SPIKE,
    0, 2, 5, 1, C_SPIKE,
    1, 1, 3, 3, C_SPIKE,
    0, 0, 1, 1, C_SPIKE,
    4, 0, 1, 1, C_SPIKE,
    0, 4, 1, 1, C_SPIKE,
    4, 4, 1, 1, C_SPIKE,

    1, 0, 1, 1, C_HEART,
    3, 0, 1, 1, C_HEART,
    0, 1, 5, 2, C_HEART,
    1, 3, 3, 1, C_HEART,
    2, 4, 1, 1, C_HEART,
    1, 1, 1, 1, C_WHITE,

    0, 1, 2, 1, C_KEY,
    0, 2, 1, 1, C_KEY,
    0, 3, 2, 1, C_KEY,
    2, 2, 3, 1, C_KEY,
    3, 3, 1, 1, C_KEY,

    0, 0, 5, 5, C_BRICK,
    1, 2, 3, 1, C_BRICK_DARK,

    0, 0, 5, 5, C_SPRING,
    1, 1, 3, 1, C_WHITE,

    0, 0, 5, 5, C_ARROW,
    3, 1, 1, 3, C_WHITE,

    0, 0, 5, 5, C_ARROW,
    1, 1, 1, 3, C_WHITE,

    0, 0, 5, 5, C_KEY,
    2, 1, 1, 3, C_BG,

    0, 0, 5, 5, C_PAD,
    1, 3, 3, 1, C_WHITE,

    0, 0, 5, 5, C_CRUMBLE,
    1, 1, 1, 1, C_CRACK,
    3, 2, 1, 1, C_CRACK,
    1, 3, 1, 1, C_CRACK,
    3, 4, 1, 1, C_CRACK,

    0, 0, 5, 5, C_CRUMBLE,
    2, 0, 1, 2, C_CRACK,
    1, 2, 1, 1, C_CRACK,
    2, 3, 1, 1, C_CRACK,
    3, 4, 1, 1, C_CRACK,

    1, 0, 3, 1, C_PAIR,
    1, 4, 3, 1, C_PAIR,
    0, 1, 1, 3, C_PAIR,
    4, 1, 1, 3, C_PAIR,
    2, 2, 1, 1, C_WHITE,

    0, 0, 5, 5, C_PAIR,
    1, 1, 3, 1, C_WHITE,
};

static const unsigned char pair_colors[] = { COL(4, 3, 5), COL(6, 3, 6), COL(3, 3, 6), COL(0, 3, 7) };
static const unsigned char plat_colors[] = { C_PLAT_SLOW, C_PLAT_MID, C_PLAT_FAST };
static const char plat_chars[] = "poP";

// One colour per letter of stone_chars. The GameTank Tools level editor adds entries here.
static const char stone_chars[] = "#ABDEFGHIJKMNOQRTUVWXYZ";
static const unsigned char stone_colors[] = {
    C_STONE,
    COL(3, 1, 0),
};
#pragma rodata-name (pop)

static const unsigned char* voice_ptr[NUM_FM_CHANNELS];
static unsigned char voice_len[NUM_FM_CHANNELS], voice_priority[NUM_FM_CHANNELS];

static void play_sound(unsigned char id, unsigned char ch, unsigned char priority) {
    if (voice_len[ch] && priority < voice_priority[ch]) return;
    push_rom_bank();
    change_rom_bank(BANK_PROG0);
    voice_ptr[ch] = sound_table[id] + 2;
    voice_len[ch] = sound_table[id][0];
    aram[FEEDBACK_AMT + ch] = sound_table[id][1];
    pop_rom_bank();
    voice_priority[ch] = priority;
}

static void tick_sounds() {
    unsigned char ch, i, op;
    const unsigned char* p;
    push_rom_bank();
    change_rom_bank(BANK_PROG0);
    for (ch = 0; ch < NUM_FM_CHANNELS; ++ch) {
        if (!voice_len[ch]) continue;
        p = voice_ptr[ch];
        op = ch << 2;
        for (i = 0; i < OPS_PER_CHANNEL; ++i) {
            aram[AMPLITUDE + op + i] = p[i];
            aram[PITCH_MSB + op + i] = p[4 + i];
            aram[PITCH_LSB + op + i] = p[8 + i];
        }
        voice_ptr[ch] = p + 12;
        if (--voice_len[ch] == 0) {
            for (i = 0; i < OPS_PER_CHANNEL; ++i) aram[AMPLITUDE + op + i] = 128;
            aram[FEEDBACK_AMT + ch] = 128;
        }
    }
    pop_rom_bank();
}

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
    unsigned char y = r * TILE, g;
    for (g = 0; g < sizeof(stone_colors); ++g) {
        if (stone_chars[g] == t) {
            gram_rect(x, y, (c < MAP_W - 1 && map[r][c + 1] == t) ? TILE : TILE - 1, TILE - 1, stone_colors[g]);
            return;
        }
    }
    if (t == 'C') {
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
            t = map[r][c];
            if (t == 'S') {
                bx = (int)(c * TILE + 1) << SUB;
                by = (int)(r * TILE + TILE - BALL_SIZE) << SUB;
                t = '.';
            } else if (t == 'h') {
                ++hearts_left;
            } else if (t == 'k') {
                ++keys_total;
            } else if (t == 'm') {
                if (movers < MAX_MOVERS) {
                    mover_x[movers] = c * TILE;
                    mover_y[movers] = r * TILE;
                    mover_dir[movers] = 1;
                    ++movers;
                }
                t = '.';
            } else {
                for (g = 0; plat_chars[g]; ++g) {
                    if (plat_chars[g] == t && platforms < MAX_PLATFORMS) {
                        plat_x[platforms] = c * TILE;
                        plat_y[platforms] = r * TILE;
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
        gram_rect(0, r * TILE, PF_W, TILE, C_BG);
        for (c = 0; c < MAP_W; ++c) {
            draw_tile(r, c, c * TILE);
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
    gram_rect(c * TILE, r * TILE, TILE, TILE, C_BG);
    draw_tile(r, c, c * TILE + dx);
    gram_end();
    pop_rom_bank();
}

static void clear_tile(unsigned char r, unsigned char c) {
    map[r][c] = '.';
    redraw_tile(r, c, 0);
}

static void load_level() {
    unsigned char r, c;
    push_rom_bank();
    change_rom_bank(BANK_PROG1);
    for (r = 0; r < MAP_H; ++r) {
        for (c = 0; c < MAP_W; ++c) map[r][c] = levels[level][r][c];
    }
    change_rom_bank(BANK_PROG0);
    load_level_banked();
    pop_rom_bank();
}

#pragma code-name (push, "PROG0")
#pragma rodata-name (push, "PROG0")

#define T6(n) n, n, n, n, n, n
static const unsigned char px_tile[] = {
    T6(0), T6(1), T6(2), T6(3), T6(4), T6(5), T6(6), T6(7), T6(8), T6(9), T6(10),
    T6(11), T6(12), T6(13), T6(14), T6(15), T6(16), T6(17), T6(18), T6(19), T6(20), T6(21),
};

#define TILE_START(p) (px_tile[p] * TILE)

static char tile_at(unsigned char px, unsigned char py) {
    py = px_tile[py];
    if (py >= MAP_H) return '.';
    return map[py][px_tile[px]];
}

static bool is_solid(char t) {
    return (t >= 'A' && t <= 'Z' && t != 'P') || t == '#' || t == 'b' || t == '^' || t == '<' || t == '>' || t == 'v' || t == 'c';
}

static void bounce_on(unsigned char px, unsigned char py) {
    unsigned char r = px_tile[py], c = px_tile[px];
    vy = -BOUNCE;
    switch (map[r][c]) {
    case '^':
        vy = -SUPER_BOUNCE;
        SFX(spring, 1, 1);
        break;
    case 'v':
        vy = -LOW_BOUNCE;
        SFX(lowpad, 1, 1);
        break;
    case '>':
        vy = 0;
        vx = DASH_SPEED;
        dash_timer = DASH_FRAMES;
        SFX(dash, 1, 1);
        break;
    case '<':
        vy = 0;
        vx = -DASH_SPEED;
        dash_timer = DASH_FRAMES;
        SFX(dash, 1, 1);
        break;
    case 'b':
        clear_tile(r, c);
        SFX(break, 1, 1);
        break;
    case 'c':
        SFX(crack, 1, 1);
        if (shake_timer) redraw_tile(shake_r, shake_c, 0);
        map[r][c] = 'C';
        shake_r = r;
        shake_c = c;
        shake_timer = SHAKE_FRAMES;
        break;
    case 'C':
        if (shake_timer && shake_r == r && shake_c == c) shake_timer = 0;
        clear_tile(r, c);
        SFX(break, 1, 1);
        break;
    default:
        SFX(bounce, 0, 0);
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
        bx = (int)(TILE_START(r) - BALL_SIZE) << SUB;
        vx = 0;
        dash_timer = 0;
    } else if (vx < 0 && (is_solid(tile_at(l, t)) || is_solid(tile_at(l, b)))) {
        bx = (int)(TILE_START(l) + TILE) << SUB;
        vx = 0;
        dash_timer = 0;
    }
    l = plat_hit();
    if (l != NO_PLATFORM) {
        bx = (int)(vx > 0 ? plat_x[l] - BALL_SIZE : plat_x[l] + TILE) << SUB;
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
            by = (int)(TILE_START(b) - BALL_SIZE) << SUB;
            mid = l + (BALL_SIZE >> 1);
            if (is_solid(tile_at(mid, b))) bounce_on(mid, b);
            else if (is_solid(tile_at(l, b))) bounce_on(l, b);
            else bounce_on(r, b);
        }
    } else if (vy < 0) {
        if (is_solid(tile_at(l, t)) || is_solid(tile_at(r, t))) {
            by = (int)(TILE_START(t) + TILE) << SUB;
            vy = 0;
        }
    }
    l = plat_hit();
    if (l != NO_PLATFORM) {
        if (vy > 0) {
            SFX(bounce, 0, 0);
            by = (int)(plat_y[l] - BALL_SIZE) << SUB;
            vy = -BOUNCE;
        } else {
            by = (int)(plat_y[l] + TILE) << SUB;
            vy = 0;
        }
    }
}

static void die() {
    SFX(death, 3, 3);
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
                bx = (int)(c * TILE + 1) << SUB;
                by = (int)(r * TILE + TILE - BALL_SIZE) << SUB;
                tele_cooldown = 1;
                SFX(teleport, 1, 2);
                return;
            }
        }
    }
}

static void touch(unsigned char px, unsigned char py) {
    unsigned char r = px_tile[py], c = px_tile[px];
    char t;
    if (r >= MAP_H) return;
    t = map[r][c];
    if (t == 'h') {
        clear_tile(r, c);
        --hearts_left;
        SFX(heart, 2, 1);
    } else if (t == 'k') {
        clear_tile(r, c);
        if (--keys_left == 0) {
            open_locks();
            SFX(unlock, 2, 2);
        } else {
            SFX(key, 2, 1);
        }
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
                if (x >= PF_W - TILE || is_solid(tile_at(x + TILE, y + TILE / 2))) mover_dir[i] = -1;
                else ++x;
            } else {
                if (x == 0 || is_solid(tile_at(x - 1, y + TILE / 2))) mover_dir[i] = 1;
                else --x;
            }
            mover_x[i] = x;
        }
        if (l + BALL_SIZE > x + 1 && l < x + TILE - 2 && t + BALL_SIZE > y + 1 && t < y + TILE - 2) die();
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
        if (l < plat_x[i] + TILE && l + BALL_SIZE > plat_x[i] && t < plat_y[i] + TILE && t + BALL_SIZE > plat_y[i]) return i;
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
            tx = plat_tc[i] * TILE;
            ty = plat_tr[i] * TILE;
            if (x == tx && y == ty) {
                plat_next(i);
                tx = plat_tc[i] * TILE;
                ty = plat_tr[i] * TILE;
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
                by = (int)(y + TILE) << SUB;
                if (vy < 0) vy = 0;
            } else if (dx > 0) {
                bx = (int)(x + TILE) << SUB;
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

static void update_play_banked() {
    if (player1_new_buttons & INPUT_MASK_START) {
        load_level();
        return;
    }
    steer();
    move_platforms();
    move_x();
    move_y();
    if ((by >> SUB) >= PF_H - 3) {
        state = STATE_DEAD;
        timer = 0;
        return;
    }
    check_touch();
    update_movers();
    update_shake();
    if (state == STATE_PLAY && hearts_left == 0 && keys_left == 0) {
        SFX(clear, 3, 3);
        state = STATE_CLEAR;
        timer = 0;
    }
}

static const int cheat_code[] = {
    INPUT_MASK_UP, INPUT_MASK_UP, INPUT_MASK_DOWN, INPUT_MASK_DOWN,
    INPUT_MASK_UP, INPUT_MASK_UP, INPUT_MASK_UP,
    INPUT_MASK_DOWN, INPUT_MASK_DOWN, INPUT_MASK_DOWN,
    INPUT_MASK_A, INPUT_MASK_B, INPUT_MASK_C,
    INPUT_MASK_A, INPUT_MASK_B, INPUT_MASK_C
};

static void check_cheat() {
    if (!player1_new_buttons) return;
    if (player1_new_buttons & cheat_code[cheat_pos]) {
        if (++cheat_pos == sizeof(cheat_code) / sizeof(cheat_code[0])) {
            cheat_pos = 0;
            best_level = NUM_LEVELS - 1;
            SFX(clear, 3, 3);
        }
    } else {
        cheat_pos = (player1_new_buttons & cheat_code[0]) ? 1 : 0;
    }
}

static void pick_level_banked() {
    check_cheat();
    if (player1_new_buttons & (INPUT_MASK_LEFT | INPUT_MASK_RIGHT)) {
        repeat_timer = 20;
    } else if (!(player1_buttons & (INPUT_MASK_LEFT | INPUT_MASK_RIGHT)) || --repeat_timer) {
        return;
    } else {
        repeat_timer = 5;
    }
    if (player1_buttons & INPUT_MASK_LEFT) {
        if (start_level) --start_level;
    } else if (start_level < best_level) {
        ++start_level;
    }
}

static void save_progress_banked() {
    clear_save_sector();
    save_write(&best_level, (void*) &saved_best, 1);
}

#pragma code-name (pop)
#pragma rodata-name (pop)

static void pick_level() {
    push_rom_bank();
    change_rom_bank(BANK_PROG0);
    pick_level_banked();
    pop_rom_bank();
}

static void save_progress() {
    push_rom_bank();
    change_rom_bank(BANK_PROG0);
    save_progress_banked();
    pop_rom_bank();
}

static void load_progress() {
    push_rom_bank();
    change_rom_bank(BANK_SAVE);
    best_level = *(volatile const unsigned char*) &saved_best;
    pop_rom_bank();
    if (best_level >= NUM_LEVELS) best_level = 0;
}

static void update_play() {
    push_rom_bank();
    change_rom_bank(BANK_PROG0);
    update_play_banked();
    pop_rom_bank();
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
    queue_draw_box(1, 7, SCREEN_WIDTH - 2, PF_Y - 7, C_HUD);
    queue_draw_box(1, PF_Y + PF_H, SCREEN_WIDTH - 2, 120 - PF_Y - PF_H, C_HUD);
    queue_draw_sprite(PF_X, PF_Y, 64, PF_H, 0, 0, LEVEL_SLOT);
    queue_draw_sprite(PF_X + 64, PF_Y, PF_W - 64, PF_H, 64, 0, LEVEL_SLOT);
    draw_text(4, 8, "LEVEL");
    draw_text(52, 8, two_digits(level + 1));
    if (keys_total) {
        queue_draw_sprite(71, 9, TILE, TILE, 16, GLYPH_Y, LEVEL_SLOT);
        draw_text(78, 8, two_digits(keys_left));
    }
    queue_draw_sprite(99, 9, TILE, TILE, 8, GLYPH_Y, LEVEL_SLOT);
    draw_text(108, 8, two_digits(hearts_left));
    for (i = 0; i < movers; ++i) {
        queue_draw_sprite(PF_X + mover_x[i], PF_Y + mover_y[i], TILE, TILE, 0, GLYPH_Y, LEVEL_SLOT);
    }
    for (i = 0; i < platforms; ++i) {
        queue_draw_sprite(PF_X + plat_x[i], PF_Y + plat_y[i], TILE, TILE, 24 + (plat_speed[i] << 3), GLYPH_Y, LEVEL_SLOT);
    }
}

static int title_y, title_vy;

static void draw_title() {
    title_vy += GRAVITY;
    title_y += title_vy;
    if (title_y > ((102 - BALL_SIZE) << SUB)) {
        title_y = (102 - BALL_SIZE) << SUB;
        title_vy = -BOUNCE;
    }
    draw_text(16, 32, "BOING BOING!");
    queue_draw_box(40, 102, 48, 7, C_STONE);
    draw_ball(61, title_y >> SUB);
    if (timer & 32) draw_text(20, 64, "PRESS START");
    draw_text(32, 48, "LEVEL");
    draw_text(80, 48, two_digits(start_level + 1));
    if (start_level) draw_text(16, 48, "<");
    if (start_level < best_level) draw_text(104, 48, ">");
}

void main() {
    font = text_load_font();
    load_progress();
    start_level = best_level;
    state = STATE_TITLE;
    title_y = 60 << SUB;
    title_vy = 0;
    timer = 0;

    while (1) {
        update_inputs();
        ++timer;

        switch (state) {
        case STATE_TITLE:
            if ((player1_new_buttons & INPUT_MASK_START) || ((player1_new_buttons & INPUT_MASK_A) && !cheat_pos)) {
                level = start_level;
                load_level();
                state = STATE_PLAY;
            } else {
                pick_level();
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
                    SFX(win, 3, 3);
                } else {
                    if (level > best_level) {
                        best_level = level;
                        start_level = level;
                        save_progress();
                    }
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
            draw_ball(PF_X + (bx >> SUB), PF_Y + (by >> SUB));
            break;
        case STATE_DEAD:
            if (timer < 4) queue_clear_screen(C_SPIKE);
            else draw_level();
            if (timer & 4) draw_ball(PF_X + (bx >> SUB), PF_Y + (by >> SUB));
            break;
        case STATE_CLEAR:
            draw_level();
            draw_ball(PF_X + (bx >> SUB), PF_Y + (by >> SUB));
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
        tick_sounds();
    }
}
