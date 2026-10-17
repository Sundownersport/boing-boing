// Live preview for the GameTank Tools VS Code extension.
// A JavaScript copy of the game logic in src/main.c; keep the two in step.

const PLAY = 1, DEAD = 2, CLEAR = 3, WIN = 4;
const PAIR = 1;
const GLYPH_Y = 104;

let K, levels, level, map, gram, hearts, keysLeft, keysTotal, state, timer, bx, by, vx, vy, dash;
let shake, teleCooldown, teleTouched, movers, platforms;
const PLAT_CHARS = 'poP';
const STEPS = [[-1, 0], [1, 0], [0, -1], [0, 1]];

function glyphs() {
    return {
        x: [[2, 0, 1, 5, K.C_SPIKE], [0, 2, 5, 1, K.C_SPIKE], [1, 1, 3, 3, K.C_SPIKE], [0, 0, 1, 1, K.C_SPIKE],
            [4, 0, 1, 1, K.C_SPIKE], [0, 4, 1, 1, K.C_SPIKE], [4, 4, 1, 1, K.C_SPIKE]],
        h: [[1, 0, 1, 1, K.C_HEART], [3, 0, 1, 1, K.C_HEART], [0, 1, 5, 2, K.C_HEART], [1, 3, 3, 1, K.C_HEART],
            [2, 4, 1, 1, K.C_HEART], [1, 1, 1, 1, K.C_WHITE]],
        k: [[0, 1, 2, 1, K.C_KEY], [0, 2, 1, 1, K.C_KEY], [0, 3, 2, 1, K.C_KEY], [2, 2, 3, 1, K.C_KEY],
            [3, 3, 1, 1, K.C_KEY]],
        b: [[0, 0, 5, 5, K.C_BRICK], [1, 2, 3, 1, K.C_BRICK_DARK]],
        '^': [[0, 0, 5, 5, K.C_SPRING], [1, 1, 3, 1, K.C_WHITE]],
        '>': [[0, 0, 5, 5, K.C_ARROW], [3, 1, 1, 3, K.C_WHITE]],
        '<': [[0, 0, 5, 5, K.C_ARROW], [1, 1, 1, 3, K.C_WHITE]],
        L: [[0, 0, 5, 5, K.C_KEY], [2, 1, 1, 3, K.C_BG]],
        v: [[0, 0, 5, 5, K.C_PAD], [1, 3, 3, 1, K.C_WHITE]],
        c: [[0, 0, 5, 5, K.C_CRUMBLE], [1, 1, 1, 1, K.C_CRACK], [3, 2, 1, 1, K.C_CRACK], [1, 3, 1, 1, K.C_CRACK],
            [3, 4, 1, 1, K.C_CRACK]],
        C: [[0, 0, 5, 5, K.C_CRUMBLE], [2, 0, 1, 2, K.C_CRACK], [1, 2, 1, 1, K.C_CRACK], [2, 3, 1, 1, K.C_CRACK],
            [3, 4, 1, 1, K.C_CRACK]],
        tele: [[1, 0, 3, 1, PAIR], [1, 4, 3, 1, PAIR], [0, 1, 1, 3, PAIR], [4, 1, 1, 3, PAIR], [2, 2, 1, 1, K.C_WHITE]],
        plat: [[0, 0, 5, 5, PAIR], [1, 1, 3, 1, K.C_WHITE]],
    };
}

function platColors() {
    return [K.C_PLAT_SLOW, K.C_PLAT_MID, K.C_PLAT_FAST];
}

function pairColors() {
    return [K.COL(4, 3, 5), K.COL(6, 3, 6), K.COL(3, 3, 6), K.COL(0, 3, 7)];
}

function rect(g, x, y, w, h, c) {
    for (let j = 0; j < h; ++j) {
        const row = y + j;
        if (row >= 128) break;
        for (let i = 0; i < w; ++i) if (x + i < 128) g[row * 128 + x + i] = c;
    }
}

function drawGlyph(g, name, x, y, pair) {
    for (const [gx, gy, w, h, c] of glyphs()[name]) rect(g, x + gx, y + gy, w, h, c === PAIR ? pair : c);
}

