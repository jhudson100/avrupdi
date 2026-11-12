#include "SerialPort.h"
#include <iostream>

int main(int argc, char* argv[])
{
    for(auto p : SerialPort::getAllPorts() ){
        std::cout << p.device << " " << p.hwid << "\n";
    }
    return 0;
}