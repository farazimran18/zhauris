#include <stdint.h>
#include <string.h>

#include "platform.h"
#include "xparameters.h"
#include "xil_types.h"
#include "xuartlite_l.h"

#define X_MAX   640
#define Y_MAX   480
#define STRIDE  1024

typedef struct color_struct {
    uint8_t R;
    uint8_t G;
    uint8_t B;
} color;

#define UART_BASEADDR XPAR_AXI_UARTLITE_0_BASEADDR
#define NBYTES        256

static inline void setVideoMemAddr(volatile int *TFT_addr, int *DDR_addr) {
    TFT_addr[0] = (int)(uintptr_t)DDR_addr;
}

static inline void disableVGA(volatile int *TFT_addr) {
    TFT_addr[1] = 0x00;
}

static inline void enableVGA(volatile int *TFT_addr) {
    TFT_addr[1] = 0x01;
}

static inline int packPixel(color c) {
    int R = c.R & 0xF;
    int G = c.G & 0xF;
    int B = c.B & 0xF;
    return (R << 20) | (G << 12) | (B << 4);
}

static inline void writeRGB(int *DDR_addr, int x, int y, color c) {
    if ((unsigned)x >= X_MAX || (unsigned)y >= Y_MAX) {
        return;
    }
    DDR_addr[STRIDE * y + x] = packPixel(c);
}

static void drawFilledRect(int *DDR_addr, int x0, int y0, int w, int h, color c) {
    if (w <= 0 || h <= 0) {
        return;
    }

    int x1 = x0 + w;
    int y1 = y0 + h;

    if (x0 < 0) {
        x0 = 0;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    if (x1 > X_MAX) {
        x1 = X_MAX;
    }
    if (y1 > Y_MAX) {
        y1 = Y_MAX;
    }

    int pix = packPixel(c);
    for (int y = y0; y < y1; y++) {
        int idx = STRIDE * y + x0;
        for (int x = x0; x < x1; x++) {
            DDR_addr[idx++] = pix;
        }
    }
}

static void drawRectOutline(int *DDR_addr, int x, int y, int w, int h, int t, color c) {
    drawFilledRect(DDR_addr, x,         y,         w, t, c);
    drawFilledRect(DDR_addr, x,         y + h - t, w, t, c);
    drawFilledRect(DDR_addr, x,         y,         t, h, c);
    drawFilledRect(DDR_addr, x + w - t, y,         t, h, c);
}

static void clearScreen(int *DDR_addr) {
    const int words = STRIDE * Y_MAX;
    for (int i = 0; i < words; i++) {
        DDR_addr[i] = 0;
    }
}

static const uint8_t GLYPH_SPACE[7] = {0, 0, 0, 0, 0, 0, 0};
static const uint8_t GLYPH_DASH[7]  = {0, 0, 0, 0x1F, 0, 0, 0};
static const uint8_t GLYPH_COLON[7] = {0, 0x04, 0x04, 0, 0x04, 0x04, 0};

static const uint8_t GLYPH_0[7] = {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E};
static const uint8_t GLYPH_1[7] = {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E};
static const uint8_t GLYPH_2[7] = {0x0E, 0x11, 0x01, 0x06, 0x08, 0x10, 0x1F};
static const uint8_t GLYPH_3[7] = {0x1F, 0x01, 0x06, 0x01, 0x01, 0x11, 0x0E};
static const uint8_t GLYPH_4[7] = {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02};
static const uint8_t GLYPH_5[7] = {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E};
static const uint8_t GLYPH_6[7] = {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E};
static const uint8_t GLYPH_7[7] = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08};
static const uint8_t GLYPH_8[7] = {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E};
static const uint8_t GLYPH_9[7] = {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C};

