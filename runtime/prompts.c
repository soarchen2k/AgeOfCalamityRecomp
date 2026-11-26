// Xbox and PlayStation button prompts: the game draws the button icons of its prompts and menus from one texture atlas (G1T 3226a433
// in asset/ScreenLayout.rdb.bin: 1024x1024 BC3, one level) and the L and R tabs of its menus from another (G1T
// 8d1f8cb8, 512x256). tools/make_prompts.py redraws the Switch buttons of the player's own textures as Xbox ones and
// writes the result next to romfs-index.bin (input_prompts_folder names the file); romfs.c overlay_init loads the one
// // every file at start (up to PROMPTS_FILES), and the one the settings ask for is active (prompts_select, live):
// vk_texture.c upload puts each texture in place of the game's when the game uploads one whose bytes are the game's
// (same size and FNV-1a hash), and marks it, so a change of the setting uploads it again from the game's bytes. The game's files stay as they are, so a RomFS
// mod that changes one of these textures keeps its own (its hash differs).
//
// File: header <8sII (magic "ACRPRM2\0", texture count, 0), then per texture <IIQQ (width, height, FNV-1a 64 of the
// game's BC3 blocks, size) and the new blocks in rows (the layout of the G1T and of vk_texture.c's staging copy).
#include "prompts.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"
#include "input.h"
#include "platform/platform.h"

enum { MAX_TEX = 8 };
static const char MAGIC[8] = "ACRPRM2";
typedef struct { uint32_t w, h; uint64_t hash, size; uint8_t *data; } PromptTex;
static PromptTex g_files[PROMPTS_FILES][MAX_TEX];
static int g_count[PROMPTS_FILES];
static int g_active = -1;      // the file in use, -1: the game's own buttons
static uint32_t g_gen = 1;     // bumped when g_active changes: marked textures upload again

uint64_t prompts_hash(const uint8_t *p, size_t n) {
    uint64_t h = 0xcbf29ce484222325ull;
    for (size_t i = 0; i < n; i++) h = (h ^ p[i]) * 0x100000001b3ull;
    return h;
}

int prompts_load(int slot, const char *path) {
    if (slot < 0 || slot >= PROMPTS_FILES || g_count[slot]) return 0;
    PromptTex *g_tex = g_files[slot];
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    uint8_t h[16];
    uint32_t n = 0;
    int ok = fread(h, 1, sizeof h, f) == sizeof h && !memcmp(h, MAGIC, 8);
    if (ok) memcpy(&n, h + 8, 4), ok = n >= 1 && n <= MAX_TEX;
    int got = 0;
    for (; ok && got < (int)n; got++) {
        uint8_t e[24];
        uint32_t w = 0, hh = 0;
        uint64_t hash = 0, size = 0;
        ok = fread(e, 1, sizeof e, f) == sizeof e;
        if (ok) {
            memcpy(&w, e, 4), memcpy(&hh, e + 4, 4), memcpy(&hash, e + 8, 8), memcpy(&size, e + 16, 8);
            ok = w && hh && w <= 16384 && hh <= 16384 && size == (uint64_t)((w + 3) / 4) * ((hh + 3) / 4) * 16;
        }
        uint8_t *d = ok ? malloc(size) : NULL;
        ok = d && fread(d, 1, size, f) == size;
        if (!ok) {
            free(d);
            break;
        }
        g_tex[got].w = w, g_tex[got].h = hh, g_tex[got].hash = hash, g_tex[got].size = size, g_tex[got].data = d;
    }
    ok = ok && fgetc(f) == EOF;
    fclose(f);
    if (!ok) {
        for (int i = 0; i < got; i++) free(g_tex[i].data), g_tex[i].data = NULL;
        return 0;
    }
    g_count[slot] = got;
    return 1;
}

void prompts_select(int slot) {
    if (slot >= PROMPTS_FILES || (slot >= 0 && !g_count[slot])) slot = -1;
    if (__atomic_exchange_n(&g_active, slot, __ATOMIC_ACQ_REL) != slot) __atomic_add_fetch(&g_gen, 1, __ATOMIC_RELEASE);
}

uint32_t prompts_gen(void) { return __atomic_load_n(&g_gen, __ATOMIC_ACQUIRE); }

