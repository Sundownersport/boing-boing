// Live preview for the GameTank Tools VS Code extension.
// A JavaScript copy of the game logic in src/main.c; keep the two in step.

const PLAY = 1, DEAD = 2, CLEAR = 3, WIN = 4;

let K, levels, level, map, gram, hearts, state, timer, bx, by, vx, vy, dash;

function rect(g, x, y, w, h, c) {
    for (let j = 0; j < h; ++j) {
        const row = y + j;
        if (row >= K.MAP_H * 8) break;
        for (let i = 0; i < w; ++i) if (x + i < 128) g[row * 128 + x + i] = c;
    }
}

function drawTile(g, m, r, c, x = c << 3, y = r << 3) {
    switch (m[r][c]) {
    case '#':
        rect(g, x, y, (c < K.MAP_W - 1 && m[r][c + 1] === '#') ? 8 : 7, 7, K.C_STONE);
        break;
    case 'x':
        rect(g, x + 3, y, 1, 7, K.C_SPIKE);
        rect(g, x, y + 3, 7, 1, K.C_SPIKE);
        rect(g, x + 2, y + 2, 3, 3, K.C_SPIKE);
        rect(g, x + 1, y + 1, 1, 1, K.C_SPIKE);
        rect(g, x + 5, y + 1, 1, 1, K.C_SPIKE);
        rect(g, x + 1, y + 5, 1, 1, K.C_SPIKE);
        rect(g, x + 5, y + 5, 1, 1, K.C_SPIKE);
        break;
    case 'b':
        rect(g, x, y, 7, 7, K.C_BRICK);
        rect(g, x + 1, y + 3, 5, 1, K.C_BRICK_DARK);
        break;
    case '^':
        rect(g, x, y, 7, 7, K.C_SPRING);
        rect(g, x + 2, y + 1, 3, 2, K.C_WHITE);
        break;
    case '>':
        rect(g, x, y, 7, 7, K.C_ARROW);
        rect(g, x + 4, y + 2, 2, 3, K.C_WHITE);
        break;
    case '<':
        rect(g, x, y, 7, 7, K.C_ARROW);
        rect(g, x + 1, y + 2, 2, 3, K.C_WHITE);
        break;
    case 'h':
        rect(g, x + 1, y + 1, 2, 1, K.C_HEART);
        rect(g, x + 4, y + 1, 2, 1, K.C_HEART);
        rect(g, x, y + 2, 7, 2, K.C_HEART);
        rect(g, x + 1, y + 4, 5, 1, K.C_HEART);
        rect(g, x + 2, y + 5, 3, 1, K.C_HEART);
        rect(g, x + 3, y + 6, 1, 1, K.C_HEART);
        rect(g, x + 1, y + 2, 1, 1, K.C_WHITE);
        break;
    }
}

function levelMap(n) {
    const rows = levels[n] ? levels[n].rows : [];
    const m = [];
    for (let r = 0; r < K.MAP_H; ++r) {
        const t = rows[r] ? rows[r].text : '';
        m.push([...t.padEnd(K.MAP_W, '.').slice(0, K.MAP_W)]);
    }
    return m;
}

function renderGram(m) {
    const g = new Uint8Array(128 * K.MAP_H * 8);
    for (let r = 0; r < K.MAP_H; ++r) {
        rect(g, 0, r << 3, 128, 8, K.C_BG);
        for (let c = 0; c < K.MAP_W; ++c) drawTile(g, m, r, c);
    }
    return g;
}

function startPos(m) {
    for (let r = 0; r < K.MAP_H; ++r) {
        for (let c = 0; c < K.MAP_W; ++c) {
            if (m[r][c] === 'S') return { r, c };
        }
    }
    return { r: 0, c: 0 };
}

function loadLevel() {
    map = levelMap(level);
    hearts = 0;
    vx = vy = dash = 0;
    const s = startPos(map);
    bx = ((s.c << 3) + 1) << K.SUB;
    by = ((s.r << 3) + 2) << K.SUB;
    for (const row of map) {
        for (let c = 0; c < row.length; ++c) {
            if (row[c] === 'S') row[c] = '.';
            else if (row[c] === 'h') ++hearts;
        }
    }
    gram = renderGram(map);
    state = PLAY;
    timer = 0;
}

function clearTile(r, c) {
    map[r][c] = '.';
    rect(gram, c << 3, r << 3, 8, 8, K.C_BG);
}

function tileAt(px, py) {
    py = (py & 255) >> 3;
    if (py >= K.MAP_H) return '.';
    return map[py][(px & 255) >> 3];
}

function isSolid(t) {
    return t === '#' || t === 'b' || t === '^' || t === '<' || t === '>';
}