static const uint8_t GLYPH_A[7] = {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
static const uint8_t GLYPH_B[7] = {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E};
static const uint8_t GLYPH_C[7] = {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E};
static const uint8_t GLYPH_D[7] = {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E};
static const uint8_t GLYPH_E[7] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F};
static const uint8_t GLYPH_F[7] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10};
static const uint8_t GLYPH_G[7] = {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F};
static const uint8_t GLYPH_H[7] = {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
static const uint8_t GLYPH_I[7] = {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E};
static const uint8_t GLYPH_J[7] = {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C};
static const uint8_t GLYPH_K[7] = {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11};
static const uint8_t GLYPH_L[7] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F};
static const uint8_t GLYPH_M[7] = {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11};
static const uint8_t GLYPH_N[7] = {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11};
static const uint8_t GLYPH_O[7] = {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
static const uint8_t GLYPH_P[7] = {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10};
static const uint8_t GLYPH_Q[7] = {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D};
static const uint8_t GLYPH_R[7] = {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11};
static const uint8_t GLYPH_S[7] = {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E};
static const uint8_t GLYPH_T[7] = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
static const uint8_t GLYPH_U[7] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
static const uint8_t GLYPH_V[7] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04};
static const uint8_t GLYPH_W[7] = {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11};
static const uint8_t GLYPH_X[7] = {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11};
static const uint8_t GLYPH_Y[7] = {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04};
static const uint8_t GLYPH_Z[7] = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F};

static const uint8_t *glyphFor(char ch) {
    if (ch >= 'a' && ch <= 'z') {
        ch = (char)(ch - 'a' + 'A');
    }

    switch (ch) {
        case ' ': return GLYPH_SPACE;
        case '-': return GLYPH_DASH;
        case ':': return GLYPH_COLON;
        case '0': return GLYPH_0;
        case '1': return GLYPH_1;
        case '2': return GLYPH_2;
        case '3': return GLYPH_3;
        case '4': return GLYPH_4;
        case '5': return GLYPH_5;
        case '6': return GLYPH_6;
        case '7': return GLYPH_7;
        case '8': return GLYPH_8;
        case '9': return GLYPH_9;
        case 'A': return GLYPH_A;
        case 'B': return GLYPH_B;
        case 'C': return GLYPH_C;
        case 'D': return GLYPH_D;
        case 'E': return GLYPH_E;
        case 'F': return GLYPH_F;
        case 'G': return GLYPH_G;
        case 'H': return GLYPH_H;
        case 'I': return GLYPH_I;
        case 'J': return GLYPH_J;
        case 'K': return GLYPH_K;
        case 'L': return GLYPH_L;
        case 'M': return GLYPH_M;
        case 'N': return GLYPH_N;
        case 'O': return GLYPH_O;
        case 'P': return GLYPH_P;
        case 'Q': return GLYPH_Q;
        case 'R': return GLYPH_R;
        case 'S': return GLYPH_S;
        case 'T': return GLYPH_T;
        case 'U': return GLYPH_U;
        case 'V': return GLYPH_V;
        case 'W': return GLYPH_W;
        case 'X': return GLYPH_X;
        case 'Y': return GLYPH_Y;
        case 'Z': return GLYPH_Z;
        default:  return GLYPH_SPACE;
    }
}

static void drawChar5x7(int *DDR, int x, int y, char ch, color fg, color bg, int scale) {
    const uint8_t *g = glyphFor(ch);

    for (int row = 0; row < 7; row++) {
        uint8_t bits = g[row];
        for (int col = 0; col < 5; col++) {
            int on = (bits >> (4 - col)) & 1;
            color cc = on ? fg : bg;
            drawFilledRect(DDR, x + col * scale, y + row * scale, scale, scale, cc);
        }
    }
}

static void drawString(int *DDR, int x, int y, const char *s, color fg, color bg, int scale) {
    while (*s) {
        drawChar5x7(DDR, x, y, *s++, fg, bg, scale);
        x += 6 * scale;
    }
}

static void drawUInt(int *DDR, int x, int y, uint32_t v, color fg, color bg, int scale) {
    char buf[11];
    int n = 0;

    if (v == 0) {
        buf[n++] = '0';
    } else {
        char tmp[11];
        int t = 0;
        while (v && t < 10) {
            tmp[t++] = (char)('0' + (v % 10));
            v /= 10;
        }
        while (t--) {
            buf[n++] = tmp[t];
        }
    }

    buf[n] = 0;
    drawString(DDR, x, y, buf, fg, bg, scale);
}

#define PLOT_X 20
#define PLOT_Y 70
#define PLOT_W 440
#define PLOT_H 260
#define NBINS  128