function isTele(t) {
    return t >= '1' && t <= '4';
}

function drawTile(g, m, r, c, x = c * K.TILE, y = r * K.TILE) {
    const t = m[r][c];
    if (t === '#') rect(g, x, y, (c < K.MAP_W - 1 && m[r][c + 1] === '#') ? K.TILE : K.TILE - 1, K.TILE - 1, K.C_STONE);
    else if (isTele(t)) drawGlyph(g, 'tele', x, y, pairColors()[t.charCodeAt(0) - 49]);
    else if (glyphs()[t]) drawGlyph(g, t, x, y, 0);
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
    const g = new Uint8Array(128 * 128);
    for (let r = 0; r < K.MAP_H; ++r) {
        rect(g, 0, r * K.TILE, K.PF_W, K.TILE, K.C_BG);
        for (let c = 0; c < K.MAP_W; ++c) drawTile(g, m, r, c);
    }
    drawGlyph(g, 'x', 0, GLYPH_Y, 0);
    drawGlyph(g, 'h', 8, GLYPH_Y, 0);
    drawGlyph(g, 'k', 16, GLYPH_Y, 0);
    platColors().forEach((c, i) => drawGlyph(g, 'plat', 24 + i * 8, GLYPH_Y, c));
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
    hearts = keysTotal = 0;
    vx = vy = dash = 0;
    shake = null;
    teleCooldown = false;
    movers = [];
    platforms = [];
    const s = startPos(map);
    bx = (s.c * K.TILE + 1) << K.SUB;
    by = (s.r * K.TILE + K.TILE - K.BALL_SIZE) << K.SUB;
    map.forEach((row, r) => {
        for (let c = 0; c < row.length; ++c) {
            const t = row[c];
            if (t === 'S') row[c] = '.';
            else if (t === 'h') ++hearts;
            else if (t === 'k') ++keysTotal;
            else if (t === 'm') {
                if (movers.length < K.MAX_MOVERS) movers.push({ x: c * K.TILE, y: r * K.TILE, dir: 1 });
                row[c] = '.';
            } else if (PLAT_CHARS.includes(t)) {
                if (platforms.length < K.MAX_PLATFORMS) {
                    platforms.push({ x: c * K.TILE, y: r * K.TILE, speed: PLAT_CHARS.indexOf(t), tr: r, tc: c, pr: r, pc: c });
                }
                row[c] = '=';
            }
        }
    });
    platforms.forEach(platNext);
    keysLeft = keysTotal;
    gram = renderGram(map);
    state = PLAY;
    timer = 0;
}

function redrawTile(r, c, dx) {
    rect(gram, c * K.TILE, r * K.TILE, K.TILE, K.TILE, K.C_BG);
    drawTile(gram, map, r, c, c * K.TILE + dx, r * K.TILE);
}

function clearTile(r, c) {
    map[r][c] = '.';
    redrawTile(r, c, 0);
}

function pxTile(p) {
    return Math.floor((p & 255) / K.TILE);
}

function tileStart(p) {
    return pxTile(p) * K.TILE;
}

function tileAt(px, py) {
    py = pxTile(py);
    if (py >= K.MAP_H) return '.';
    return map[py][pxTile(px)];
}

function isSolid(t) {
    return '#b^<>LvcC'.includes(t);
}

