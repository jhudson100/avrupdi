
#include "conf.h"

#if PLATFORM == PLATFORM_LINUX

#include <cstdio>
#include <string>
#include <stdexcept>
#include <vector>
#include <poll.h>
#include <unistd.h>
#include <termios.h>
#include "LinuxPort.h"
#include <filesystem>
#include <iostream>
#include <fstream>

void linuxSetBaudRate(int fd, unsigned baudrate);

//ref: https://www.downtowndougbrown.com/2013/11/linux-custom-serial-baud-rates/



LinuxPort::LinuxPort(unsigned timeout, unsigned baudrate, std::string devname) 
{
    this->timeout=timeout;
    fp = fopen(devname.c_str(),"r+b");
    if(!fp)
        throw std::runtime_error("Could not open "+devname);
    fd = fileno(fp);
    try{
        linuxSetBaudRate(fd,baudrate);
    } catch(...){
        fclose(fp);
        throw;
    }
}

LinuxPort::~LinuxPort(){
    if(fp){
        fclose(fp);
    }
}

void LinuxPort::discardInput(){
    tcflush(this->fd,TCIOFLUSH);
}

void LinuxPort::write(const void* v, std::size_t size) {
    const char* p = (const char*) v;
    
    while(size>0){
        pollfd P;
        P.fd = this->fd;
        P.events = POLLOUT ;
        auto rv = poll( &P,1,this->timeout);
        if( rv == 0 ){
            throw std::runtime_error("Timeout on write");
        }
        if( P.revents & (POLLERR|POLLHUP|POLLNVAL|POLLPRI) ){
            throw std::runtime_error("I/O error on write");
        }

        auto nw = fwrite(p,1,size,fp);
        if( nw == 0 )
            throw std::runtime_error("I/O error on write");
        size -= nw;
        p += nw;
    }
    fflush(fp);
}

std::size_t LinuxPort::read(void* v, std::size_t count) {
    char* p = (char*) v;
    std::size_t totalr=0;

    while(count > 0 ){
        pollfd P;
        P.fd = this->fd;
        P.events = POLLIN ;
        auto rv = poll( &P,1,this->timeout);
        if( rv == 0 ){
            throw std::runtime_error("Timeout on read");
        }
        if( P.revents & (POLLERR|POLLHUP|POLLNVAL|POLLPRI) ){
            throw std::runtime_error("I/O error on read");
        }
        //if we get here, fp must be ready for reading
        auto nr = fread(p,1,count,fp);
        if(nr == 0 ){
            if( ferror(fp)){
                throw std::runtime_error("I/O error on read");
            } else {
                return totalr;
            }
        }
        p += nr;
        count -= nr;
        totalr += nr;
    }
    return totalr;
}


std::vector<PortInfo> SerialPort::getAllPorts(){
    using namespace std::filesystem;
    using std::string;
    std::vector<PortInfo> pinfo;

    path sys("/sys/class/tty");
    directory_iterator iter(sys);
    for(auto& dentry : iter ){
        string s = dentry.path().string();
        if( s == "." || s == ".." ){
        } else if( dentry.is_directory() ) {
            path p2 = dentry.path() / "device" / "driver" ;
            if( std::filesystem::exists(p2)){
                std::ifstream in(dentry / path("uevent") );
                if(in.good()){
                    while(1){
                        std::string s;
                        getline(in,s);
                        if( in.fail())
                            break;
                        if( s.find("DEVNAME=") == 0){
                            std::string devfile=s.substr(8);
                            FILE* fp = fopen( ("/dev/"+devfile).c_str(),"r+b");
                            if(fp){
                                int fd = fileno(fp);
                                termios t;
                                if( 0 == tcgetattr(fd,&t) ){
                                    path p3 = dentry.path() / "device";
                                    p3 = canonical(p3);
                                    p3 = canonical( p3 / "..");
                                    std::string vendor,product;
                                    
                                    {
                                        std::ifstream in( p3 / "idProduct");
                                        if(in.good()){
                                            std::getline(in,product);
                                        }
                                    }
                                        {
                                        std::ifstream in( p3 / "idVendor");
                                        if(in.good()){
                                            std::getline(in,vendor);
                                        }
                                    }

                                    PortInfo pi;
                                    pi.device = devfile;
                                    if( !vendor.empty() && !product.empty() )
                                        pi.hwid = vendor+":"+product;
                                    pinfo.push_back(pi);
                                }
                                fclose(fp);
                            }
                        }
                    }
                }
            }
        }
    }
    return pinfo;
}
 


#endif // PLATFORM_LINUX