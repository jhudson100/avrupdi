
#include "SerialPort.h"
#include <memory>
#include <string>
#include <stdexcept>
#include <vector>
#include <cstdint>
#include <iostream>
#include "conf.h"
#include <map>

void SerialPort::write(const std::string& s){
    this->write(s.data(),s.length());
}


std::vector<char> SerialPort::read(std::size_t count){
    std::vector<char> lst(count);
    auto num = this->read(lst.data(),lst.size());
    lst.resize(num);
    return lst;
}
 
//SerialPort::getAllPorts() is implemented in the backends

    
std::string SerialPort::scanForPort(){
    std::vector<PortInfo> ports = SerialPort::getAllPorts();
    unsigned best=0;
    PortInfo bestp;
    for(auto port : ports ){
        unsigned c=0;
        // if( port.vendor.find("Microchip Technology" ) != std::string::npos)
            // c++;
        if( port.hwid == "04d8:00dd" )
            c++;
        // if( port.interface == "usb" )
            // c++;
        if( c > best ){
            bestp=port;
        }
    }
    if(best>0){
        return bestp.device;
    }
    throw std::runtime_error("Cannot find serial device");
}