function bounceOn(px, py) {
    const r = py >> 3, c = px >> 3;
    switch (map[r][c]) {
    case '^': vy = -K.SUPER_BOUNCE; break;
    case '>': vy = 0; vx = K.DASH_SPEED; dash = K.DASH_FRAMES; break;
    case '<': vy = 0; vx = -K.DASH_SPEED; dash = K.DASH_FRAMES; break;
    case 'b': clearTile(r, c); vy = -K.BOUNCE; break;
    default: vy = -K.BOUNCE;
    }
}

function moveX() {
    const B = K.BALL_SIZE;
    bx += vx;
    if (bx < 0) { bx = 0; vx = 0; dash = 0; }
    else if (bx > K.MAX_BX) { bx = K.MAX_BX; vx = 0; dash = 0; }
    const l = (bx >> K.SUB) & 255, r = (l + B - 1) & 255;
    const t = (by >> K.SUB) & 255, b = (t + B - 1) & 255;
    if (vx > 0 && (isSolid(tileAt(r, t)) || isSolid(tileAt(r, b)))) {
        bx = ((r & 0xF8) - B) << K.SUB; vx = 0; dash = 0;
    } else if (vx < 0 && (isSolid(tileAt(l, t)) || isSolid(tileAt(l, b)))) {
        bx = ((l & 0xF8) + 8) << K.SUB; vx = 0; dash = 0;
    }
}

function moveY() {
    const B = K.BALL_SIZE;
    if (dash) { --dash; vy = 0; }
    else { vy += K.GRAVITY; if (vy > K.MAX_FALL) vy = K.MAX_FALL; }
    by += vy;
    if (by < 0) { by = 0; if (vy < 0) vy = 0; }
    const l = (bx >> K.SUB) & 255, r = (l + B - 1) & 255;
    const t = (by >> K.SUB) & 255, b = (t + B - 1) & 255;
    if (vy > 0) {
        if (isSolid(tileAt(l, b)) || isSolid(tileAt(r, b))) {
            by = ((b & 0xF8) - B) << K.SUB;
            const mid = l + (B >> 1);
            if (isSolid(tileAt(mid, b))) bounceOn(mid, b);
            else if (isSolid(tileAt(l, b))) bounceOn(l, b);
            else bounceOn(r, b);
        }
    } else if (vy < 0) {
        if (isSolid(tileAt(l, t)) || isSolid(tileAt(r, t))) {
            by = ((t & 0xF8) + 8) << K.SUB; vy = 0;
        }
    }
}

function touch(px, py) {
    const r = (py & 255) >> 3, c = (px & 255) >> 3;
    if (r >= K.MAP_H) return;
    if (map[r][c] === 'h') { clearTile(r, c); --hearts; }
    else if (map[r][c] === 'x') { state = DEAD; timer = 0; }
}

function checkTouch() {
    const l = (bx >> K.SUB) + 1, r = l + K.BALL_SIZE - 3;
    const t = (by >> K.SUB) + 1, b = t + K.BALL_SIZE - 3;
    touch(l, t); touch(r, t); touch(l, b); touch(r, b);
}

function steer(held) {
    if (dash) return;
    if (held.left) { vx -= K.ACCEL; if (vx < -K.MAX_VX) vx = -K.MAX_VX; }
    else if (held.right) { vx += K.ACCEL; if (vx > K.MAX_VX) vx = K.MAX_VX; }
    else if (vx > K.FRICTION) vx -= K.FRICTION;
    else if (vx < -K.FRICTION) vx += K.FRICTION;
    else vx = 0;
}

function drawBall(gt, x, y) {
    const B = K.BALL_SIZE;
    gt.box(x + 1, y, B - 2, B, K.C_WHITE);
    gt.box(x, y + 1, B, B - 2, K.C_WHITE);
}

function two(n) {
    return String(Math.floor(n / 10) % 10) + String(n % 10);
}

function drawLevel(gt, g, n, heartCount) {
    gt.box(1, 7, 126, K.PF_Y - 7, K.C_BG);
    gt.blit(g, 128, 0, K.PF_Y);
    gt.text(4, 8, 'LEVEL');
    gt.text(52, 8, two(n + 1));
    gt.box(98, 9, 2, 1, K.C_HEART);
    gt.box(101, 9, 2, 1, K.C_HEART);
    gt.box(97, 10, 7, 2, K.C_HEART);
    gt.box(98, 12, 5, 1, K.C_HEART);
    gt.box(99, 13, 3, 1, K.C_HEART);
    gt.box(100, 14, 1, 1, K.C_HEART);
    gt.box(98, 10, 1, 1, K.C_WHITE);
    gt.text(108, 8, two(heartCount));
}

function border(gt) {
    gt.box(0, 0, 127, 7, 0);
    gt.box(0, 7, 1, 121, 0);
    gt.box(127, 0, 1, 120, 0);
    gt.box(1, 120, 127, 8, 0);
}

const TILES = '.#b^><xhS';

