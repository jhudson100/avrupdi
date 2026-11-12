#pragma once

#include "conf.h"
#if PLATFORM == PLATFORM_LINUX

#include "SerialPort.h"

class LinuxPort : public SerialPort{
  public:
    FILE* fp;
    int fd;
    unsigned timeout;

    LinuxPort(unsigned timeout, unsigned baudrate, std::string devname) ;
    ~LinuxPort();
    virtual void discardInput();
    void write(const void* v, std::size_t size) override;
    std::size_t read(void* v, std::size_t count) override;
  
};

#endif // PLATFORM_LINUX