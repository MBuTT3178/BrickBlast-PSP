/*
 * BLOCK BLAST — PSP Edition
 * Language: C99 / PSPSDK / MIPS R4000 (Allegrex)
 * Target: PlayStation Portable & PPSSPP
 * Audio: Official Block Blast BGM loop + official sound effects
 * Visuals: Authentic Block Blast glossy gem blocks, socket board, and arcade palette
 */

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <psptypes.h>
#include <pspiofilemgr.h>
#include <psppower.h>
#include <pspaudiolib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "real_sounds.h"

PSP_MODULE_INFO("BlockBlast", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(4096);

/* ─── EXIT CALLBACK ──────────────────────────────────────────────────── */

static int running = 1;

static int exit_cb(int arg1, int arg2, void *common) {
    (void)arg1; (void)arg2; (void)common;
    running = 0;
    sceKernelExitGame();
    return 0;
}

static int cb_thread(SceSize args, void *argp) {
    (void)args; (void)argp;
    int cbid = sceKernelCreateCallback("exit_cb", exit_cb, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

static void setup_callbacks(void) {
    int thid = sceKernelCreateThread("cb_thread", cb_thread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0) sceKernelStartThread(thid, 0, 0);
}

/* ─── AUDIO ENGINE (BGM STREAMER + MULTI-VOICE SFX MIXER) ────────────── */

#define MAX_AUDIO_VOICES 4

typedef struct {
    const s16 *data;
    int total_samples;
    int current_sample;
    int active;
} AudioVoice;

static AudioVoice audio_voices[MAX_AUDIO_VOICES];
static int bgm_pos = 0;
static int bgm_enabled = 1;

static void play_sound(const s16 *data, int sample_count) {
    if (!data || sample_count <= 0) return;

    int slot = -1;
    for (int i = 0; i < MAX_AUDIO_VOICES; i++) {
        if (!audio_voices[i].active) {
            slot = i;
            break;
        }
    }
    if (slot == -1) slot = 0;

    audio_voices[slot].data = data;
    audio_voices[slot].total_samples = sample_count;
    audio_voices[slot].current_sample = 0;
    audio_voices[slot].active = 1;
}

static void audio_callback(void *buf, unsigned int reqn, void *pdata) {
    short *out = (short *)buf;

    /* 1. Mix Background Music (22050 Hz source -> 44100 Hz output with linear interpolation) */
    if (bgm_enabled) {
        for (unsigned int i = 0; i < reqn; i++) {
            int idx = bgm_pos >> 1;
            int sub = bgm_pos & 1;
            int s0 = snd_bgm[idx];
            int s1 = (idx + 1 < SND_BGM_SAMPLES) ? snd_bgm[idx + 1] : snd_bgm[0];
            int raw_samp = sub ? ((s0 + s1) >> 1) : s0;
            int bgm_samp = (raw_samp * 20) / 100; /* 20% soft ambient volume */

            bgm_pos++;
            if ((bgm_pos >> 1) >= SND_BGM_SAMPLES) bgm_pos = 0;

            out[i * 2]     = (short)bgm_samp;
            out[i * 2 + 1] = (short)bgm_samp;
        }
    } else {
        memset(out, 0, reqn * 2 * sizeof(short));
    }

    /* 2. Mix active sound effects on top (22050 Hz -> 44100 Hz with saturation clipping) */
    for (int v = 0; v < MAX_AUDIO_VOICES; v++) {
        if (!audio_voices[v].active) continue;

        const s16 *src = audio_voices[v].data;
        int total_output_frames = audio_voices[v].total_samples << 1;
        int rem = total_output_frames - audio_voices[v].current_sample;
        int count = (int)reqn;
        if (count > rem) count = rem;

        for (int i = 0; i < count; i++) {
            int curr = audio_voices[v].current_sample + i;
            int idx = curr >> 1;
            int sub = curr & 1;
            int s0 = src[idx];
            int s1 = (idx + 1 < audio_voices[v].total_samples) ? src[idx + 1] : s0;
            int samp = sub ? ((s0 + s1) >> 1) : s0;

            int l = out[i * 2] + samp;
            int r = out[i * 2 + 1] + samp;
            out[i * 2]     = (short)(l > 32767 ? 32767 : (l < -32768 ? -32768 : l));
            out[i * 2 + 1] = (short)(r > 32767 ? 32767 : (r < -32768 ? -32768 : r));
        }

        audio_voices[v].current_sample += count;
        if (audio_voices[v].current_sample >= total_output_frames) {
            audio_voices[v].active = 0;
        }
    }
}

/* ─── HARDWARE DISPLAY & DOUBLE BUFFERING ────────────────────────────── */

#define SCR_W       480
#define SCR_H       272
#define BUF_W       512
#define FRAME_BYTES (BUF_W * SCR_H * 4)

#define VRAM_BASE0  0x44000000
#define VRAM_BASE1  0x44088000

static u32 *fb_draw = (u32 *)VRAM_BASE1;
static void *fb_disp = (void *)VRAM_BASE0;

static void init_display(void) {
    sceDisplaySetMode(0, SCR_W, SCR_H);
    fb_draw = (u32 *)VRAM_BASE1;
    fb_disp = (void *)VRAM_BASE0;
    sceDisplaySetFrameBuf(fb_disp, BUF_W, PSP_DISPLAY_PIXEL_FORMAT_8888, PSP_DISPLAY_SETBUF_IMMEDIATE);
}

static void flip_buffers(void) {
    sceDisplaySetFrameBuf((void *)fb_draw, BUF_W, PSP_DISPLAY_PIXEL_FORMAT_8888, PSP_DISPLAY_SETBUF_NEXTFRAME);
    sceDisplayWaitVblankStart();
    fb_disp = (void *)fb_draw;
    fb_draw = (fb_draw == (u32 *)VRAM_BASE0) ? (u32 *)VRAM_BASE1 : (u32 *)VRAM_BASE0;
}

/* ─── COLOR PALETTE (OFFICIAL BLOCK BLAST PALETTE) ───────────────────── */

#define RGBA(r, g, b, a) ((u32)(r) | ((u32)(g) << 8) | ((u32)(b) << 16) | ((u32)(a) << 24))
#define RGB(r, g, b)     RGBA(r, g, b, 255)

static inline u8 clamp8(int v) { return (u8)(v > 255 ? 255 : (v < 0 ? 0 : v)); }

static inline u32 col_darken(u32 c, int pct) {
    int r = (c & 0xFF) * pct / 100;
    int g = ((c >> 8) & 0xFF) * pct / 100;
    int b = ((c >> 16) & 0xFF) * pct / 100;
    return RGBA(clamp8(r), clamp8(g), clamp8(b), 255);
}

static inline u32 col_lighten(u32 c, int pct) {
    int r = (c & 0xFF) * pct / 100;
    int g = ((c >> 8) & 0xFF) * pct / 100;
    int b = ((c >> 16) & 0xFF) * pct / 100;
    return RGBA(clamp8(r), clamp8(g), clamp8(b), 255);
}

/* Block Blast Theme Colors */
#define COL_BG_TOP      RGB(12, 20, 52)   /* Deep Sapphire Navy */
#define COL_BG_BOT      RGB(8, 14, 36)    /* Dark Midnight Abyss */
#define COL_BOARD_CARD  RGB(16, 26, 68)   /* Board Container */
#define COL_BOARD_EDGE  RGB(28, 42, 98)   /* Board Rim */
#define COL_SOCKET_BG   RGB(24, 36, 88)   /* Empty socket base */
#define COL_SOCKET_SH   RGB(14, 22, 56)   /* Socket inner shadow */
#define COL_SOCKET_HI   RGB(36, 52, 115)  /* Socket inner highlight */
#define COL_TRAY_BG     RGB(20, 30, 76)   /* Tray container */
#define COL_TEXT_WHITE  RGB(255, 255, 255)
#define COL_TEXT_DIM    RGB(140, 160, 205)
#define COL_TEXT_GOLD   RGB(255, 215, 20)
#define COL_CYAN_GLOW   RGB(0, 225, 255)
#define COL_ALERT_RED   RGB(255, 60, 70)
#define COL_FLASH       RGB(255, 255, 255)

/* Authentic Block Blast 8 Gem Block Colors */
#define NUM_COLORS 8
static const u32 piece_colors[NUM_COLORS] = {
    RGB(0, 215, 255),   /* 0: Bright Cyan       */
    RGB(255, 140, 20),  /* 1: Radiant Orange    */
    RGB(255, 60, 140),  /* 2: Neon Pink/Rose    */
    RGB(255, 225, 30),  /* 3: Lemon Yellow      */
    RGB(55, 225, 75),   /* 4: Lime Green        */
    RGB(180, 80, 255),  /* 5: Electric Purple   */
    RGB(30, 115, 255),  /* 6: Royal Sapphire    */
    RGB(255, 50, 60),   /* 7: Crimson Red       */
};

/* ─── DRAWING PRIMITIVES ─────────────────────────────────────────────── */

static inline void put_pixel(int x, int y, u32 c) {
    if ((unsigned)x < SCR_W && (unsigned)y < SCR_H)
        fb_draw[y * BUF_W + x] = c;
}

static void fill_rect(int x, int y, int w, int h, u32 c) {
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w > SCR_W ? SCR_W : x + w;
    int y1 = y + h > SCR_H ? SCR_H : y + h;
    for (int j = y0; j < y1; j++) {
        u32 *row = fb_draw + j * BUF_W;
        for (int i = x0; i < x1; i++) {
            row[i] = c;
        }
    }
}

static void draw_border(int x, int y, int w, int h, int thickness, u32 c) {
    fill_rect(x, y, w, thickness, c);
    fill_rect(x, y + h - thickness, w, thickness, c);
    fill_rect(x, y, thickness, h, c);
    fill_rect(x + w - thickness, y, thickness, h, c);
}

/* Draw official-style rounded recessed socket */
static void draw_socket(int x, int y, int sz) {
    /* Socket fill */
    fill_rect(x + 1, y + 1, sz - 2, sz - 2, COL_SOCKET_BG);
    /* Inset shadow (top & left) */
    fill_rect(x + 1, y + 1, sz - 2, 2, COL_SOCKET_SH);
    fill_rect(x + 1, y + 1, 2, sz - 2, COL_SOCKET_SH);
    /* Soft inset highlight (bottom & right) */
    fill_rect(x + 1, y + sz - 3, sz - 2, 2, COL_SOCKET_HI);
    fill_rect(x + sz - 3, y + 1, 2, sz - 2, COL_SOCKET_HI);
}

/* Draw official glossy 3D gem block with glass specular pill */
static void draw_gem_block(int x, int y, int sz, u32 color) {
    /* Main body fill */
    fill_rect(x + 1, y + 1, sz - 2, sz - 2, color);

    /* Outer dark border */
    draw_border(x, y, sz, sz, 1, col_darken(color, 60));

    /* Top and Left beveled rim highlight */
    u32 hi = col_lighten(color, 140);
    fill_rect(x + 1, y + 1, sz - 2, 2, hi);
    fill_rect(x + 1, y + 1, 2, sz - 2, hi);

    /* Bottom and Right shadow bevel */
    u32 sh = col_darken(color, 50);
    fill_rect(x + 1, y + sz - 3, sz - 2, 2, sh);
    fill_rect(x + sz - 3, y + 1, 2, sz - 2, sh);

    /* Specular glass reflection pill in upper-left */
    if (sz >= 16) {
        u32 flare = col_lighten(color, 190);
        int pill_w = sz / 2 - 2;
        int pill_h = sz / 4;
        fill_rect(x + 4, y + 4, pill_w, pill_h, flare);
        fill_rect(x + 5, y + 5, pill_w - 2, pill_h - 2, RGB(255, 255, 255));
    }
}

/* Draw ghost placement block */
static void draw_ghost_gem(int x, int y, int sz, u32 color, int valid) {
    if (valid) {
        u32 body = col_darken(color, 45);
        fill_rect(x + 1, y + 1, sz - 2, sz - 2, body);
        draw_border(x, y, sz, sz, 2, col_lighten(color, 160));
        fill_rect(x + sz/2 - 2, y + sz/2 - 2, 4, 4, RGB(255, 255, 255));
    } else {
        fill_rect(x + 1, y + 1, sz - 2, sz - 2, RGB(120, 25, 35));
        draw_border(x, y, sz, sz, 2, COL_ALERT_RED);
        for (int k = 4; k < sz - 4; k++) {
            put_pixel(x + k, y + k, COL_ALERT_RED);
            put_pixel(x + sz - 1 - k, y + k, COL_ALERT_RED);
        }
    }
}

/* ─── 8×8 BITMAP FONT ─────────────────────────────────────────────────── */

static const u8 font8x8[59][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 32 SPC */
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, /* 33 !   */
    {0x6C,0x6C,0x00,0x00,0x00,0x00,0x00,0x00}, /* 34 "   */
    {0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00}, /* 35 #   */
    {0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00}, /* 36 $   */
    {0x00,0xC6,0xCC,0x18,0x30,0x66,0xC6,0x00}, /* 37 %   */
    {0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00}, /* 38 &   */
    {0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00}, /* 39 '   */
    {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00}, /* 40 (   */
    {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00}, /* 41 )   */
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, /* 42 *   */
    {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00}, /* 43 +   */
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30}, /* 44 ,   */
    {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00}, /* 45 -   */
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, /* 46 .   */
    {0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00}, /* 47 /   */
    {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00}, /* 48 0   */
    {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00}, /* 49 1   */
    {0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00}, /* 50 2   */
    {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00}, /* 51 3   */
    {0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C,0x00}, /* 52 4   */
    {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00}, /* 53 5   */
    {0x1C,0x30,0x60,0x7C,0x66,0x66,0x3C,0x00}, /* 54 6   */
    {0x7E,0x06,0x0C,0x18,0x18,0x18,0x18,0x00}, /* 55 7   */
    {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00}, /* 56 8   */
    {0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38,0x00}, /* 57 9   */
    {0x00,0x00,0x18,0x00,0x00,0x18,0x00,0x00}, /* 58 :   */
    {0x00,0x00,0x18,0x00,0x00,0x18,0x18,0x30}, /* 59 ;   */
    {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00}, /* 60 <   */
    {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00}, /* 61 =   */
    {0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00}, /* 62 >   */
    {0x3C,0x66,0x06,0x0C,0x18,0x00,0x18,0x00}, /* 63 ?   */
    {0x3C,0x66,0x6E,0x6E,0x60,0x62,0x3C,0x00}, /* 64 @   */
    {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00}, /* 65 A   */
    {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00}, /* 66 B   */
    {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00}, /* 67 C   */
    {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00}, /* 68 D   */
    {0x7E,0x60,0x60,0x78,0x60,0x60,0x7E,0x00}, /* 69 E   */
    {0x7E,0x60,0x60,0x78,0x60,0x60,0x60,0x00}, /* 70 F   */
    {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3E,0x00}, /* 71 G   */
    {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00}, /* 72 H   */
    {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, /* 73 I   */
    {0x1E,0x0C,0x0C,0x0C,0x0C,0x6C,0x38,0x00}, /* 74 J   */
    {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00}, /* 75 K   */
    {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00}, /* 76 L   */
    {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00}, /* 77 M   */
    {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00}, /* 78 N   */
    {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, /* 79 O   */
    {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00}, /* 80 P   */
    {0x3C,0x66,0x66,0x66,0x6A,0x6C,0x36,0x00}, /* 81 Q   */
    {0x7C,0x66,0x66,0x7C,0x78,0x6C,0x66,0x00}, /* 82 R   */
    {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00}, /* 83 S   */
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, /* 84 T   */
    {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, /* 85 U   */
    {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00}, /* 86 V   */
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00}, /* 87 W   */
    {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00}, /* 88 X   */
    {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00}, /* 89 Y   */
    {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00}, /* 90 Z   */
};

static void draw_char(int x, int y, char ch, u32 color, int scale) {
    if (ch >= 'a' && ch <= 'z') ch -= 32;
    if (ch < 32 || ch > 90) return;
    const u8 *g = font8x8[ch - 32];
    for (int row = 0; row < 8; row++) {
        u8 bits = g[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) {
                if (scale == 1) {
                    put_pixel(x + col, y + row, color);
                } else {
                    fill_rect(x + col * scale, y + row * scale, scale, scale, color);
                }
            }
        }
    }
}

static void draw_str(int x, int y, const char *s, u32 color, int scale) {
    while (*s) {
        draw_char(x, y, *s, color, scale);
        x += 8 * scale;
        s++;
    }
}

static void draw_int(int x, int y, int val, u32 color, int scale) {
    char buf[16];
    int i = 0;
    if (val <= 0) { buf[i++] = '0'; }
    else {
        int tmp = val;
        while (tmp > 0) { buf[i++] = '0' + (tmp % 10); tmp /= 10; }
    }
    buf[i] = 0;
    for (int j = 0; j < i / 2; j++) {
        char t = buf[j]; buf[j] = buf[i - 1 - j]; buf[i - 1 - j] = t;
    }
    draw_str(x, y, buf, color, scale);
}

/* ─── SHAPES (34 AUTHENTIC BLOCK BLAST SHAPES) ───────────────────────── */

#define MAX_CELLS   9
#define NUM_SHAPES  34

typedef struct {
    int c[MAX_CELLS][2];
    int n;
    int w, h;
} Shape;

static Shape shapes[NUM_SHAPES];

static void init_shapes(void) {
    static const struct { int cells[MAX_CELLS][2]; int n; } raw[NUM_SHAPES] = {
        /* 0: 1x1 dot */          {{{0,0}}, 1},
        /* 1: 2x1 horiz */        {{{0,0},{1,0}}, 2},
        /* 2: 1x2 vert */         {{{0,0},{0,1}}, 2},
        /* 3: 3x1 horiz */        {{{0,0},{1,0},{2,0}}, 3},
        /* 4: 1x3 vert */         {{{0,0},{0,1},{0,2}}, 3},
        /* 5: 4x1 horiz */        {{{0,0},{1,0},{2,0},{3,0}}, 4},
        /* 6: 1x4 vert */         {{{0,0},{0,1},{0,2},{0,3}}, 4},
        /* 7: 5x1 horiz */        {{{0,0},{1,0},{2,0},{3,0},{4,0}}, 5},
        /* 8: 1x5 vert */         {{{0,0},{0,1},{0,2},{0,3},{0,4}}, 5},
        /* 9: 2x2 square */       {{{0,0},{1,0},{0,1},{1,1}}, 4},
        /* 10: 3x3 square */      {{{0,0},{1,0},{2,0},{0,1},{1,1},{2,1},{0,2},{1,2},{2,2}}, 9},
        /* 11: 2x3 rect */        {{{0,0},{1,0},{0,1},{1,1},{0,2},{1,2}}, 6},
        /* 12: 3x2 rect */        {{{0,0},{1,0},{2,0},{0,1},{1,1},{2,1}}, 6},
        /* 13: small-corner TL */ {{{0,0},{1,0},{0,1}}, 3},
        /* 14: small-corner TR */ {{{0,0},{1,0},{1,1}}, 3},
        /* 15: small-corner BL */ {{{0,0},{0,1},{1,1}}, 3},
        /* 16: small-corner BR */ {{{1,0},{0,1},{1,1}}, 3},
        /* 17: big-corner TL */   {{{0,0},{1,0},{2,0},{0,1},{0,2}}, 5},
        /* 18: big-corner TR */   {{{0,0},{1,0},{2,0},{2,1},{2,2}}, 5},
        /* 19: big-corner BL */   {{{0,0},{0,1},{0,2},{1,2},{2,2}}, 5},
        /* 20: big-corner BR */   {{{2,0},{2,1},{0,2},{1,2},{2,2}}, 5},
        /* 21: T-down */          {{{0,0},{1,0},{2,0},{1,1}}, 4},
        /* 22: T-up */            {{{1,0},{0,1},{1,1},{2,1}}, 4},
        /* 23: T-right */         {{{0,0},{0,1},{0,2},{1,1}}, 4},
        /* 24: T-left */          {{{1,0},{1,1},{1,2},{0,1}}, 4},
        /* 25: S-horiz */         {{{1,0},{2,0},{0,1},{1,1}}, 4},
        /* 26: S-vert */          {{{0,0},{0,1},{1,1},{1,2}}, 4},
        /* 27: Z-horiz */         {{{0,0},{1,0},{1,1},{2,1}}, 4},
        /* 28: Z-vert */          {{{1,0},{0,1},{1,1},{0,2}}, 4},
        /* 29: L-1 */             {{{0,0},{0,1},{0,2},{1,2}}, 4},
        /* 30: L-2 */             {{{0,0},{1,0},{2,0},{0,1}}, 4},
        /* 31: J-1 */             {{{1,0},{1,1},{0,2},{1,2}}, 4},
        /* 32: J-2 */             {{{0,0},{0,1},{1,1},{2,1}}, 4},
        /* 33: Plus / Cross */    {{{1,0},{0,1},{1,1},{2,1},{1,2}}, 5},
    };

    for (int i = 0; i < NUM_SHAPES; i++) {
        shapes[i].n = raw[i].n;
        int max_c = 0, max_r = 0;
        for (int j = 0; j < raw[i].n; j++) {
            shapes[i].c[j][0] = raw[i].cells[j][0];
            shapes[i].c[j][1] = raw[i].cells[j][1];
            if (shapes[i].c[j][0] > max_c) max_c = shapes[i].c[j][0];
            if (shapes[i].c[j][1] > max_r) max_r = shapes[i].c[j][1];
        }
        shapes[i].w = max_c + 1;
        shapes[i].h = max_r + 1;
    }
}

/* ─── GRID & PANEL METRICS ───────────────────────────────────────────── */

#define HEADER_H    32

#define GRID_N      8
#define CELL_SZ     25
#define CELL_GAP    2
#define GRID_INNER  (GRID_N * CELL_SZ + (GRID_N - 1) * CELL_GAP) /* 214 */
#define BOARD_BORDER 4
#define BOARD_TOTAL (GRID_INNER + BOARD_BORDER * 2)               /* 222 */
#define BOARD_X     16
#define BOARD_Y     40

#define PANEL_X     254
#define PANEL_Y     40
#define PANEL_W     212
#define PANEL_H     222

/* ─── GAME STATE ─────────────────────────────────────────────────────── */

#define REVIVE_BASE_COST 100
#define REVIVE_STEP_COST 50
#define BOMB_COST        300

typedef struct {
    u8  grid[GRID_N][GRID_N];
    int score;
    int high_score;
    int coins;          /* Persistent player coins */
    int combo;          /* Lines cleared in current placement (multi-line) */
    int streak;         /* Consecutive turns with line clears */
    int streak_moves_left; /* 0..3 moves remaining to keep streak alive */
    int streak_lost_timer; /* Timer to flash streak lost alert */
    int cursor_r, cursor_c;
    int sel;

    struct {
        int shape;
        int color;
        int used;
    } pieces[3];

    struct {
        int shape;
        int color;
        int active;
    } hold;
    int hold_locked;

    int hammer_mode;
    int hammer_charges;

    int game_over;
    int revive_modal;   /* 1 when Out of Moves & asking player to revive */
    int revive_count;   /* Number of revives used in current run */
    int paused;         /* 1 when paused */
    int flash_timer;
    u8  flash[GRID_N][GRID_N];

    int popup_val;
    int popup_timer;
    int coin_popup_val;
    int coin_popup_timer;
    int combo_display;
    int combo_timer;
    int streak_display;

    int anim_tick;
} Game;

static int get_revive_cost(const Game *g) {
    return REVIVE_BASE_COST + g->revive_count * REVIVE_STEP_COST;
}

static int count_digits(int v) {
    if (v < 0) v = -v;
    if (v == 0) return 1;
    int d = 0;
    while (v > 0) { d++; v /= 10; }
    return d;
}

/* ─── RNG ────────────────────────────────────────────────────────────── */

static unsigned int rng_state = 1;
static void rng_seed(unsigned int s) { rng_state = s ? s : 1; }
static unsigned int rng_next(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}
static int rng_range(int n) { return (int)(rng_next() % (unsigned)n); }

/* ─── PERSISTENCE ────────────────────────────────────────────────────── */

#define SAVE_DIR         "ms0:/PSP/SAVEDATA/BRICKBLAST"
#define SAVE_FILE        SAVE_DIR "/score.dat"
#define LEGACY_SAVE_FILE "ms0:/PSP/SAVEDATA/BLOCKBLAST/score.dat"

typedef struct {
    int high_score;
    int coins;
} SaveData;

static SaveData save_data = {0, 200};

static void load_save_data(void) {
    SceUID fd = sceIoOpen(SAVE_FILE, PSP_O_RDONLY, 0777);
    if (fd < 0) {
        /* Fallback / migration from legacy BLOCKBLAST save */
        fd = sceIoOpen(LEGACY_SAVE_FILE, PSP_O_RDONLY, 0777);
    }
    if (fd >= 0) {
        sceIoRead(fd, &save_data, sizeof(save_data));
        sceIoClose(fd);
        if (save_data.coins < 0) save_data.coins = 200;
    } else {
        save_data.high_score = 0;
        save_data.coins = 200; /* 200 free starter coins */
    }
}

static void save_game_data(void) {
    sceIoMkdir("ms0:/PSP/SAVEDATA/BRICKBLAST", 0777);
    SceUID fd = sceIoOpen(SAVE_FILE, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (fd >= 0) {
        sceIoWrite(fd, &save_data, sizeof(save_data));
        sceIoClose(fd);
    }
}

/* ─── GAME LOGIC & SOLVER ALGORITHM ──────────────────────────────────── */

static int can_place(const Game *g, const Shape *s, int cr, int cc) {
    for (int i = 0; i < s->n; i++) {
        int r = cr + s->c[i][1];
        int c = cc + s->c[i][0];
        if (r < 0 || r >= GRID_N || c < 0 || c >= GRID_N) return 0;
        if (g->grid[r][c] != 0) return 0;
    }
    return 1;
}

static int shape_can_fit(const Game *g, int shape_idx) {
    if (shape_idx < 0 || shape_idx >= NUM_SHAPES) return 0;
    const Shape *s = &shapes[shape_idx];
    for (int r = 0; r <= GRID_N - s->h; r++) {
        for (int c = 0; c <= GRID_N - s->w; c++) {
            if (can_place(g, s, r, c)) return 1;
        }
    }
    return 0;
}

/* Check if placing this shape ANYWHERE on the current board completes a line */
static int shape_can_clear(const Game *g, int shape_idx) {
    if (shape_idx < 0 || shape_idx >= NUM_SHAPES) return 0;
    const Shape *s = &shapes[shape_idx];
    for (int r = 0; r <= GRID_N - s->h; r++) {
        for (int c = 0; c <= GRID_N - s->w; c++) {
            if (!can_place(g, s, r, c)) continue;
            for (int i = 0; i < s->n; i++) {
                int pr = r + s->c[i][1];
                int full_r = 1;
                for (int cc = 0; cc < GRID_N; cc++) {
                    if (g->grid[pr][cc] != 0) continue;
                    int covered = 0;
                    for (int j = 0; j < s->n; j++) {
                        if (r + s->c[j][1] == pr && c + s->c[j][0] == cc) {
                            covered = 1;
                            break;
                        }
                    }
                    if (!covered) { full_r = 0; break; }
                }
                if (full_r) return 1;

                int pc = c + s->c[i][0];
                int full_c = 1;
                for (int rr = 0; rr < GRID_N; rr++) {
                    if (g->grid[rr][pc] != 0) continue;
                    int covered = 0;
                    for (int j = 0; j < s->n; j++) {
                        if (r + s->c[j][1] == rr && c + s->c[j][0] == pc) {
                            covered = 1;
                            break;
                        }
                    }
                    if (!covered) { full_c = 0; break; }
                }
                if (full_c) return 1;
            }
        }
    }
    return 0;
}

static void get_clears_preview(const Game *g, int *out_rows, int *out_cols, int *num_lines) {
    for (int i = 0; i < GRID_N; i++) { out_rows[i] = 0; out_cols[i] = 0; }
    *num_lines = 0;
    if (g->pieces[g->sel].used || g->game_over || g->revive_modal || g->hammer_mode) return;

    const Shape *s = &shapes[g->pieces[g->sel].shape];
    if (!can_place(g, s, g->cursor_r, g->cursor_c)) return;

    for (int r = 0; r < GRID_N; r++) {
        int full = 1;
        for (int c = 0; c < GRID_N; c++) {
            if (g->grid[r][c] != 0) continue;
            int covered = 0;
            for (int j = 0; j < s->n; j++) {
                if (g->cursor_r + s->c[j][1] == r && g->cursor_c + s->c[j][0] == c) {
                    covered = 1;
                    break;
                }
            }
            if (!covered) { full = 0; break; }
        }
        if (full) { out_rows[r] = 1; (*num_lines)++; }
    }

    for (int c = 0; c < GRID_N; c++) {
        int full = 1;
        for (int r = 0; r < GRID_N; r++) {
            if (g->grid[r][c] != 0) continue;
            int covered = 0;
            for (int j = 0; j < s->n; j++) {
                if (g->cursor_r + s->c[j][1] == r && g->cursor_c + s->c[j][0] == c) {
                    covered = 1;
                    break;
                }
            }
            if (!covered) { full = 0; break; }
        }
        if (full) { out_cols[c] = 1; (*num_lines)++; }
    }
}

/* Canonical Block Blast Color Assignment */
static int get_shape_color(int shape_idx) {
    switch (shape_idx) {
        case 0: return 3; /* 1x1 Dot: Lemon Yellow */
        case 1: case 2: return 0; /* 2x1, 1x2: Azure Cyan */
        case 3: case 4: return 1; /* 3x1, 1x3: Radiant Orange */
        case 5: case 6: return 4; /* 4x1, 1x4: Lime Green */
        case 7: case 8: return 1; /* 5x1, 1x5: Radiant Orange */
        case 9: return 3; /* 2x2 Square: Lemon Yellow */
        case 10: return 6; /* 3x3 Square: Royal Sapphire */
        case 11: case 12: return 5; /* 2x3, 3x2: Electric Purple */
        case 13: case 14: case 15: case 16: return 2; /* Small Corners: Neon Rose */
        case 17: case 18: case 19: case 20: return 7; /* Big Corners: Crimson Red */
        case 21: case 22: case 23: case 24: return 5; /* T-Shapes: Electric Purple */
        case 25: case 26: case 27: case 28: return 4; /* S/Z Shapes: Lime Green */
        case 29: case 30: case 31: case 32: return 0; /* L/J Shapes: Azure Cyan */
        case 33: return 7; /* Plus: Crimson Red */
        default: return shape_idx % NUM_COLORS;
    }
}

/* ─── INTELLIGENT BLOCK BLAST SPAWNING ALGORITHM ─────────────────────── */
static void generate_pieces(Game *g) {
    int occupied = 0;
    for (int r = 0; r < GRID_N; r++) {
        for (int c = 0; c < GRID_N; c++) {
            if (g->grid[r][c]) occupied++;
        }
    }

    int is_tight    = (occupied >= 24); /* > 37% full */
    int is_crowded  = (occupied >= 32); /* > 50% full: emergency small pieces only */

    /* Tier pools */
    static const int small_pool[]  = {0, 1, 2, 3, 4, 13, 14, 15, 16}; /* 1x1, 2-bars, 3-bars, small corners */
    static const int simple_pool[] = {0, 1, 2, 3, 4, 9, 13, 14, 15, 16, 29, 30, 31, 32}; /* adds 2x2 and simple L/J */
    static const int med_pool[]    = {5, 6, 9, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};
    static const int large_pool[]  = {7, 8, 10, 11, 12, 17, 18, 19, 20, 33};

    int n_small  = (int)(sizeof(small_pool) / sizeof(small_pool[0]));
    int n_simple = (int)(sizeof(simple_pool) / sizeof(simple_pool[0]));
    int n_med    = (int)(sizeof(med_pool) / sizeof(med_pool[0]));
    int n_large  = (int)(sizeof(large_pool) / sizeof(large_pool[0]));

    /* Slot 0: Clear-enabler assist (streak feeder) or guaranteed small filler */
    int s0 = -1;
    int clear_candidates[NUM_SHAPES];
    int n_clear = 0;
    for (int s = 0; s < NUM_SHAPES; s++) {
        if (shape_can_clear(g, s)) {
            clear_candidates[n_clear++] = s;
        }
    }

    if (n_clear > 0 && rng_range(100) < 85) {
        /* 85% chance to hand the player a piece that will complete their setup! */
        s0 = clear_candidates[rng_range(n_clear)];
    } else {
        for (int tries = 0; tries < 25; tries++) {
            int candidate = small_pool[rng_range(n_small)];
            if (shape_can_fit(g, candidate)) {
                s0 = candidate;
                break;
            }
        }
        if (s0 == -1) s0 = small_pool[rng_range(n_small)];
    }

    /* Slot 1: Standard piece (small pool if crowded, simple pool if tight) */
    int s1 = -1;
    for (int tries = 0; tries < 25; tries++) {
        int candidate;
        if (is_crowded) {
            candidate = small_pool[rng_range(n_small)];
        } else if (is_tight) {
            candidate = simple_pool[rng_range(n_simple)];
        } else {
            candidate = (rng_range(100) < 65) ? med_pool[rng_range(n_med)] : simple_pool[rng_range(n_simple)];
        }
        if (shape_can_fit(g, candidate) && candidate != s0) {
            s1 = candidate;
            break;
        }
    }
    if (s1 == -1) s1 = small_pool[rng_range(n_small)];

    /* Slot 2: Situational (strictly small when crowded or tight) */
    int s2 = -1;
    if (is_crowded) {
        for (int tries = 0; tries < 25; tries++) {
            int candidate = small_pool[rng_range(n_small)];
            if (shape_can_fit(g, candidate) && candidate != s0 && candidate != s1) {
                s2 = candidate;
                break;
            }
        }
        if (s2 == -1) s2 = small_pool[0]; /* 1x1 dot emergency fallback */
    } else if (is_tight) {
        for (int tries = 0; tries < 25; tries++) {
            int candidate = simple_pool[rng_range(n_simple)];
            if (shape_can_fit(g, candidate) && candidate != s0 && candidate != s1) {
                s2 = candidate;
                break;
            }
        }
        if (s2 == -1) s2 = small_pool[rng_range(n_small)];
    } else {
        if (rng_range(100) < 40) {
            s2 = large_pool[rng_range(n_large)];
        } else {
            s2 = med_pool[rng_range(n_med)];
        }
    }

    g->pieces[0].shape = s0;
    g->pieces[1].shape = s1;
    g->pieces[2].shape = s2;

    /* ─── SOLVABILITY / ANTI-LOCKOUT GUARANTEE ─── */
    /* Ensure that every single piece in the trio can be placed */
    for (int i = 0; i < 3; i++) {
        if (!shape_can_fit(g, g->pieces[i].shape)) {
            for (int s = 0; s < n_small; s++) {
                if (shape_can_fit(g, small_pool[s])) {
                    g->pieces[i].shape = small_pool[s];
                    break;
                }
            }
        }
    }

    /* Assign canonical color per shape */
    for (int i = 0; i < 3; i++) {
        g->pieces[i].color = get_shape_color(g->pieces[i].shape);
        g->pieces[i].used  = 0;
    }
}

static int any_piece_fits(const Game *g) {
    for (int p = 0; p < 3; p++) {
        if (g->pieces[p].used) continue;
        const Shape *s = &shapes[g->pieces[p].shape];
        for (int r = 0; r <= GRID_N - s->h; r++) {
            for (int c = 0; c <= GRID_N - s->w; c++) {
                if (can_place(g, s, r, c)) return 1;
            }
        }
    }
    if (g->hold.active && !g->hold_locked) {
        const Shape *s = &shapes[g->hold.shape];
        for (int r = 0; r <= GRID_N - s->h; r++) {
            for (int c = 0; c <= GRID_N - s->w; c++) {
                if (can_place(g, s, r, c)) return 1;
            }
        }
    }
    return 0;
}

static void clamp_cursor(Game *g) {
    if (g->hammer_mode) {
        if (g->cursor_r < 0) g->cursor_r = 0;
        if (g->cursor_r >= GRID_N) g->cursor_r = GRID_N - 1;
        if (g->cursor_c < 0) g->cursor_c = 0;
        if (g->cursor_c >= GRID_N) g->cursor_c = GRID_N - 1;
        return;
    }
    if (g->pieces[g->sel].used) return;
    const Shape *s = &shapes[g->pieces[g->sel].shape];
    if (g->cursor_r > GRID_N - s->h) g->cursor_r = GRID_N - s->h;
    if (g->cursor_c > GRID_N - s->w) g->cursor_c = GRID_N - s->w;
    if (g->cursor_r < 0) g->cursor_r = 0;
    if (g->cursor_c < 0) g->cursor_c = 0;
}

static void game_init(Game *g) {
    int cur_coins = save_data.coins;
    int best = save_data.high_score;
    memset(g, 0, sizeof(*g));
    g->coins = cur_coins;
    g->high_score = best;
    g->cursor_r = 3;
    g->cursor_c = 3;
    g->hammer_charges = 0;
    generate_pieces(g);
    clamp_cursor(g);
}

static void execute_revive(Game *g) {
    int cost = get_revive_cost(g);
    if (g->coins < cost) return;

    g->coins -= cost;
    save_data.coins = g->coins;
    save_game_data();

    g->revive_modal = 0;
    g->revive_count++;

    /* Clear central 4x4 area with explosion flash */
    for (int r = 2; r <= 5; r++) {
        for (int c = 2; c <= 5; c++) {
            g->grid[r][c] = 0;
            g->flash[r][c] = 1;
        }
    }
    g->flash_timer = 20;

    /* Refill 3 fresh guaranteed-fitting pieces */
    generate_pieces(g);
    g->sel = 0;
    clamp_cursor(g);
    if (g->streak > 0) {
        g->streak_moves_left = 3;
    }

    play_sound(snd_clear, SND_CLEAR_SAMPLES);
}

static void check_clears(Game *g) {
    int rows[GRID_N] = {0};
    int cols[GRID_N] = {0};
    int total_lines = 0;

    for (int r = 0; r < GRID_N; r++) {
        int full = 1;
        for (int c = 0; c < GRID_N; c++) {
            if (g->grid[r][c] == 0) { full = 0; break; }
        }
        if (full) { rows[r] = 1; total_lines++; }
    }

    for (int c = 0; c < GRID_N; c++) {
        int full = 1;
        for (int r = 0; r < GRID_N; r++) {
            if (g->grid[r][c] == 0) { full = 0; break; }
        }
        if (full) { cols[c] = 1; total_lines++; }
    }

    if (total_lines == 0) {
        /* No lines cleared on this turn -> Decrement 3-move streak grace counter */
        g->combo = 0;
        if (g->streak > 0) {
            g->streak_moves_left--;
            if (g->streak_moves_left <= 0) {
                g->streak = 0;
                g->streak_moves_left = 0;
                g->streak_lost_timer = 45;
            }
        }
        return;
    }

    /* Turn with lines cleared -> STREAK increases & 3-move grace counter resets! */
    g->streak++;
    g->streak_moves_left = 3;
    g->combo = total_lines;

    for (int r = 0; r < GRID_N; r++) {
        for (int c = 0; c < GRID_N; c++) {
            if (rows[r] || cols[c]) {
                g->flash[r][c] = 1;
            }
        }
    }
    g->flash_timer = 14;

    /* Score calculation: Base + Multi-line clear + Streak multiplier */
    int base_pts = total_lines * 100;
    int multi_pts = (total_lines > 1) ? ((total_lines - 1) * 150) : 0;
    int streak_pts = (g->streak > 1) ? (g->streak * 60) : 0;
    int total_pts = base_pts + multi_pts + streak_pts;

    g->score += total_pts;
    g->popup_val = total_pts;
    g->popup_timer = 45;

    /* Award coins based on lines and streak */
    int earned_coins = total_lines * 5;
    if (total_lines > 1) earned_coins += (total_lines - 1) * 8;
    if (g->streak > 1) earned_coins += g->streak * 2;

    g->coins += earned_coins;
    save_data.coins = g->coins;

    g->coin_popup_val = earned_coins;
    g->coin_popup_timer = 40;

    /* Audio: Play authentic ascending scale note for each consecutive streak tier */
    int note_idx = g->streak - 1;
    if (note_idx >= NUM_SCALE_NOTES) note_idx = NUM_SCALE_NOTES - 1;
    play_sound(snd_scale[note_idx], snd_scale_samples[note_idx]);

    /* If multiple lines cleared at once, also play the line clear sweep! */
    if (total_lines > 1) {
        play_sound(snd_clear, SND_CLEAR_SAMPLES);
    }

    /* Fanfare display timers */
    g->combo_display = total_lines;
    g->streak_display = g->streak;
    g->combo_timer = 55;

    if (g->score > g->high_score) {
        g->high_score = g->score;
        save_data.high_score = g->high_score;
    }
    save_game_data();

    for (int r = 0; r < GRID_N; r++) {
        for (int c = 0; c < GRID_N; c++) {
            if (rows[r] || cols[c]) {
                g->grid[r][c] = 0;
            }
        }
    }
}

static void place_selected_piece(Game *g) {
    if (g->game_over || g->revive_modal || g->pieces[g->sel].used) return;

    const Shape *s = &shapes[g->pieces[g->sel].shape];
    if (!can_place(g, s, g->cursor_r, g->cursor_c)) return;

    u8 col_idx = (u8)(g->pieces[g->sel].color + 1);
    for (int i = 0; i < s->n; i++) {
        int r = g->cursor_r + s->c[i][1];
        int c = g->cursor_c + s->c[i][0];
        g->grid[r][c] = col_idx;
    }

    g->score += s->n * 10;
    g->pieces[g->sel].used = 1;
    g->hold_locked = 0;

    /* Play authentic wooden block place thump */
    play_sound(snd_place, SND_PLACE_SAMPLES);

    check_clears(g);

    if (g->pieces[0].used && g->pieces[1].used && g->pieces[2].used) {
        generate_pieces(g);
    }

    for (int i = 0; i < 3; i++) {
        int idx = (g->sel + i) % 3;
        if (!g->pieces[idx].used) {
            g->sel = idx;
            break;
        }
    }
    clamp_cursor(g);

    if (!any_piece_fits(g)) {
        g->revive_modal = 1;
        play_sound(snd_gameover, SND_GAMEOVER_SAMPLES);
    }
}

static void cycle_selection(Game *g, int dir) {
    if (g->game_over || g->revive_modal) return;
    for (int i = 1; i <= 3; i++) {
        int idx = (g->sel + dir * i + 3) % 3;
        if (!g->pieces[idx].used) {
            g->sel = idx;
            clamp_cursor(g);
            play_sound(snd_snap, SND_SNAP_SAMPLES);
            return;
        }
    }
}

static void hold_selected_piece(Game *g) {
    if (g->game_over || g->revive_modal || g->hammer_mode) return;
    if (g->pieces[g->sel].used || g->hold_locked) return;

    if (!g->hold.active) {
        g->hold.shape  = g->pieces[g->sel].shape;
        g->hold.color  = g->pieces[g->sel].color;
        g->hold.active = 1;
        g->pieces[g->sel].used = 1;

        int all_used = 1;
        for (int i = 0; i < 3; i++) {
            if (!g->pieces[i].used) { all_used = 0; break; }
        }
        if (all_used) {
            generate_pieces(g);
        } else {
            cycle_selection(g, 1);
        }
    } else {
        int temp_shape = g->pieces[g->sel].shape;
        int temp_color = g->pieces[g->sel].color;

        g->pieces[g->sel].shape = g->hold.shape;
        g->pieces[g->sel].color = g->hold.color;

        g->hold.shape = temp_shape;
        g->hold.color = temp_color;
    }

    g->hold_locked = 1;
    clamp_cursor(g);
    play_sound(snd_snap, SND_SNAP_SAMPLES);
}

static void execute_hammer_blast(Game *g) {
    if (g->hammer_charges > 0) {
        g->hammer_charges--;
    } else if (g->coins >= BOMB_COST) {
        g->coins -= BOMB_COST;
        save_data.coins = g->coins;
        save_game_data();
    } else {
        play_sound(snd_gameover, SND_GAMEOVER_SAMPLES);
        g->hammer_mode = 0;
        return;
    }

    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            int r = g->cursor_r + dr;
            int c = g->cursor_c + dc;
            if (r >= 0 && r < GRID_N && c >= 0 && c < GRID_N) {
                g->grid[r][c] = 0;
                g->flash[r][c] = 1;
            }
        }
    }
    g->flash_timer = 18;
    g->hammer_mode = 0;
    clamp_cursor(g);
    play_sound(snd_clear, SND_CLEAR_SAMPLES);
}

/* ─── RENDERING ──────────────────────────────────────────────────────── */

static void render_background(void) {
    /* Authentic vertical midnight navy gradient */
    for (int y = 0; y < SCR_H; y++) {
        int t = y * 100 / SCR_H;
        int r = (12 * (100 - t) + 8 * t) / 100;
        int g = (20 * (100 - t) + 14 * t) / 100;
        int b = (52 * (100 - t) + 36 * t) / 100;
        u32 c = RGB(r, g, b);
        u32 *row = fb_draw + y * BUF_W;
        for (int x = 0; x < SCR_W; x++) {
            row[x] = c;
        }
    }
}

static void render_header(const Game *g) {
    /* Dynamically centered score with drop shadow */
    int d = count_digits(g->score);
    int sw = d * 24; /* scale 3: 24px width per digit */
    int sx = 240 - sw / 2;
    draw_int(sx + 2, 6, g->score, RGB(10, 16, 42), 3); /* drop shadow */
    draw_int(sx, 4, g->score, COL_TEXT_WHITE, 3);       /* main score */

    /* Left row 1: BrickBlast logo */
    draw_str(14, 6, "BRICK", COL_TEXT_WHITE, 1);
    draw_str(58, 6, "BLAST", COL_CYAN_GLOW, 1);

    /* Left row 2: Active streak indicator or subtitle */
    if (g->streak >= 1) {
        u32 flame = ((g->anim_tick / 6) % 2) ? RGB(255, 120, 20) : COL_TEXT_GOLD;
        draw_str(14, 20, "STREAK x", flame, 1);
        draw_int(78, 20, g->streak, RGB(255, 255, 255), 1);

        /* 3 grace moves pips */
        int pip_x = (g->streak >= 10) ? 104 : 96;
        for (int p = 0; p < 3; p++) {
            int px = pip_x + p * 8;
            if (p < g->streak_moves_left) {
                u32 pip_col = (g->streak_moves_left == 1) ? RGB(255, 60, 60) : RGB(255, 180, 40);
                fill_rect(px, 22, 5, 5, pip_col);
                draw_border(px, 22, 5, 5, 1, RGB(255, 255, 255));
            } else {
                fill_rect(px, 22, 5, 5, RGB(30, 36, 60));
                draw_border(px, 22, 5, 5, 1, COL_BOARD_EDGE);
            }
        }
    } else if (g->streak_lost_timer > 0) {
        draw_str(14, 20, "STREAK LOST", COL_ALERT_RED, 1);
    } else {
        draw_str(14, 20, "PSP EDITION", COL_TEXT_DIM, 1);
    }

    /* Right row 1: Best score */
    draw_str(346, 6, "# BEST", COL_TEXT_GOLD, 1);
    draw_int(402, 6, g->high_score, COL_TEXT_WHITE, 1);

    /* Right row 2: Coins balance */
    draw_str(346, 20, "COINS", COL_TEXT_GOLD, 1);
    draw_int(402, 20, g->coins, COL_TEXT_GOLD, 1);
}

static void render_grid(const Game *g) {
    /* Rounded board card container */
    draw_border(BOARD_X, BOARD_Y, BOARD_TOTAL, BOARD_TOTAL, BOARD_BORDER, COL_BOARD_EDGE);
    fill_rect(BOARD_X + BOARD_BORDER, BOARD_Y + BOARD_BORDER, GRID_INNER, GRID_INNER, COL_BOARD_CARD);

    /* Grid sockets & blocks */
    for (int r = 0; r < GRID_N; r++) {
        for (int c = 0; c < GRID_N; c++) {
            int cx = BOARD_X + BOARD_BORDER + c * (CELL_SZ + CELL_GAP);
            int cy = BOARD_Y + BOARD_BORDER + r * (CELL_SZ + CELL_GAP);

            if (g->flash[r][c] && g->flash_timer > 0) {
                fill_rect(cx, cy, CELL_SZ, CELL_SZ, COL_FLASH);
            } else if (g->grid[r][c] != 0) {
                u32 col = piece_colors[g->grid[r][c] - 1];
                draw_gem_block(cx, cy, CELL_SZ, col);
            } else {
                draw_socket(cx, cy, CELL_SZ);
            }
        }
    }

    /* Glowing clear preview lines */
    int prev_rows[GRID_N];
    int prev_cols[GRID_N];
    int num_clears = 0;
    get_clears_preview(g, prev_rows, prev_cols, &num_clears);

    if (num_clears > 0) {
        u32 glow_col = ((g->anim_tick / 4) % 2) ? RGB(255, 235, 70) : RGB(60, 240, 255);
        for (int r = 0; r < GRID_N; r++) {
            if (prev_rows[r]) {
                int ry = BOARD_Y + BOARD_BORDER + r * (CELL_SZ + CELL_GAP);
                draw_border(BOARD_X + BOARD_BORDER - 1, ry - 1, GRID_INNER + 2, CELL_SZ + 2, 2, glow_col);
            }
        }
        for (int c = 0; c < GRID_N; c++) {
            if (prev_cols[c]) {
                int cx = BOARD_X + BOARD_BORDER + c * (CELL_SZ + CELL_GAP);
                draw_border(cx - 1, BOARD_Y + BOARD_BORDER - 1, CELL_SZ + 2, GRID_INNER + 2, 2, glow_col);
            }
        }
    }
}

static void render_preview(const Game *g) {
    if (g->game_over || g->revive_modal) return;

    if (g->hammer_mode) {
        u32 blast_col = ((g->anim_tick / 6) % 2) ? RGB(255, 60, 60) : RGB(255, 220, 40);
        for (int dr = -1; dr <= 1; dr++) {
            for (int dc = -1; dc <= 1; dc++) {
                int r = g->cursor_r + dr;
                int c = g->cursor_c + dc;
                if (r >= 0 && r < GRID_N && c >= 0 && c < GRID_N) {
                    int cx = BOARD_X + BOARD_BORDER + c * (CELL_SZ + CELL_GAP);
                    int cy = BOARD_Y + BOARD_BORDER + r * (CELL_SZ + CELL_GAP);
                    draw_border(cx, cy, CELL_SZ, CELL_SZ, 2, blast_col);
                }
            }
        }
        return;
    }

    if (g->pieces[g->sel].used) return;

    const Shape *s = &shapes[g->pieces[g->sel].shape];
    int valid = can_place(g, s, g->cursor_r, g->cursor_c);
    u32 piece_col = piece_colors[g->pieces[g->sel].color];

    for (int i = 0; i < s->n; i++) {
        int r = g->cursor_r + s->c[i][1];
        int c = g->cursor_c + s->c[i][0];
        if (r < 0 || r >= GRID_N || c < 0 || c >= GRID_N) continue;

        int cx = BOARD_X + BOARD_BORDER + c * (CELL_SZ + CELL_GAP);
        int cy = BOARD_Y + BOARD_BORDER + r * (CELL_SZ + CELL_GAP);

        draw_ghost_gem(cx, cy, CELL_SZ, piece_col, valid);
    }
}

static void render_panel(const Game *g) {
    /* Right tray panel */
    draw_border(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, 2, COL_BOARD_EDGE);
    fill_rect(PANEL_X + 2, PANEL_Y + 2, PANEL_W - 4, PANEL_H - 4, COL_BOARD_CARD);

    int slot_w = PANEL_W - 16;
    int slot_h = 36;
    int slot_x = PANEL_X + 8;
    int mini_cell = 9;

    for (int i = 0; i < 3; i++) {
        int slot_y = PANEL_Y + 8 + i * 42;

        fill_rect(slot_x, slot_y, slot_w, slot_h, COL_TRAY_BG);

        if (i == g->sel && !g->pieces[i].used && !g->hammer_mode) {
            u32 glow = ((g->anim_tick / 8) % 2) ? RGB(255, 255, 255) : COL_CYAN_GLOW;
            draw_border(slot_x - 1, slot_y - 1, slot_w + 2, slot_h + 2, 2, glow);
        } else {
            draw_border(slot_x, slot_y, slot_w, slot_h, 1, COL_BOARD_EDGE);
        }

        if (g->pieces[i].used) {
            draw_str(slot_x + slot_w / 2 - 16, slot_y + slot_h / 2 - 4, "USED", COL_TEXT_DIM, 1);
            continue;
        }

        const Shape *sh = &shapes[g->pieces[i].shape];
        u32 col = piece_colors[g->pieces[i].color];
        if (i != g->sel || g->hammer_mode) col = col_darken(col, 65);

        int pw = sh->w * (mini_cell + 1);
        int ph = sh->h * (mini_cell + 1);
        int ox = slot_x + (slot_w - pw) / 2;
        int oy = slot_y + (slot_h - ph) / 2;

        for (int j = 0; j < sh->n; j++) {
            int bx = ox + sh->c[j][0] * (mini_cell + 1);
            int by = oy + sh->c[j][1] * (mini_cell + 1);
            draw_gem_block(bx, by, mini_cell, col);
        }
    }

    /* Power Tools Row: HOLD and BOMB */
    int tool_y = PANEL_Y + 138;
    int tool_w = (slot_w - 6) / 2;
    int tool_h = 42;

    /* HOLD Slot [SQ] */
    int hold_x = slot_x;
    fill_rect(hold_x, tool_y, tool_w, tool_h, COL_TRAY_BG);
    u32 hold_border = g->hold_locked ? COL_BOARD_EDGE : COL_CYAN_GLOW;
    draw_border(hold_x, tool_y, tool_w, tool_h, 1, hold_border);
    draw_str(hold_x + 6, tool_y + 3, "[SQ] HOLD", g->hold_locked ? COL_TEXT_DIM : COL_TEXT_WHITE, 1);

    if (g->hold.active) {
        const Shape *sh = &shapes[g->hold.shape];
        u32 col = piece_colors[g->hold.color];
        if (g->hold_locked) col = col_darken(col, 60);

        int pw = sh->w * (mini_cell + 1);
        int ph = sh->h * (mini_cell + 1);
        int ox = hold_x + (tool_w - pw) / 2;
        int oy = tool_y + 14 + (24 - ph) / 2;

        for (int j = 0; j < sh->n; j++) {
            int bx = ox + sh->c[j][0] * (mini_cell + 1);
            int by = oy + sh->c[j][1] * (mini_cell + 1);
            draw_gem_block(bx, by, mini_cell, col);
        }
    } else {
        draw_str(hold_x + 28, tool_y + 20, "EMPTY", COL_TEXT_DIM, 1);
    }

    /* BOMB Slot [TRI] */
    int bomb_x = slot_x + tool_w + 6;
    fill_rect(bomb_x, tool_y, tool_w, tool_h, COL_TRAY_BG);
    if (g->hammer_mode) {
        u32 bflash = ((g->anim_tick / 6) % 2) ? RGB(255, 60, 60) : RGB(255, 220, 40);
        draw_border(bomb_x - 1, tool_y - 1, tool_w + 2, tool_h + 2, 2, bflash);
    } else {
        draw_border(bomb_x, tool_y, tool_w, tool_h, 1, COL_TEXT_GOLD);
    }
    draw_str(bomb_x + 6, tool_y + 3, "[^] BOMB", g->hammer_mode ? RGB(255, 80, 80) : COL_TEXT_GOLD, 1);

    if (g->hammer_charges > 0) {
        char chg_buf[32];
        snprintf(chg_buf, sizeof(chg_buf), "FREE x%d", g->hammer_charges);
        draw_str(bomb_x + 18, tool_y + 20, chg_buf, RGB(80, 255, 120), 1);
    } else {
        draw_str(bomb_x + 10, tool_y + 20, "300 COINS", (g->coins >= BOMB_COST) ? COL_TEXT_GOLD : COL_ALERT_RED, 1);
    }

    /* Helper hints */
    fill_rect(PANEL_X + 8, PANEL_Y + 186, PANEL_W - 16, 1, COL_BOARD_EDGE);
    draw_str(PANEL_X + 12, PANEL_Y + 192, "D-PAD: MOVE   L/R: SELECT", COL_TEXT_DIM, 1);
    draw_str(PANEL_X + 12, PANEL_Y + 205, "[X]: DROP  [SQ]: HOLD  [^]: BOMB", COL_TEXT_WHITE, 1);
}

static void render_overlays(const Game *g) {
    /* Authentic Streak & Combo Fanfare Banner */
    if (g->combo_timer > 0) {
        char banner[32];
        if (g->streak_display > 1 && g->combo_display > 1) {
            if (g->combo_display == 2) snprintf(banner, sizeof(banner), "DOUBLE! STREAK %d", g->streak_display);
            else if (g->combo_display == 3) snprintf(banner, sizeof(banner), "TRIPLE! STREAK %d", g->streak_display);
            else snprintf(banner, sizeof(banner), "MEGA! STREAK %d", g->streak_display);
        } else if (g->streak_display > 1) {
            snprintf(banner, sizeof(banner), "STREAK x%d", g->streak_display);
        } else if (g->combo_display > 1) {
            if (g->combo_display == 2) snprintf(banner, sizeof(banner), "DOUBLE CLEAR!");
            else if (g->combo_display == 3) snprintf(banner, sizeof(banner), "TRIPLE CLEAR!");
            else snprintf(banner, sizeof(banner), "MEGA CLEAR!");
        } else {
            snprintf(banner, sizeof(banner), "+100");
        }

        int len = (int)strlen(banner);
        int text_scale = (len > 11) ? 1 : 2;
        int banner_w = len * 8 * text_scale;
        int cx = BOARD_X + (BOARD_TOTAL - banner_w) / 2;
        int cy = BOARD_Y + BOARD_TOTAL - ((text_scale == 2) ? 22 : 18);
        int pill_h = (text_scale == 2) ? 24 : 16;

        /* Glowing backdrop badge pill */
        fill_rect(cx - 8, cy - 4, banner_w + 16, pill_h, RGB(10, 16, 44));
        draw_border(cx - 8, cy - 4, banner_w + 16, pill_h, 2, RGB(255, 140, 20));

        /* Fiery banner text */
        draw_str(cx + 1, cy + 1, banner, RGB(12, 16, 36), text_scale);
        u32 txt_col = (g->streak_display > 2) ? RGB(255, 215, 30) : COL_TEXT_WHITE;
        draw_str(cx, cy, banner, txt_col, text_scale);
    }

    /* Score popup */
    if (g->popup_timer > 0) {
        int py = BOARD_Y + 40 - (45 - g->popup_timer) / 2;
        int px = BOARD_X + 45;
        draw_char(px, py, '+', COL_CYAN_GLOW, 2);
        draw_int(px + 18, py, g->popup_val, COL_CYAN_GLOW, 2);
    }

    /* Floating Coin popup */
    if (g->coin_popup_timer > 0) {
        int cy = BOARD_Y + 68 - (40 - g->coin_popup_timer) / 2;
        int cx = BOARD_X + 40;
        char cbuf[24];
        snprintf(cbuf, sizeof(cbuf), "+%d COINS", g->coin_popup_val);
        draw_str(cx + 1, cy + 1, cbuf, RGB(10, 16, 40), 1);
        draw_str(cx, cy, cbuf, COL_TEXT_GOLD, 1);
    }

    /* Revive / Second Chance Modal */
    if (g->revive_modal) {
        int cost = get_revive_cost(g);
        int modal_w = 340;
        int modal_h = 195;
        int mx = (SCR_W - modal_w) / 2;
        int my = (SCR_H - modal_h) / 2;

        fill_rect(mx, my, modal_w, modal_h, RGB(14, 22, 54));
        draw_border(mx, my, modal_w, modal_h, 3, RGB(255, 180, 40));

        draw_str(mx + 75, my + 10, "OUT OF MOVES!", RGB(255, 200, 40), 2);

        char stats[64];
        snprintf(stats, sizeof(stats), "SCORE: %d   STREAK: %d   COINS: %d", g->score, g->streak, g->coins);
        draw_str(mx + 28, my + 34, stats, COL_TEXT_WHITE, 1);

        /* Option 1: Revive */
        if (g->coins >= cost) {
            u32 pulse = ((g->anim_tick / 10) % 2) ? RGB(60, 240, 130) : RGB(40, 190, 100);
            draw_border(mx + 18, my + 52, modal_w - 36, 34, 2, pulse);
            fill_rect(mx + 20, my + 54, modal_w - 40, 30, RGB(16, 36, 40));
            char btn_txt[64];
            snprintf(btn_txt, sizeof(btn_txt), "[X] REVIVE #%d (%d COINS)", g->revive_count + 1, cost);
            draw_str(mx + 28, my + 58, btn_txt, pulse, 1);
            draw_str(mx + 28, my + 72, "CLEARS 4X4 CENTER & RESTOCKS", COL_TEXT_DIM, 1);
        } else {
            draw_border(mx + 18, my + 52, modal_w - 36, 34, 1, COL_ALERT_RED);
            fill_rect(mx + 20, my + 54, modal_w - 40, 30, RGB(34, 16, 24));
            char btn_txt[64];
            snprintf(btn_txt, sizeof(btn_txt), "[X] REVIVE (%d COINS - NEED %d)", cost, cost - g->coins);
            draw_str(mx + 28, my + 58, btn_txt, COL_ALERT_RED, 1);
            draw_str(mx + 28, my + 72, "NOT ENOUGH COINS FOR REVIVE", COL_TEXT_DIM, 1);
        }

        /* Option 2: Bomb */
        int by = my + 94;
        if (g->hammer_charges > 0 || g->coins >= BOMB_COST) {
            u32 bpulse = ((g->anim_tick / 10) % 2) ? RGB(255, 200, 40) : RGB(220, 160, 30);
            draw_border(mx + 18, by, modal_w - 36, 34, 2, bpulse);
            fill_rect(mx + 20, by + 2, modal_w - 40, 30, RGB(40, 30, 16));
            char b_txt[64];
            if (g->hammer_charges > 0) snprintf(b_txt, sizeof(b_txt), "[^] USE BOMB (FREE x%d)", g->hammer_charges);
            else snprintf(b_txt, sizeof(b_txt), "[^] BUY BOMB (%d COINS)", BOMB_COST);
            draw_str(mx + 28, by + 6, b_txt, bpulse, 1);
            draw_str(mx + 28, by + 20, "BLASTS ANY 3X3 AREA OF BOARD", COL_TEXT_DIM, 1);
        } else {
            draw_border(mx + 18, by, modal_w - 36, 34, 1, COL_BOARD_EDGE);
            fill_rect(mx + 20, by + 2, modal_w - 40, 30, RGB(26, 20, 24));
            char b_txt[64];
            snprintf(b_txt, sizeof(b_txt), "[^] BOMB (%d COINS - NEED %d)", BOMB_COST, BOMB_COST - g->coins);
            draw_str(mx + 28, by + 6, b_txt, COL_TEXT_DIM, 1);
            draw_str(mx + 28, by + 20, "BLASTS 3X3 SECTION OF BOARD", COL_TEXT_DIM, 1);
        }

        /* Option 3: Give Up */
        u32 dim_pulse = ((g->anim_tick / 15) % 2) ? COL_TEXT_WHITE : COL_TEXT_DIM;
        draw_str(mx + 60, my + 144, "[O] OR [START] GIVE UP", dim_pulse, 1);
    }
    /* Game Over Modal */
    else if (g->game_over) {
        int modal_w = 340;
        int modal_h = 168;
        int mx = (SCR_W - modal_w) / 2;
        int my = (SCR_H - modal_h) / 2;

        fill_rect(mx, my, modal_w, modal_h, RGB(14, 22, 54));
        draw_border(mx, my, modal_w, modal_h, 3, COL_ALERT_RED);

        draw_str(mx + 60, my + 16, "GAME OVER", COL_ALERT_RED, 3);

        draw_str(mx + 55, my + 54, "FINAL SCORE", COL_TEXT_DIM, 1);
        draw_int(mx + 175, my + 52, g->score, COL_CYAN_GLOW, 2);

        draw_str(mx + 55, my + 76, "BEST SCORE", COL_TEXT_DIM, 1);
        draw_int(mx + 175, my + 76, g->high_score, COL_TEXT_GOLD, 1);

        draw_str(mx + 55, my + 94, "TOTAL COINS", COL_TEXT_DIM, 1);
        draw_int(mx + 175, my + 94, g->coins, COL_TEXT_GOLD, 1);

        draw_str(mx + 55, my + 112, "REVIVES USED", COL_TEXT_DIM, 1);
        draw_int(mx + 175, my + 112, g->revive_count, COL_TEXT_WHITE, 1);

        u32 pulse = ((g->anim_tick / 15) % 2) ? COL_TEXT_WHITE : COL_TEXT_DIM;
        draw_str(mx + 55, my + 138, "PRESS START TO RETRY", pulse, 1);
    }

    /* Pause Overlay Modal */
    if (g->paused) {
        int modal_w = 260;
        int modal_h = 100;
        int mx = (SCR_W - modal_w) / 2;
        int my = (SCR_H - modal_h) / 2;

        fill_rect(mx, my, modal_w, modal_h, RGB(14, 22, 54));
        draw_border(mx, my, modal_w, modal_h, 2, COL_CYAN_GLOW);

        draw_str(mx + 68, my + 18, "PAUSED", COL_CYAN_GLOW, 2);
        u32 pulse = ((g->anim_tick / 15) % 2) ? COL_TEXT_WHITE : COL_TEXT_DIM;
        draw_str(mx + 30, my + 52, "PRESS START TO RESUME", pulse, 1);
        draw_str(mx + 28, my + 70, "PRESS SELECT TO RESTART", COL_TEXT_DIM, 1);
    }
}

/* ─── INPUT HANDLING ─────────────────────────────────────────────────── */

#define REP_DELAY 12
#define REP_RATE  4

static SceCtrlData pad, pad_prev;
static int rep_timers[4];

static int is_button_pressed(u32 btn) {
    return (pad.Buttons & btn);
}

static int is_button_tapped(u32 btn) {
    return (pad.Buttons & btn) && !(pad_prev.Buttons & btn);
}

static int handle_directional_repeat(int is_active, int timer_idx) {
    if (!is_active) {
        rep_timers[timer_idx] = 0;
        return 0;
    }
    if (rep_timers[timer_idx] == 0) {
        rep_timers[timer_idx] = 1;
        return 1;
    }
    rep_timers[timer_idx]++;
    if (rep_timers[timer_idx] >= REP_DELAY &&
        (rep_timers[timer_idx] - REP_DELAY) % REP_RATE == 0) {
        return 1;
    }
    return 0;
}

static void process_input(Game *g) {
    pad_prev = pad;
    sceCtrlReadBufferPositive(&pad, 1);

    if (g->paused) {
        if (is_button_tapped(PSP_CTRL_START)) {
            g->paused = 0;
        } else if (is_button_tapped(PSP_CTRL_SELECT)) {
            g->paused = 0;
            game_init(g);
        }
        return;
    }

    if (g->revive_modal) {
        if (is_button_tapped(PSP_CTRL_CROSS)) {
            if (g->coins >= get_revive_cost(g)) {
                execute_revive(g);
            }
        } else if (is_button_tapped(PSP_CTRL_TRIANGLE)) {
            if (g->hammer_charges > 0 || g->coins >= BOMB_COST) {
                g->revive_modal = 0;
                g->hammer_mode = 1;
                clamp_cursor(g);
                play_sound(snd_snap, SND_SNAP_SAMPLES);
            }
        } else if (is_button_tapped(PSP_CTRL_CIRCLE) || is_button_tapped(PSP_CTRL_START)) {
            g->revive_modal = 0;
            g->game_over = 1;
        }
        return;
    }

    if (g->game_over) {
        if (is_button_tapped(PSP_CTRL_START) || is_button_tapped(PSP_CTRL_CROSS)) {
            game_init(g);
        }
        return;
    }

    if (is_button_tapped(PSP_CTRL_START)) {
        g->paused = 1;
        return;
    }

    int up    = is_button_pressed(PSP_CTRL_UP)    || (pad.Ly < 70);
    int down  = is_button_pressed(PSP_CTRL_DOWN)  || (pad.Ly > 185);
    int left  = is_button_pressed(PSP_CTRL_LEFT)  || (pad.Lx < 70);
    int right = is_button_pressed(PSP_CTRL_RIGHT) || (pad.Lx > 185);

    if (handle_directional_repeat(up, 0)) {
        if (g->cursor_r > 0) g->cursor_r--;
        clamp_cursor(g);
    }
    if (handle_directional_repeat(down, 1)) {
        g->cursor_r++;
        clamp_cursor(g);
    }
    if (handle_directional_repeat(left, 2)) {
        if (g->cursor_c > 0) g->cursor_c--;
        clamp_cursor(g);
    }
    if (handle_directional_repeat(right, 3)) {
        g->cursor_c++;
        clamp_cursor(g);
    }

    if (is_button_tapped(PSP_CTRL_LTRIGGER)) {
        if (!g->hammer_mode) cycle_selection(g, -1);
    }
    if (is_button_tapped(PSP_CTRL_RTRIGGER)) {
        if (!g->hammer_mode) cycle_selection(g, 1);
    }

    if (is_button_tapped(PSP_CTRL_SQUARE)) {
        hold_selected_piece(g);
    }

    if (is_button_tapped(PSP_CTRL_TRIANGLE)) {
        if (!g->hammer_mode) {
            if (g->hammer_charges > 0 || g->coins >= BOMB_COST) {
                g->hammer_mode = 1;
                clamp_cursor(g);
                play_sound(snd_snap, SND_SNAP_SAMPLES);
            } else {
                play_sound(snd_gameover, SND_GAMEOVER_SAMPLES);
            }
        } else {
            g->hammer_mode = 0;
            clamp_cursor(g);
            play_sound(snd_snap, SND_SNAP_SAMPLES);
        }
    }

    if (is_button_tapped(PSP_CTRL_CIRCLE)) {
        if (g->hammer_mode) {
            g->hammer_mode = 0;
            clamp_cursor(g);
            play_sound(snd_snap, SND_SNAP_SAMPLES);
        } else {
            cycle_selection(g, 1);
        }
    }

    if (is_button_tapped(PSP_CTRL_CROSS)) {
        if (g->hammer_mode) {
            execute_hammer_blast(g);
        } else {
            place_selected_piece(g);
        }
    }
}

/* ─── MAIN ───────────────────────────────────────────────────────────── */

int main(void) {
    setup_callbacks();
    init_display();
    init_shapes();

    scePowerSetClockFrequency(333, 333, 166);
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    /* Hardware Audio Subsystem Init */
    memset(audio_voices, 0, sizeof(audio_voices));
    bgm_pos = 0;
    bgm_enabled = 1;
    pspAudioInit();
    pspAudioSetVolume(0, 0x8000, 0x8000);
    pspAudioSetChannelCallback(0, audio_callback, NULL);

    rng_seed((unsigned int)sceKernelGetSystemTimeLow());

    load_save_data();

    Game game;
    game_init(&game);

    memset(&pad, 0, sizeof(pad));
    memset(&pad_prev, 0, sizeof(pad_prev));
    memset(rep_timers, 0, sizeof(rep_timers));

    while (running) {
        process_input(&game);

        if (!game.paused) {
            game.anim_tick++;
            if (game.flash_timer > 0) {
                game.flash_timer--;
                if (game.flash_timer == 0) {
                    memset(game.flash, 0, sizeof(game.flash));
                }
            }
            if (game.popup_timer > 0) game.popup_timer--;
            if (game.coin_popup_timer > 0) game.coin_popup_timer--;
            if (game.combo_timer > 0) game.combo_timer--;
            if (game.streak_lost_timer > 0) game.streak_lost_timer--;
        } else {
            game.anim_tick++;
        }

        render_background();
        render_header(&game);
        render_grid(&game);
        render_preview(&game);
        render_panel(&game);
        render_overlays(&game);

        flip_buffers();
    }

    pspAudioEnd();
    sceKernelExitGame();
    return 0;
}
