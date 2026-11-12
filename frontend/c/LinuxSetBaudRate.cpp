
#include "conf.h"

#if PLATFORM == PLATFORM_LINUX


#include <asm/termbits.h>
#include <sys/ioctl.h>
#include <stdexcept>

void linuxSetBaudRate(int fd, unsigned baudrate)
{

    termios2 io;
    ioctl(fd,TCGETS2,&io);
    io.c_cflag &= ~CBAUD;
    io.c_cflag |= BOTHER;
    io.c_ispeed = io.c_ospeed = baudrate;
    if( ioctl(fd, TCSETS2, &io) ){
        throw std::runtime_error("Cannot set baud rate");
    }
}

#endif