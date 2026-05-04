#ifdef __linux__
#pragma once

#include <termios.h>

int myTcgetattr(int fd, struct termios *termios_p);
int myTcsetattr(int fd, int optional_actions, const struct termios *termios_p);
speed_t myCfgetispeed(const struct termios *termios_p);
speed_t myCfgetospeed(const struct termios *termios_p);
int myCfsetispeed(struct termios *termios_p, speed_t speed);
int myCfsetospeed(struct termios *termios_p, speed_t speed);

#endif