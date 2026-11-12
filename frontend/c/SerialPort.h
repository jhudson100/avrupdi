#pragma once

#include <string>
#include <vector>
#include <cstdint>

class PortInfo{
    public:
    std::string device;       //device name
    std::string hwid;
};

class SerialPort{

    public:

    virtual void discardInput()=0;

    std::vector<char> read(std::size_t count);
    virtual std::size_t read(void* p, std::size_t count) = 0;

    void write(const std::string& s);
    template<typename T>
    void write(const std::vector<T>&v);
    virtual void write(const void* p, std::size_t size)=0;

    static std::vector<PortInfo> getAllPorts();
    static std::string scanForPort();
};