function bounceOn(px, py) {
    const r = pxTile(py), c = pxTile(px);
    vy = -K.BOUNCE;
    switch (map[r][c]) {
    case '^': vy = -K.SUPER_BOUNCE; break;
    case 'v': vy = -K.LOW_BOUNCE; break;
    case '>': vy = 0; vx = K.DASH_SPEED; dash = K.DASH_FRAMES; break;
    case '<': vy = 0; vx = -K.DASH_SPEED; dash = K.DASH_FRAMES; break;
    case 'b': clearTile(r, c); break;
    case 'c':
        if (shake) redrawTile(shake.r, shake.c, 0);
        map[r][c] = 'C';
        shake = { r, c, t: K.SHAKE_FRAMES };
        break;
    case 'C':
        if (shake && shake.r === r && shake.c === c) shake = null;
        clearTile(r, c);
        break;
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
        bx = (tileStart(r) - B) << K.SUB; vx = 0; dash = 0;
    } else if (vx < 0 && (isSolid(tileAt(l, t)) || isSolid(tileAt(l, b)))) {
        bx = (tileStart(l) + K.TILE) << K.SUB; vx = 0; dash = 0;
    }
    const p = platHit();
    if (p) {
        bx = (vx > 0 ? p.x - B : p.x + K.TILE) << K.SUB;
        vx = 0; dash = 0;
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
            by = (tileStart(b) - B) << K.SUB;
            const mid = l + (B >> 1);
            if (isSolid(tileAt(mid, b))) bounceOn(mid, b);
            else if (isSolid(tileAt(l, b))) bounceOn(l, b);
            else bounceOn(r, b);
        }
    } else if (vy < 0) {
        if (isSolid(tileAt(l, t)) || isSolid(tileAt(r, t))) {
            by = (tileStart(t) + K.TILE) << K.SUB; vy = 0;
        }
    }
    const p = platHit();
    if (p) {
        if (vy > 0) { by = (p.y - B) << K.SUB; vy = -K.BOUNCE; }
        else { by = (p.y + K.TILE) << K.SUB; vy = 0; }
    }
}

function platNext(p) {
    for (const [dr, dc] of STEPS) {
        const nr = p.tr + dr, nc = p.tc + dc;
        if (nr >= 0 && nr < K.MAP_H && nc >= 0 && nc < K.MAP_W && map[nr][nc] === '=' && (nr !== p.pr || nc !== p.pc)) {
            p.pr = p.tr; p.pc = p.tc; p.tr = nr; p.tc = nc;
            return;
        }
    }
    [p.tr, p.tc, p.pr, p.pc] = [p.pr, p.pc, p.tr, p.tc];
}

function platHit() {
    const l = (bx >> K.SUB) & 255, t = (by >> K.SUB) & 255, B = K.BALL_SIZE;
    return platforms.find(p => l < p.x + K.TILE && l + B > p.x && t < p.y + K.TILE && t + B > p.y);
}

function movePlatforms() {
    for (const p of platforms) {
        let dx = 0, dy = 0;
        let steps = p.speed ? p.speed : (timer & 1);
        while (steps--) {
            if (p.x === p.tc * K.TILE && p.y === p.tr * K.TILE) platNext(p);
            const tx = p.tc * K.TILE, ty = p.tr * K.TILE;
            dx = Math.sign(tx - p.x);
            dy = Math.sign(ty - p.y);
            p.x += dx;
            p.y += dy;
        }
        if (platHit() === p) {
            if (dy < 0) { by = (p.y - K.BALL_SIZE) << K.SUB; vy = -K.BOUNCE; }
            else if (dy > 0) { by = (p.y + K.TILE) << K.SUB; if (vy < 0) vy = 0; }
            else if (dx > 0) bx = (p.x + K.TILE) << K.SUB;
            else if (dx < 0) bx = (p.x - K.BALL_SIZE) << K.SUB;
        }
    }
}

function die() {
    state = DEAD;
    timer = 0;
}

function openLocks() {
    map.forEach((row, r) => row.forEach((t, c) => { if (t === 'L') clearTile(r, c); }));
}

function teleport(t, r0, c0) {
    for (let r = 0; r < K.MAP_H; ++r) {
        for (let c = 0; c < K.MAP_W; ++c) {
            if (map[r][c] === t && (r !== r0 || c !== c0)) {
                bx = (c * K.TILE + 1) << K.SUB;
                by = (r * K.TILE + K.TILE - K.BALL_SIZE) << K.SUB;
                teleCooldown = true;
                return;
            }
        }
    }
}