GameTank.defineGame({
    source: 'src/main.c',
    requires: ['C_BG', 'C_STONE', 'C_BRICK', 'C_BRICK_DARK', 'C_SPRING', 'C_ARROW', 'C_SPIKE', 'C_HEART', 'C_WHITE',
        'MAP_W', 'MAP_H', 'PF_Y', 'NUM_LEVELS', 'BALL_SIZE', 'SUB', 'MAX_BX', 'GRAVITY', 'MAX_FALL', 'BOUNCE',
        'SUPER_BOUNCE', 'ACCEL', 'FRICTION', 'MAX_VX', 'DASH_SPEED', 'DASH_FRAMES'],

    load(gt) {
        K = gt.defines;
        levels = gt.table('levels');
        levels.forEach((l, n) => {
            if (l.rows.length !== K.MAP_H) gt.problem(`Level ${n + 1}: ${l.rows.length} rows, expected ${K.MAP_H}`);
            for (const r of l.rows) {
                if (r.text.length !== K.MAP_W) gt.problem(`Level ${n + 1}, line ${r.line + 1}: ${r.text.length} chars, expected ${K.MAP_W}`);
                const bad = [...r.text].find(ch => !TILES.includes(ch));
                if (bad) gt.problem(`Level ${n + 1}, line ${r.line + 1}: unknown tile '${bad}'`);
            }
            const all = l.rows.map(r => r.text).join('');
            const starts = all.split('S').length - 1;
            if (starts !== 1) gt.problem(`Level ${n + 1}: ${starts} start points (S), expected 1`);
            if (!all.includes('h')) gt.problem(`Level ${n + 1}: no hearts, it would be cleared instantly`);
        });
        if (levels.length !== K.NUM_LEVELS) gt.problem(`NUM_LEVELS is ${K.NUM_LEVELS} but levels[] has ${levels.length}`);
        if (map) gram = renderGram(map);
    },

    levels() {
        return levels;
    },

    start(gt, n) {
        level = n;
        loadLevel();
    },

    step(gt) {
        timer = (timer + 1) & 255;
        const { held, pressed } = gt.input;
        switch (state) {
        case PLAY:
            if (pressed.start) { loadLevel(); break; }
            steer(held);
            moveX();
            moveY();
            if ((by >> K.SUB) >= (K.MAP_H << 3) - 4) { state = DEAD; timer = 0; break; }
            checkTouch();
            if (state === PLAY && hearts === 0) { state = CLEAR; timer = 0; }
            break;
        case DEAD:
            if (timer === 45) loadLevel();
            break;
        case CLEAR:
            if (timer === 60) {
                if (level + 1 >= Math.min(K.NUM_LEVELS, levels.length)) state = WIN;
                else { ++level; loadLevel(); }
            }
            break;
        case WIN:
            if (pressed.start) { level = 0; loadLevel(); }
            break;
        }
    },

    draw(gt) {
        const ball = () => drawBall(gt, bx >> K.SUB, K.PF_Y + (by >> K.SUB));
        switch (state) {
        case PLAY:
            drawLevel(gt, gram, level, hearts);
            ball();
            break;
        case DEAD:
            if (timer < 4) gt.clear(K.C_SPIKE);
            else drawLevel(gt, gram, level, hearts);
            if (timer & 4) ball();
            break;
        case CLEAR:
            drawLevel(gt, gram, level, hearts);
            ball();
            gt.text(40, 56, 'CLEAR!');
            break;
        case WIN:
            gt.clear(K.C_BG);
            gt.text(28, 48, 'YOU WIN!');
            gt.text(20, 72, 'PRESS START');
            break;
        }
        border(gt);
    },

    drawLevel(gt, n) {
        const m = levelMap(n);
        const s = startPos(m);
        let count = 0;
        for (const row of m) for (let c = 0; c < row.length; ++c) {
            if (row[c] === 'S') row[c] = '.';
            else if (row[c] === 'h') ++count;
        }
        gt.clear(K.C_BG);
        drawLevel(gt, renderGram(m), n, count);
        drawBall(gt, (s.c << 3) + 1, K.PF_Y + (s.r << 3) + 2);
        border(gt);
    },

    editor: {
        x: 0,
        y: () => K.PF_Y,
        cellW: 8,
        cellH: 8,
        empty: '.',
        unique: ['S'],
        tiles: [
            { ch: '.', name: 'Empty' },
            { ch: '#', name: 'Stone' },
            { ch: 'b', name: 'Brick' },
            { ch: '^', name: 'Spring' },
            { ch: '>', name: 'Dash right' },
            { ch: '<', name: 'Dash left' },
            { ch: 'x', name: 'Spikes' },
            { ch: 'h', name: 'Heart' },
            { ch: 'S', name: 'Start' },
        ],
        drawTile(gt, ch) {
            gt.box(0, 0, 8, 8, K.C_BG);
            if (ch === 'S') { drawBall(gt, 1, 1); return; }
            const g = new Uint8Array(128 * K.MAP_H * 8);
            drawTile(g, [[ch]], 0, 0);
            gt.blit(g, 128, 0, 0, 0, 0, 8, 8);
        },
    },
});