// ---- remapped face buttons: each of the four icons of A, B, X and Y shows the button that now presses it
// The button atlas (1024x1024) holds a white and a blue (highlighted) icon of each face button in a 52x52 box
// (tools/make_prompts.py FACE). g_phys[s] is the face button (FACE_SOUTH..) that presses Switch button s (A, B, X, Y)
// in the layout in use, -1 when none does; g_layout is that layout. The file in use was drawn for a layout's defaults
// (the even slots by position: the Nintendo layout; the odd ones by label: the Xbox one), the game's own icons for the
// layout in use; where a button is pressed by another than in those defaults, its boxes take the icon drawn for that
// one. The boxes are not 4-aligned, so the atlas is decoded, the boxes moved and the blocks under them encoded again.
enum { FACE_SOUTH, FACE_EAST, FACE_WEST, FACE_NORTH, FACE_BOX = 52 };
static const uint16_t face_box[4][2][2] = {   // [A, B, X, Y][white, blue] = {y, x}
    {{404, 782}, {404, 835}}, {{404, 888}, {404, 941}}, {{669, 689}, {669, 742}}, {{669, 795}, {669, 848}}};
static const int8_t face_default[PROMPTS_LAYOUTS][4] = {   // input_map.c defaults of A, B, X, Y
    {FACE_EAST, FACE_SOUTH, FACE_NORTH, FACE_WEST},    // Nintendo: by position
    {FACE_SOUTH, FACE_EAST, FACE_WEST, FACE_NORTH}};   // Xbox: by label
static int8_t g_phys[4] = {-1, -1, -1, -1};
static int g_layout;
static PlatMutex face_mu = PLAT_MUTEX_INIT;   // g_phys and g_layout: the input thread writes, uploads read

void prompts_face(int layout, const int8_t phys[4]) {
    if (layout < 0 || layout >= PROMPTS_LAYOUTS) return;
    plat_mutex_lock(&face_mu);
    if (memcmp(g_phys, phys, 4) || g_layout != layout) {
        memcpy(g_phys, phys, 4);
        g_layout = layout;
        __atomic_add_fetch(&g_gen, 1, __ATOMIC_RELEASE);
    }
    plat_mutex_unlock(&face_mu);
}

// The button each of A, B, X, Y takes its icons from (itself when unchanged); 1 when any differs.
static int face_sources(int active, int src[4]) {
    plat_mutex_lock(&face_mu);
    const int8_t *def = face_default[active >= 0 ? (active & 1) : g_layout];
    int moved = 0;
    for (int s = 0; s < 4; s++) {
        src[s] = s;
        for (int t = 0; g_phys[s] >= 0 && t < 4; t++)
            if (def[t] == g_phys[s]) src[s] = t;
        moved |= src[s] != s;
    }
    plat_mutex_unlock(&face_mu);
    return moved;
}

static void bc3_decode(const uint8_t *b, uint8_t out[16][4]) {
    uint8_t a[8] = {b[0], b[1]};
    if (a[0] > a[1])
        for (int i = 1; i < 7; i++) a[i + 1] = (uint8_t)(((7 - i) * a[0] + i * a[1]) / 7);
    else {
        for (int i = 1; i < 5; i++) a[i + 1] = (uint8_t)(((5 - i) * a[0] + i * a[1]) / 5);
        a[6] = 0, a[7] = 255;
    }
    uint64_t ai = 0;
    for (int i = 0; i < 6; i++) ai |= (uint64_t)b[2 + i] << (8 * i);
    uint16_t c0 = (uint16_t)(b[8] | b[9] << 8), c1 = (uint16_t)(b[10] | b[11] << 8);
    uint8_t c[4][3];
    for (int k = 0; k < 2; k++) {
        uint16_t v = k ? c1 : c0;
        c[k][0] = (uint8_t)((v >> 11) * 255 / 31), c[k][1] = (uint8_t)((v >> 5 & 63) * 255 / 63);
        c[k][2] = (uint8_t)((v & 31) * 255 / 31);
    }
    for (int j = 0; j < 3; j++)   // BC3's colour block always has four colours
        c[2][j] = (uint8_t)((2 * c[0][j] + c[1][j]) / 3), c[3][j] = (uint8_t)((c[0][j] + 2 * c[1][j]) / 3);
    uint32_t ci = (uint32_t)(b[12] | b[13] << 8 | b[14] << 16 | (uint32_t)b[15] << 24);
    for (int i = 0; i < 16; i++) {
        memcpy(out[i], c[ci >> (2 * i) & 3], 3);
        out[i][3] = a[ai >> (3 * i) & 7];
    }
}

