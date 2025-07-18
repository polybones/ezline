/*
  ezline.h

  LICENSE:
  
    zlib/libpng license

    Copyright (c) 2025 polybones
    
    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the
    use of this software.
    
    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:
    
        1. The origin of this software must not be misrepresented; you must not
        claim that you wrote the original software. If you use this software in a
        product, an acknowledgment in the product documentation would be
        appreciated but is not required.
    
        2. Altered source versions must be plainly marked as such, and must not
        be misrepresented as being the original software.
    
        3. This notice may not be removed or altered from any source
        distribution.
*/

#ifndef EZLINE_H_
#define EZLINE_H_

#include <assert.h>
#include <math.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#ifndef EZLINE_MAX_BUF_LEN
#define EZLINE_MAX_BUF_LEN 4096
#endif // EZLINE_MAX_BUF_LEN

#define EZLINE_CHUNK_SIZE 256

typedef enum {
  EZLINE_STAT_OK = 0,
  EZLINE_STAT_ABORT,
  EZLINE_STAT_CANCEL,
  EZLINE_STAT_NO_INPUT,
} Ezline_Status;

typedef struct {
  char buf[EZLINE_MAX_BUF_LEN];
  int offset;
  Ezline_Status status;
} Ezline_State;

char *ezline(const char *prompt);
const Ezline_Status ezline_stat();

#endif // EZLINE_H_

#ifdef EZLINE_IMPLEMENTATION

#define EZ_CTRL(k) ((k) & 0x1f)
#define slwrite(msg, len) write(STDOUT_FILENO, msg, len)
#define swrite(msg) write(STDOUT_FILENO, msg, strlen(msg))

static Ezline_State ezline_state_glob;

int utf8_byte_len(char c) {
  if((c & 0b11111000) == 0b11110000) return 4;
  else if((c & 0b11110000) == 0b11100000) return 3;
  else if((c & 0b11100000) == 0b11000000) return 2;
  else if((c & 0b11000000) == 0b10000000) return -1;
  else return 1;
}

int is_utf8_continuation(unsigned char c) {
  return (c & 0b11000000) == 0b10000000;
}

// TODO: add support for shifting up/down in rows if the cursor
// exceeds the size of the terminal columns.
// TODO(2): Implement a "builder" system for concatinating
// actions (like clearing, moving cursor, etc.)
// TODO(3): Do bounds checking for rows/cols.
// https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797
void ez_cur_move(int amount) {
  Ezline_State *ez_state = &ezline_state_glob;
  assert(amount != 0);
  int shift = abs(amount);
  assert(shift <= 256);

  int size = floor(((sizeof(int) * shift) / 10)) + 5;
  char buf[256];
  if(amount > 0) {
    // go right
  }
  else {
    snprintf(buf, 5, "\x1B[%dD", shift);
    ez_state->offset -= shift;
  }
  slwrite(buf, 5);
}

char *ezline(const char *prompt) {
  Ezline_State *ez_state = &ezline_state_glob;

  struct termios old_termios;
  if(tcgetattr(STDIN_FILENO, &old_termios) == -1) {
    ez_state->status = EZLINE_STAT_ABORT;
    goto done;
  }
  
  struct termios raw = old_termios;
  raw.c_cflag |= (CS8);
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_oflag &= ~(OPOST);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
    ez_state->status = EZLINE_STAT_CANCEL;
    goto done;
  }
  reprompt:
  memset(ez_state->buf, 0, sizeof(ez_state->buf));
  ez_state->offset = 0;
  ez_state->status = EZLINE_STAT_OK;
  swrite(prompt);

  unsigned char read_buf[EZLINE_CHUNK_SIZE];
  int read_pos = 0;
  int read_len = 0;

  repl: for(;;) {
    if(read_pos >= read_len) {
      scan:
      struct pollfd ufds[1];
      ufds[0].fd = STDIN_FILENO;
      ufds[0].events = POLLIN;
      if(poll(ufds, 1, -1) == -1) return NULL;

      if((read_len = read(STDIN_FILENO, read_buf, sizeof(read_buf))) == -1) {
        ez_state->status = EZLINE_STAT_ABORT;
        goto done;
      }
      if(read_len == 0) continue;
      read_pos = 0;
    }

    while(read_pos < read_len) {
      unsigned char c = read_buf[read_pos];

      switch(c) {
        case(127):
          // slwrite("\x1B[5D", 4);
          ez_cur_move(-1);
          goto scan;
          // break;
        case('\r'):
          if(ez_state->offset == 0) {
            ez_state->status = EZLINE_STAT_NO_INPUT;
          }
          goto done;
        case(EZ_CTRL('c')):
          slwrite("^C", 2);
          ez_state->status = EZLINE_STAT_CANCEL;
          goto done;
      }

      int expected_len = utf8_byte_len(c);
      if(expected_len == -1) {
        read_pos++;
        continue;
      }

      if(read_pos + expected_len > read_len) {
        // if we have a utf8 character that exceeds the size of our buffer,
        // we extend for more room.
        memmove(read_buf, read_buf + read_pos, read_len - read_pos);
        read_len -= read_pos;
        read_pos = 0;
        
        int additional = read(STDIN_FILENO, read_buf + read_len, sizeof(read_buf) - read_len);
        if(additional > 0) {
          read_len += additional;
        }
        
        if(read_len < expected_len) {
          read_pos = read_len;
          continue;
        }
      }

      if(ez_state->offset + expected_len >= sizeof(ez_state->buf)) {
        read_pos += expected_len;
        continue;
      }
      
      memcpy(ez_state->buf + ez_state->offset, read_buf + read_pos, expected_len);
      slwrite(ez_state->buf + ez_state->offset, expected_len);
      
      ez_state->offset += expected_len;
      read_pos += expected_len;
    }
  }

  done:
  if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_termios) == -1) {
    ez_state->status = EZLINE_STAT_ABORT;
  }
  slwrite("\n", 1);
  return ez_state->buf;
}

const Ezline_Status ezline_stat() {
  return ezline_state_glob.status;
}

#endif // EZLINE_IMPLEMENTATION
