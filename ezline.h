#ifndef EZLINE_H_
#define EZLINE_H_

#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

typedef struct {
  char buf[4096];
  int offset;
} Ezline_State;

char *ezline(const char *prompt);

#endif // EZLINE_H_

#ifdef EZLINE_IMPLEMENTATION

#define IS_CTRL(k) ((k) & 0x1f)
#define slwrite(msg, len) write(STDOUT_FILENO, msg, len)
#define swrite(msg) write(STDOUT_FILENO, msg, strlen(msg))

static Ezline_State ezline_state_glob;

int utf8_byte_len(char c) {
  if((c & 0b11111000) == 0b11110000) return 4;
  else if((c & 0b11110000) == 0b11100000) return 3;
  else if((c & 0b11100000) == 0b11000000) return 2;
  else if((c & 0b11000000) == 0b10000000) return -1;
  return 1;
}

/*
  TODO:
    - copy pasting
    - handle ctrl+c (done)
    - arrow keys support
    - utf8 validation
    - handle WINCH signal
    - add more comments/clean up code
*/
char *ezline(const char *prompt) {
  Ezline_State *ez_state = &ezline_state_glob;

  struct termios old_termios;
  if(tcgetattr(STDIN_FILENO, &old_termios) == -1) return NULL;
  
  struct termios raw = old_termios;
  raw.c_cflag |= (CS8);
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_oflag &= ~(OPOST);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) return NULL;
  reprompt:
  memset(ez_state->buf, 0, sizeof(ez_state->buf));
  ez_state->offset = 0;
  swrite(prompt);

  repl: for(;;) {
    struct pollfd ufds[1];
    ufds[0].fd = STDIN_FILENO;
    ufds[0].events = POLLIN;
    if(poll(ufds, 1, -1) == -1) return NULL;

    char buf[4];
    if(read(0, &buf, 4) == -1) return NULL;
    switch(buf[0]) {
      case('\r'): break repl;
      case(IS_CTRL('c')):
        slwrite("^C\n\r", 4);
        goto reprompt;
    }
    int len = sizeof(char) * utf8_byte_len(buf[0]);
    memcpy(ez_state->buf + ez_state->offset, buf, len);
    slwrite(ez_state->buf + ez_state->offset, len);
    ez_state->offset += len;
  }

  if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_termios) == -1) return NULL;
  slwrite("\n", 1);
  return ez_state->buf;
}

#endif // EZLINE_IMPLEMENTATION