static void renderStaticUI(int *DDR, color bg, color fg, color accent) {
    clearScreen(DDR);

    drawFilledRect(DDR, 0, 0, X_MAX, Y_MAX, bg);
    drawFilledRect(DDR, 0, 0, X_MAX, 50, accent);
    drawString(DDR, 16, 12, "ZHAURIS", fg, accent, 2);

    drawRectOutline(DDR, PLOT_X - 2, PLOT_Y - 2, PLOT_W + 4, PLOT_H + 4, 2, fg);

    int sx = 480;
    int sy = 70;
    int sw = 140;
    int sh = 260;

    drawRectOutline(DDR, sx - 2, sy - 2, sw + 4, sh + 4, 2, fg);
    drawString(DDR, sx,      sy - 28, "STATUS", fg, bg, 2);
    drawString(DDR, sx + 10, sy + 20, "CAV",    fg, bg, 2);
    drawString(DDR, sx + 10, sy + 95, "DIR",    fg, bg, 2);
    drawString(DDR, sx + 10, sy + 170, "ENERGY", fg, bg, 2);

    drawRectOutline(DDR, 18, 360, 604, 100, 2, fg);
    drawString(DDR, 30, 372, "UART PKTS:", fg, bg, 2);
    drawString(DDR, 30, 402, "LAST SUM:",  fg, bg, 2);
}

static void renderSpectrum(int *DDR, const uint8_t bins[NBINS], color bg, color bar) {
    drawFilledRect(DDR, PLOT_X, PLOT_Y, PLOT_W, PLOT_H, bg);

    color grid = { .R = 0x2, .G = 0x2, .B = 0x2 };
    for (int k = 0; k <= 4; k++) {
        int y = PLOT_Y + (PLOT_H * k) / 4;
        drawFilledRect(DDR, PLOT_X, y, PLOT_W, 1, grid);
    }

    int barW = PLOT_W / NBINS;
    if (barW < 1) {
        barW = 1;
    }

    for (int i = 0; i < NBINS; i++) {
        int h = (bins[i] * (PLOT_H - 10)) / 255;
        int x = PLOT_X + i * barW;
        int y = PLOT_Y + (PLOT_H - 1) - h;
        drawFilledRect(DDR, x, y, barW, h, bar);
    }
}

static void renderStatus(int *DDR, int cav_on, int dir, uint32_t energy, color bg, color fg) {
    int sx = 480;
    int sy = 70;

    color cav_on_struct  = { .R = 0xF, .G = 0x0, .B = 0x0 };
    color cav_off_struct = { .R = 0x0, .G = 0xA, .B = 0x0 };

    drawFilledRect(DDR, sx + 70, sy + 16, 50, 30, cav_on ? cav_on_struct : cav_off_struct);
    drawRectOutline(DDR, sx + 70, sy + 16, 50, 30, 2, fg);

    color hi = { .R = 0x0, .G = 0xA, .B = 0xF };
    color lo = { .R = 0x2, .G = 0x2, .B = 0x2 };

    int bx = sx + 10;
    int by = sy + 120;
    int bw = 35;
    int bh = 30;
    int gap = 10;

    drawFilledRect(DDR, bx + 0 * (bw + gap), by, bw, bh, (dir == 0) ? hi : lo);
    drawFilledRect(DDR, bx + 1 * (bw + gap), by, bw, bh, (dir == 1) ? hi : lo);
    drawFilledRect(DDR, bx + 2 * (bw + gap), by, bw, bh, (dir == 2) ? hi : lo);

    drawRectOutline(DDR, bx + 0 * (bw + gap), by, bw, bh, 2, fg);
    drawRectOutline(DDR, bx + 1 * (bw + gap), by, bw, bh, 2, fg);
    drawRectOutline(DDR, bx + 2 * (bw + gap), by, bw, bh, 2, fg);

    drawString(DDR, bx + 12,                    by + 6, "L", fg, (dir == 0) ? hi : lo, 2);
    drawString(DDR, bx + 12 + (bw + gap),       by + 6, "C", fg, (dir == 1) ? hi : lo, 2);
    drawString(DDR, bx + 12 + 2 * (bw + gap),   by + 6, "R", fg, (dir == 2) ? hi : lo, 2);

    int ex = sx + 10;
    int ey = sy + 200;
    drawFilledRect(DDR, ex, ey, 120, 50, bg);
    drawUInt(DDR, ex, ey, energy, fg, bg, 2);
}

typedef struct {
    uint8_t  rx[NBYTES];
    uint16_t rx_i;
    uint8_t  receiving;

    uint8_t  tx[NBYTES + 2];
    uint16_t tx_len;
    uint16_t tx_i;
    uint8_t  tx_active;

    uint32_t pkts;
    uint16_t last_sum;

    uint32_t ready_tick;
} uart_demo_t;

