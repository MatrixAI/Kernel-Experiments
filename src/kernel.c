#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum vga_color {
  VGA_COLOR_BLACK = 0,
  VGA_COLOR_BLUE = 1,
  VGA_COLOR_GREEN = 2,
  VGA_COLOR_CYAN = 3,
  VGA_COLOR_RED = 4,
  VGA_COLOR_MAGENTA = 5,
  VGA_COLOR_BROWN = 6,
  VGA_COLOR_LIGHT_GREY = 7,
  VGA_COLOR_DARK_GREY = 8,
  VGA_COLOR_LIGHT_BLUE = 9,
  VGA_COLOR_LIGHT_GREEN = 10,
  VGA_COLOR_LIGHT_CYAN = 11,
  VGA_COLOR_LIGHT_RED = 12,
  VGA_COLOR_LIGHT_MAGENTA = 13,
  VGA_COLOR_LIGHT_BROWN = 14,
  VGA_COLOR_WHITE = 15,
};

static inline uint8_t vga_entry_color (enum vga_color fg, enum vga_color bg) {
  return fg | (bg << 4);
}

static inline uint16_t vga_entry (unsigned char uc, uint8_t color) {
  return (uint16_t) uc | ((uint16_t) color << 8);
}

size_t strlen (const char * str) {
  size_t len = 0;
  while (str[len]) {
    len++;
  }
  return len;
}

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

// declaration of variables outside of functions, and note defined to be static
// global mutable variables apparently
size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t * terminal_buffer;

void terminal_initialize (void) {
  terminal_row = 0;
  terminal_column = 0;
  terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
  // in 32  bit protected mode, the 0xB8000 is the VGA location
  // for monochrome monitors, it would be at 0xB0000
  // text mode memory takes 2 bytes for every character on screen
  // one is the ascii code byte, the other is the attribute byte
  // the attribute byte is the colour of the character
  // so if we use `uint16_t` this provides enough space for both the code point and the colour byte
  // the fg colour is in the lowest 4 bits, while the background colour is in the highest 3 bits
  // on x86 this turns out to be low to means the right side, while the high bits is on the left side
  // so this monitor has a defined size right? not sure
  // but the idea is that this memory location is where you need to start writing
  // if you want to print to text
  // using VGA_HEIGHT and VGA_WIDTH, we can then set the actual locations we care about
  // so I guess if you write out of the limits of the terminal buffer, you will have a problem!
  // but our VGA_HEIGHT and VGA_WIDTH is really small!
  // this 0xB8000 is essentially a magic number representing a location to write to
  // it's also a meta coupling, not very functional!
  // better would be if the external system passed this into our main function to know where these things are
  terminal_buffer = (uint16_t *) 0xB8000; // where does this point to?
  for (size_t y = 0; y < VGA_HEIGHT; y++) {
    for (size_t x = 0; x < VGA_WIDTH; x++) {
      const size_t index = (y * VGA_WIDTH) + x;
      // writes an empty character with a terminal color
      terminal_buffer[index] = vga_entry(' ', terminal_color);
    }
  }
}

void terminal_setcolor (uint8_t color) {
  terminal_color = color;
}

void terminal_putentryat (char c, uint8_t color, size_t x, size_t y) {
  const size_t index = y * VGA_WIDTH + x;
  terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar (char c) {
  terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
  if (++terminal_column == VGA_WIDTH) {
    terminal_column = 0;
    if (++terminal_row == VGA_HEIGHT) {
      terminal_row = 0;
    }
  }
}

void terminal_write (const char * data, size_t size) {
  for (size_t i = 0; i < size; i++) {
    terminal_putchar(data[i]);
  }
}

void kernel_main (void) {
  terminal_initialize();
  const char str[] = "Hello, kernel World!\n";
  const size_t strsize = sizeof(str);
  terminal_write(str, strsize);
}