function touch(px, py) {
    const r = pxTile(py), c = pxTile(px);
    if (r >= K.MAP_H) return;
    const t = map[r][c];
    if (t === 'h') { clearTile(r, c); --hearts; }
    else if (t === 'k') { clearTile(r, c); if (--keysLeft === 0) openLocks(); }
    else if (t === 'x') die();
    else if (isTele(t)) { teleTouched = true; if (!teleCooldown) teleport(t, r, c); }
}

function checkTouch() {
    const l = (bx >> K.SUB) + 1, r = l + K.BALL_SIZE - 3;
    const t = (by >> K.SUB) + 1, b = t + K.BALL_SIZE - 3;
    teleTouched = false;
    touch(l, t); touch(r, t); touch(l, b); touch(r, b);
    teleCooldown = teleTouched;
}

function updateMovers() {
    const l = (bx >> K.SUB) & 255, t = (by >> K.SUB) & 255, B = K.BALL_SIZE;
    for (const m of movers) {
        if (timer & 1) {
            if (m.dir > 0) {
                if (m.x >= K.PF_W - K.TILE || isSolid(tileAt(m.x + K.TILE, m.y + (K.TILE >> 1)))) m.dir = -1;
                else ++m.x;
            } else if (m.x === 0 || isSolid(tileAt(m.x - 1, m.y + (K.TILE >> 1)))) m.dir = 1;
            else --m.x;
        }
        if (l + B > m.x + 1 && l < m.x + K.TILE - 2 && t + B > m.y + 1 && t < m.y + K.TILE - 2) die();
    }
}

