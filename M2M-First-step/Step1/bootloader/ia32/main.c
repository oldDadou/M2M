/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Author: Olivier Gruber (olivier dot gruber at acm dot org)
*/
#include "constante.h"

typedef struct s_hisory {
  int elements;
  int cursor;
  char history[ HISTORY_SIZE * LINE_SIZE ];
} history;

static __inline __attribute__((always_inline, no_instrument_function))
uint8_t inb(uint16_t port) {
  uint8_t data;
  __asm __volatile("inb %w1,%0" : "=a" (data) : "d" (port));
  return data;
}

static __inline __attribute__((always_inline, no_instrument_function))
void outb(uint16_t port, uint8_t data) {
  __asm __volatile("outb %0,%w1" : : "a" (data), "d" (port));
}


static void serial_init(uint16_t port) {

  outb(port+1,0x00);    // Disable all interrupts
  outb(port+3,0x80);    // Enable DLAB (set baud rate divisor)

  outb(port+0,0x01);  // Set divisor to 1 (lo byte) 115200 baud
  outb(port+1,0x00);  //                0 (hi byte)

  outb(port+3,0x03);    // 8 bits, no parity, one stop bit
  outb(port+2,/*0xC7*/ 0x00);    // Enable FIFO, clear them, with 14-byte threshold
  outb(port+4,/*0x0B*/ 0x08);    // IRQs enabled, RTS/DSR set

  // outb(port+1,0x0d); // enable all intrs but writes
}

static
void serial_write_char(uint16_t port, char c) {
  while ((inb(port + 5) & 0x20) == 0);
  outb(port,c);
}

static
void serial_write_string(uint16_t port, const unsigned char *s) {
  while(*s != '\0') {
    serial_write_char(port,*s);
    s++;
  }
}

char serial_read(uint16_t port) {
   while ((inb(port + 5) & 1) == 0);
   return inb(port);
}

/*
 * See:
 *   http://wiki.osdev.org/Printing_To_Screen
 *   http://wiki.osdev.org/Text_UI
 *
 * Note this example will always write to the top
 * line of the screen.
 * Note it assumes a color screen 0xB8000.
 * It also assumes the screen is in text mode,
 * and page-0 is displayed.
 * Screen is 80x25 (80 characters, 25 lines).
 *
 * For more info:
 *   http://wiki.osdev.org/VGA_Resources
 */
void write_string( int colour, const char *string ) {
    volatile char *video = (volatile char*)0xB8000;
    while( *string != 0 ) {
        *video++ = *string++;
        *video++ = colour;
    }
}

void kputchar(int c, void *arg) {
  serial_write_char(COM2,c);
}

void shift_screen(volatile char* screen) {
  for(int i = 0; i < (80*25*2) - LINE_SIZE ; i++) {
      screen[i] = screen[i + LINE_SIZE];
  }
}

void history_get_line(int n_line, volatile char *out_line, const history *hist) {
  n_line = hist->cursor + n_line >= hist->elements ? (n_line + hist->cursor) - hist->elements : hist->cursor + n_line;
  int start = n_line * LINE_SIZE;
  for(int i  = 0; i < LINE_SIZE; i++) {
    out_line[i] =  hist->history[start + i];
  }
}

void save_line(volatile char *line, history *hist) {
  int start = hist->cursor * LINE_SIZE;
  for(int i = 0; i < LINE_SIZE; i++) {
    hist->history[start + i] = line[i];
  }
  if(hist->elements < HISTORY_SIZE) hist->elements++;

  hist->cursor++;
  if(hist->cursor == hist->elements) hist->cursor = 0;
}

void kmain(void) {
  int cursor = 0;
  volatile char *screen = (volatile char*)0xB8000;
  history hist;
  hist.cursor = 0;
  hist.elements = 0;

  for(int i = 0 ; i < LINE_SIZE * HISTORY_SIZE;i++) {
        hist.history[i] = 'c';
  }

  for (int i = 0 ; i < 80*25*2; i++) {
    screen[i++] = ' ';
    screen[i] = 0x2a;
  }

  serial_init(COM2);
  serial_write_string(COM2,"\n\rHello!\n\r\nThis is a simple echo console... please type something.\n\r");
  kprintf("hello !!!! ");
  serial_write_char(COM2,'>');
  int current_history = 0;
  while(1) {
    unsigned char c;
    c=serial_read(COM2);
      if (c==13) {
        save_line(&(screen[LINE_SIZE * (cursor / LINE_SIZE)]), &hist);
        if(cursor / LINE_SIZE >= 24 ) {
          shift_screen(screen);
          int pos = cursor % LINE_SIZE;
          cursor = cursor - pos;
        } else {
          int pos = cursor % LINE_SIZE;
          cursor += LINE_SIZE - pos;
        }
        current_history = 0;
      } else if(c==127) {
        if(cursor % LINE_SIZE != 0) {
          cursor -= 2;
          screen[cursor] = ' ';
        }
        current_history = 0;
      } else if (c == '\033') {
        serial_read(COM2);
        char arrow = serial_read(COM2);
        if(arrow == 'A') {
          history_get_line(current_history, &(screen[LINE_SIZE * (cursor / LINE_SIZE)]), &hist);
          if(current_history + 1 < hist.elements) current_history++;
        } else if(arrow == 'B') {
          if(current_history - 1 < 0) current_history--;
          history_get_line(current_history, &(screen[LINE_SIZE * (cursor / LINE_SIZE)]), &hist);
        }
      } else {
        screen[cursor] = c;
        cursor+=2;
        current_history = 0;
      }
  }
}