static uint16_t to565(const float v[3]) {
    int r = (int)(v[0] * 31 / 255 + 0.5f), g = (int)(v[1] * 63 / 255 + 0.5f), b = (int)(v[2] * 31 / 255 + 0.5f);
    r = r < 0 ? 0 : r > 31 ? 31 : r, g = g < 0 ? 0 : g > 63 ? 63 : g, b = b < 0 ? 0 : b > 31 ? 31 : b;
    return (uint16_t)(r << 11 | g << 5 | b);
}

// A plain BC3 encoder for the icons: alpha between its extremes in eight steps, colour along the main axis of the
// visible texels' colours.
static void bc3_encode(const uint8_t px[16][4], uint8_t *b) {
    int lo = 255, hi = 0;
    for (int i = 0; i < 16; i++) lo = px[i][3] < lo ? px[i][3] : lo, hi = px[i][3] > hi ? px[i][3] : hi;
    b[0] = (uint8_t)hi, b[1] = (uint8_t)lo;
    uint64_t ai = 0;
    for (int i = 0; i < 16; i++) {
        int k = hi == lo ? 0 : ((hi - px[i][3]) * 7 + (hi - lo) / 2) / (hi - lo);   // 0 = hi .. 7 = lo
        int idx = k == 0 ? 0 : k == 7 ? 1 : k + 1;
        ai |= (uint64_t)idx << (3 * i);
    }
    for (int i = 0; i < 6; i++) b[2 + i] = (uint8_t)(ai >> (8 * i));
    float mean[3] = {0}, n = 0;
    for (int i = 0; i < 16; i++)
        if (px[i][3]) n++, mean[0] += px[i][0], mean[1] += px[i][1], mean[2] += px[i][2];
    int all = n == 0;   // nothing visible: fit every texel
    if (all)
        for (int i = 0; i < 16; i++) n++, mean[0] += px[i][0], mean[1] += px[i][1], mean[2] += px[i][2];
    for (int j = 0; j < 3; j++) mean[j] /= n;
    float cov[6] = {0};
    for (int i = 0; i < 16; i++) {
        if (!all && !px[i][3]) continue;
        float d[3] = {px[i][0] - mean[0], px[i][1] - mean[1], px[i][2] - mean[2]};
        cov[0] += d[0] * d[0], cov[1] += d[0] * d[1], cov[2] += d[0] * d[2];
        cov[3] += d[1] * d[1], cov[4] += d[1] * d[2], cov[5] += d[2] * d[2];
    }
    float ax[3] = {1, 1, 1};
    for (int it = 0; it < 8; it++) {   // power iteration: the main axis
        float v[3] = {cov[0] * ax[0] + cov[1] * ax[1] + cov[2] * ax[2], cov[1] * ax[0] + cov[3] * ax[1] + cov[4] * ax[2],
                      cov[2] * ax[0] + cov[4] * ax[1] + cov[5] * ax[2]};
        float m = fmaxf(fmaxf(fabsf(v[0]), fabsf(v[1])), fabsf(v[2]));
        if (m < 1e-6f) break;
        ax[0] = v[0] / m, ax[1] = v[1] / m, ax[2] = v[2] / m;
    }
    float tmin = 1e30f, tmax = -1e30f;
    for (int i = 0; i < 16; i++) {
        if (!all && !px[i][3]) continue;
        float t = (px[i][0] - mean[0]) * ax[0] + (px[i][1] - mean[1]) * ax[1] + (px[i][2] - mean[2]) * ax[2];
        tmin = fminf(tmin, t), tmax = fmaxf(tmax, t);
    }
    float l2 = ax[0] * ax[0] + ax[1] * ax[1] + ax[2] * ax[2];
    float e0[3], e1[3];
    for (int j = 0; j < 3; j++) e0[j] = mean[j] + ax[j] * tmax / l2, e1[j] = mean[j] + ax[j] * tmin / l2;
    uint16_t c0 = to565(e0), c1 = to565(e1);
    if (c0 < c1) { uint16_t t = c0; c0 = c1; c1 = t; }
    b[8] = (uint8_t)c0, b[9] = (uint8_t)(c0 >> 8), b[10] = (uint8_t)c1, b[11] = (uint8_t)(c1 >> 8);
    uint8_t pal[4][3], dec[16][4];
    for (int k = 0; k < 4; k++) {   // the four colours as a decoder makes them
        uint32_t all_k = (uint32_t)k * 0x55555555u;
        memcpy(b + 12, &all_k, 4);
        bc3_decode(b, dec);
        memcpy(pal[k], dec[0], 3);
    }
    uint32_t ci = 0;
    for (int i = 0; i < 16; i++) {
        int best = 0, bd = 1 << 30;
        for (int k = 0; k < 4; k++) {
            int d = 0;
            for (int j = 0; j < 3; j++) d += (px[i][j] - pal[k][j]) * (px[i][j] - pal[k][j]);
            if (d < bd) bd = d, best = k;
        }
        ci |= (uint32_t)best << (2 * i);
    }
    memcpy(b + 12, &ci, 4);
}

