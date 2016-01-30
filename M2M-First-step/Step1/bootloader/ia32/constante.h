#ifndef __CONSTANTE_H_
#define __CONSTANTE_H_

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

#define COM1 ((uint16_t)0x3f8)
#define COM2 ((uint16_t)0x2f8)
#define UART ((uint16_t)0xB8000)

#define HISTORY_SIZE 2
#define LINE_SIZE (80*2)

#define SCREEN_SIZE  80*25*2

#endif