static inline void uart_try_send_ready(uart_demo_t *u) {
    if (u->receiving || u->tx_active) {
        return;
    }

    u->ready_tick++;
    if (u->ready_tick < 50000) {
        return;
    }
    u->ready_tick = 0;

    if (!XUartLite_IsTransmitFull(UART_BASEADDR)) {
        XUartLite_SendByte(UART_BASEADDR, 0x52); // 'R'
        u->receiving = 1;
        u->rx_i = 0;
    }
}

static inline void uart_service_rx(uart_demo_t *u, int *got_packet) {
    *got_packet = 0;
    if (!u->receiving) {
        return;
    }

    while (u->rx_i < NBYTES && !XUartLite_IsReceiveEmpty(UART_BASEADDR)) {
        u->rx[u->rx_i++] = XUartLite_RecvByte(UART_BASEADDR);
    }

    if (u->rx_i == NBYTES) {
        uint16_t sum = 0;
        for (int i = 0; i < NBYTES; i++) {
            sum = (uint16_t)(sum + u->rx[i]);
        }

        u->last_sum = sum;
        u->pkts++;

        memcpy(u->tx, u->rx, NBYTES);
        u->tx[NBYTES + 0] = (uint8_t)(sum & 0xFF);
        u->tx[NBYTES + 1] = (uint8_t)((sum >> 8) & 0xFF);
        u->tx_len = NBYTES + 2;
        u->tx_i = 0;
        u->tx_active = 1;

        u->receiving = 0;
        *got_packet = 1;
    }
}

static inline void uart_service_tx(uart_demo_t *u) {
    if (!u->tx_active) {
        return;
    }

    while (u->tx_i < u->tx_len && !XUartLite_IsTransmitFull(UART_BASEADDR)) {
        XUartLite_SendByte(UART_BASEADDR, u->tx[u->tx_i++]);
    }

    if (u->tx_i >= u->tx_len) {
        u->tx_active = 0;
    }
}

int main(void) {
    init_platform();

    int *DDR_addr = (int *)0x80000000;
    volatile int *TFT_addr = (int *)0x44a00000;

    color bg     = { .R = 0x0, .G = 0x0, .B = 0x0 };
    color fg     = { .R = 0x0, .G = 0xF, .B = 0xF };
    color accent = { .R = 0x0, .G = 0x4, .B = 0x8 };
    color bar    = { .R = 0xD, .G = 0x4, .B = 0x9 };

    disableVGA(TFT_addr);
    setVideoMemAddr(TFT_addr, DDR_addr);
    enableVGA(TFT_addr);

    renderStaticUI(DDR_addr, bg, fg, accent);

    uart_demo_t U;
    memset(&U, 0, sizeof(U));

    uint8_t bins[NBINS] = {0};
    for (int i = 0; i < NBINS; i++) {
        bins[i] = (uint8_t)(i * 2);
    }

    renderSpectrum(DDR_addr, bins, bg, bar);
    renderStatus(DDR_addr, 0, 1, 0, bg, fg);

    while (1) {
        uart_try_send_ready(&U);

        int got_packet = 0;
        uart_service_rx(&U, &got_packet);
        uart_service_tx(&U);

        if (got_packet) {
            memcpy(bins, &U.rx[0], NBINS);

            int cav_on = (U.rx[128] != 0);
            int dir    = (int)(U.rx[129] % 3);

            uint32_t energy =
                ((uint32_t)U.rx[130]) |
                ((uint32_t)U.rx[131] << 8) |
                ((uint32_t)U.rx[132] << 16) |
                ((uint32_t)U.rx[133] << 24);

            renderSpectrum(DDR_addr, bins, bg, bar);
            renderStatus(DDR_addr, cav_on, dir, energy, bg, fg);

            drawFilledRect(DDR_addr, 180, 372, 180, 20, bg);
            drawUInt(DDR_addr, 180, 372, U.pkts, fg, bg, 2);

            drawFilledRect(DDR_addr, 180, 402, 180, 20, bg);
            drawUInt(DDR_addr, 180, 402, U.last_sum, fg, bg, 2);
        }
    }

    cleanup_platform();
    return 0;
}