// The button atlas's blocks (w x h, rows of blocks) with the boxes of moved buttons replaced (face_sources).
static void face_move(uint8_t *blocks, uint32_t w, uint32_t h, const int src[4]) {
    uint32_t bw = w / 4;
    uint8_t *img = malloc((size_t)w * h * 4);
    uint8_t(*box)[2][FACE_BOX][FACE_BOX][4] = malloc(sizeof(uint8_t[4][2][FACE_BOX][FACE_BOX][4]));
    if (!img || !box) {
        free(img), free(box);
        return;
    }
    for (uint32_t by = 0; by < h / 4; by++)
        for (uint32_t bx = 0; bx < bw; bx++) {
            uint8_t px[16][4];
            bc3_decode(blocks + ((size_t)by * bw + bx) * 16, px);
            for (int i = 0; i < 16; i++) memcpy(img + (((size_t)by * 4 + i / 4) * w + bx * 4 + i % 4) * 4, px[i], 4);
        }
    for (int s = 0; s < 4; s++)   // the icons before the move
        for (int k = 0; k < 2; k++)
            for (int y = 0; y < FACE_BOX; y++)
                memcpy(box[s][k][y], img + (((size_t)face_box[s][k][0] + y) * w + face_box[s][k][1]) * 4, FACE_BOX * 4);
    for (int s = 0; s < 4; s++)
        for (int k = 0; src[s] != s && k < 2; k++) {
            uint32_t y0 = face_box[s][k][0], x0 = face_box[s][k][1];
            for (int y = 0; y < FACE_BOX; y++)
                memcpy(img + (((size_t)y0 + y) * w + x0) * 4, box[src[s]][k][y], FACE_BOX * 4);
        }
    for (int s = 0; s < 4; s++)   // after every move: a block under two boxes holds both
        for (int k = 0; src[s] != s && k < 2; k++) {
            uint32_t y0 = face_box[s][k][0], x0 = face_box[s][k][1];
            for (uint32_t by = y0 / 4; by <= (y0 + FACE_BOX - 1) / 4; by++)
                for (uint32_t bx = x0 / 4; bx <= (x0 + FACE_BOX - 1) / 4; bx++) {
                    uint8_t px[16][4];
                    for (int i = 0; i < 16; i++)
                        memcpy(px[i], img + (((size_t)by * 4 + i / 4) * w + bx * 4 + i % 4) * 4, 4);
                    bc3_encode(px, blocks + ((size_t)by * bw + bx) * 16);
                }
        }
    free(img), free(box);
}

int prompts_swap(uint32_t w, uint32_t h, int bc3, uint8_t *blocks, size_t size) {
    if (!bc3) return 0;
    uint64_t hash = 0;
    int active = __atomic_load_n(&g_active, __ATOMIC_ACQUIRE);
    for (int f = 0; f < PROMPTS_FILES; f++)   // every file holds the game's fingerprints: one is enough to know it
        for (int i = 0; i < g_count[f]; i++) {
            const PromptTex *g = &g_files[f][i];
            if (g->w != w || g->h != h || g->size != size) continue;
            if (!hash) hash = prompts_hash(blocks, size);
            if (hash != g->hash) continue;
            for (int k = 0; active >= 0 && k < g_count[active]; k++) {
                const PromptTex *a = &g_files[active][k];
                if (a->hash == hash && a->size == size) memcpy(blocks, a->data, size);
            }
            int src[4];
            if (w == 1024 && h == 1024) input_face_refresh();   // the bindings now: the first upload comes before input starts
            if (w == 1024 && h == 1024 && face_sources(active, src)) {
                face_move(blocks, w, h, src);
                rt_log("prompts: remapped buttons: A, B, X, Y show the icons of %c, %c, %c, %c", "ABXY"[src[0]],
                       "ABXY"[src[1]], "ABXY"[src[2]], "ABXY"[src[3]]);
            }
            rt_log("prompts: a texture of the game's buttons (%ux%u) uploaded with the %s buttons", w, h,
                   active >= 0 ? "new" : "game's own");
            return 1;
        }
    return 0;
}