function updateShake() {
    if (!shake) return;
    --shake.t;
    redrawTile(shake.r, shake.c, (shake.t & 2) ? 1 : 0);
    if (!shake.t) shake = null;
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

function drawLevel(gt, g, n, heartCount, keyCount, keyTotal, moverList, platList) {
    gt.box(1, 7, 126, K.PF_Y - 7, K.C_HUD);
    gt.box(1, K.PF_Y + K.PF_H, 126, 120 - K.PF_Y - K.PF_H, K.C_HUD);
    gt.blit(g, 128, K.PF_X, K.PF_Y, 0, 0, K.PF_W, K.PF_H);
    gt.text(4, 8, 'LEVEL');
    gt.text(52, 8, two(n + 1));
    if (keyTotal) {
        gt.blit(g, 128, 71, 9, 16, GLYPH_Y, K.TILE, K.TILE);
        gt.text(78, 8, two(keyCount));
    }
    gt.blit(g, 128, 99, 9, 8, GLYPH_Y, K.TILE, K.TILE);
    gt.text(108, 8, two(heartCount));
    for (const m of moverList) gt.blit(g, 128, K.PF_X + m.x, K.PF_Y + m.y, 0, GLYPH_Y, K.TILE, K.TILE);
    for (const p of platList) gt.blit(g, 128, K.PF_X + p.x, K.PF_Y + p.y, 24 + p.speed * 8, GLYPH_Y, K.TILE, K.TILE);
}

function drawTrack(gt, m) {
    m.forEach((row, r) => row.forEach((t, c) => {
        if (t !== '=') return;
        const x = K.PF_X + c * K.TILE, y = K.PF_Y + r * K.TILE;
        gt.box(x + 2, y + 2, 2, 2, K.C_TRACK);
        for (const [dr, dc] of STEPS) {
            const n = m[r + dr] && m[r + dr][c + dc];
            if (n === '=') gt.box(x + 2 + dc * 2, y + 2 + dr * 2, 2, 2, K.C_TRACK);
        }
    }));
}

function border(gt) {
    gt.box(0, 0, 127, 7, 0);
    gt.box(0, 7, 1, 121, 0);
    gt.box(127, 0, 1, 120, 0);
    gt.box(1, 120, 127, 8, 0);
}

const TILES = '.#b^><xhSkLvcm1234=poP';

GameTank.defineGame({
    source: 'src/main.c',
    requires: ['C_BG', 'C_HUD', 'C_STONE', 'C_BRICK', 'C_BRICK_DARK', 'C_SPRING', 'C_ARROW', 'C_SPIKE', 'C_HEART', 'C_KEY',
        'C_PAD', 'C_CRUMBLE', 'C_CRACK', 'C_WHITE', 'TILE', 'MAP_W', 'MAP_H', 'PF_W', 'PF_H', 'PF_X', 'PF_Y',
        'BALL_SIZE', 'SUB',
        'MAX_BX', 'GRAVITY', 'MAX_FALL', 'BOUNCE', 'SUPER_BOUNCE', 'LOW_BOUNCE', 'ACCEL', 'FRICTION', 'MAX_VX',
        'DASH_SPEED', 'DASH_FRAMES', 'SHAKE_FRAMES', 'MAX_MOVERS', 'C_PLAT_SLOW', 'C_PLAT_MID', 'C_PLAT_FAST',
        'MAX_PLATFORMS'],

    load(gt) {
        K = { ...gt.defines, COL: (h, s, l) => gt.color(h, s, l), C_TRACK: gt.color(5, 1, 4) };
        levels = gt.table('levels');
        levels.forEach((l, n) => {
            const name = `Level ${n + 1}`;
            if (l.rows.length !== K.MAP_H) gt.problem(`${name}: ${l.rows.length} rows, expected ${K.MAP_H}`);
            for (const r of l.rows) {
                if (r.text.length !== K.MAP_W) gt.problem(`${name}, line ${r.line + 1}: ${r.text.length} chars, expected ${K.MAP_W}`);
                const bad = [...r.text].find(ch => !TILES.includes(ch));
                if (bad) gt.problem(`${name}, line ${r.line + 1}: unknown tile '${bad}'`);
            }
            const all = l.rows.map(r => r.text).join('');
            const count = ch => all.split(ch).length - 1;
            if (count('S') !== 1) gt.problem(`${name}: ${count('S')} start points (S), expected 1`);
            if (!all.includes('h')) gt.problem(`${name}: no hearts, it would be cleared instantly`);
            if (count('L') && !count('k')) gt.problem(`${name}: has locks (L) but no keys (k)`);
            if (count('m') > K.MAX_MOVERS) gt.problem(`${name}: ${count('m')} moving spikes, only ${K.MAX_MOVERS} will appear`);
            const plats = [...all].filter(ch => PLAT_CHARS.includes(ch)).length;
            if (plats > K.MAX_PLATFORMS) gt.problem(`${name}: ${plats} platforms, only ${K.MAX_PLATFORMS} will move`);
            const grid = l.rows.map(r => [...r.text]);
            const onTrack = ch => ch === '=' || PLAT_CHARS.includes(ch);
            grid.forEach((row, r) => row.forEach((ch, c) => {
                if (!onTrack(ch)) return;
                const links = STEPS.filter(([dr, dc]) => grid[r + dr] && onTrack(grid[r + dr][c + dc])).length;
                if (links > 2) gt.problem(`${name}, line ${l.rows[r].line + 1}: track splits at column ${c + 1}; platforms always take the first branch`);
                if (links === 0 && ch !== '=') gt.problem(`${name}, line ${l.rows[r].line + 1}: platform at column ${c + 1} has no track next to it, so it won't move`);
            }));
            for (const d of '1234') {
                if (count(d) && count(d) !== 2) gt.problem(`${name}: teleporter ${d} appears ${count(d)} times, needs exactly 2`);
            }
        });
        if (map) {
            const g = renderGram(map);
            for (let i = 0; i < g.length; ++i) gram[i] = g[i];
        }
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
            movePlatforms();
            moveX();
            moveY();
            if ((by >> K.SUB) >= K.PF_H - 3) { die(); break; }
            checkTouch();
            updateMovers();
            updateShake();
            if (state === PLAY && hearts === 0 && keysLeft === 0) { state = CLEAR; timer = 0; }
            break;
        case DEAD:
            if (timer === 45) loadLevel();
            break;
        case CLEAR:
            if (timer === 60) {
                if (level + 1 >= levels.length) state = WIN;
                else { ++level; loadLevel(); }
            }
            break;
        case WIN:
            if (pressed.start) { level = 0; loadLevel(); }
            break;
        }
    },

    draw(gt) {
        const ball = () => drawBall(gt, K.PF_X + (bx >> K.SUB), K.PF_Y + (by >> K.SUB));
        const scene = () => drawLevel(gt, gram, level, hearts, keysLeft, keysTotal, movers, platforms);
        switch (state) {
        case PLAY:
            scene();
            ball();
            break;
        case DEAD:
            if (timer < 4) gt.clear(K.C_SPIKE);
            else scene();
            if (timer & 4) ball();
            break;
        case CLEAR:
            scene();
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
        let heartCount = 0, keyCount = 0;
        const moverList = [], platList = [];
        m.forEach((row, r) => {
            for (let c = 0; c < row.length; ++c) {
                if (row[c] === 'S') row[c] = '.';
                else if (row[c] === 'h') ++heartCount;
                else if (row[c] === 'k') ++keyCount;
                else if (row[c] === 'm') { moverList.push({ x: c * K.TILE, y: r * K.TILE }); row[c] = '.'; }
                else if (PLAT_CHARS.includes(row[c])) { platList.push({ x: c * K.TILE, y: r * K.TILE, speed: PLAT_CHARS.indexOf(row[c]) }); row[c] = '='; }
            }
        });
        gt.clear(K.C_BG);
        const g = renderGram(m);
        drawLevel(gt, g, n, heartCount, keyCount, keyCount, moverList, []);
        drawTrack(gt, m);
        for (const p of platList) gt.blit(g, 128, K.PF_X + p.x, K.PF_Y + p.y, 24 + p.speed * 8, GLYPH_Y, K.TILE, K.TILE);
        drawBall(gt, K.PF_X + s.c * K.TILE + 1, K.PF_Y + s.r * K.TILE + K.TILE - K.BALL_SIZE);
        border(gt);
    },

    editor: {
        x: () => K.PF_X,
        y: () => K.PF_Y,
        cellW: () => K.TILE,
        cellH: () => K.TILE,
        empty: '.',
        unique: ['S'],
        tiles: [
            { ch: '.', name: 'Empty' },
            { ch: '#', name: 'Stone' },
            { ch: 'b', name: 'Brick' },
            { ch: 'c', name: 'Crumble' },
            { ch: '^', name: 'Spring' },
            { ch: 'v', name: 'Low pad' },
            { ch: '>', name: 'Dash right' },
            { ch: '<', name: 'Dash left' },
            { ch: 'x', name: 'Spikes' },
            { ch: 'm', name: 'Moving spike' },
            { ch: 'h', name: 'Heart' },
            { ch: 'k', name: 'Key' },
            { ch: 'L', name: 'Lock' },
            { ch: '1', name: 'Teleporter 1' },
            { ch: '2', name: 'Teleporter 2' },
            { ch: '3', name: 'Teleporter 3' },
            { ch: '4', name: 'Teleporter 4' },
            { ch: '=', name: 'Track' },
            { ch: 'p', name: 'Platform slow' },
            { ch: 'o', name: 'Platform medium' },
            { ch: 'P', name: 'Platform fast' },
            { ch: 'S', name: 'Start' },
        ],
        drawTile(gt, ch) {
            gt.box(0, 0, K.TILE, K.TILE, K.C_BG);
            if (ch === 'S') { drawBall(gt, 1, K.TILE - K.BALL_SIZE); return; }
            if (ch === '=') { gt.box(0, 2, K.TILE, 2, K.C_TRACK); return; }
            if (PLAT_CHARS.includes(ch)) {
                const g = new Uint8Array(128 * 128);
                drawGlyph(g, 'plat', 0, 0, platColors()[PLAT_CHARS.indexOf(ch)]);
                gt.blit(g, 128, 0, 0, 0, 0, K.TILE, K.TILE);
                return;
            }
            const g = new Uint8Array(128 * 128);
            drawTile(g, [[ch === 'm' ? 'x' : ch]], 0, 0);
            gt.blit(g, 128, 0, 0, 0, 0, K.TILE, K.TILE);
        },
    },
});
