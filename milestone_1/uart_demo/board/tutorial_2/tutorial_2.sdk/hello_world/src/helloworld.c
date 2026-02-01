#include "platform.h"
#include "xparameters.h"
#include "xil_types.h"
#include "xuartlite_l.h"

#define UART_BASEADDR XPAR_AXI_UARTLITE_0_BASEADDR
#define NBYTES 256

static inline u8 uart_getc_blocking(void) {
    return XUartLite_RecvByte(UART_BASEADDR);
}
static inline void uart_putc(u8 c) {
    XUartLite_SendByte(UART_BASEADDR, c);
}

int main() {
    init_platform();

    static u8  buf[NBYTES];
    while (1) {
        // handshake
        uart_putc(0x52); // 'R'

        // receive payload
        for (int i = 0; i < NBYTES; i++) {
            buf[i] = uart_getc_blocking();
        }

        // echo payload back
        for (int i = 0; i < NBYTES; i++) {
            uart_putc(buf[i]);
        }

        // send checksum (sum mod 65536), LSB then MSB
        u16 sum = 0;
        for (int i = 0; i < NBYTES; i++) sum = (u16)(sum + buf[i]);
        uart_putc((u8)(sum & 0xFF));
        uart_putc((u8)((sum >> 8) & 0xFF));
    }

    cleanup_platform();
    return 0;
}
