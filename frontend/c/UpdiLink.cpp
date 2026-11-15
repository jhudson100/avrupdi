
#include <cstdint>
#include "log.h"
#include <thread>
#include <chrono>

static const unsigned VERSION=0x2658;


static const unsigned SIZE_BYTE = 0b00;
static const unsigned SIZE_WORD = 0b01;
static const unsigned SIZE_LONG = 0b10;        //three bytes

static const unsigned PTR_DEREF = 0b00;
static const unsigned PTR_DEREF_POSTINC = 0b01;
static const unsigned PTR_ITSELF = 0b10;

static const unsigned KEY_SIZE_64_BITS = 0b00;
static const unsigned KEY_SIZE_128_BITS = 0b01;

static const unsigned SIB_OP_SEND_KEY = 0b0;
static const unsigned SIB_OP_RECV_SIB = 0b1;

//write these lsb first (i.e., right to left)
static const char* KEY_CHIP_ERASE    =      "NVMErase" ;
static const char* KEY_CHIP_NVM_PROG =      "NVMProg " ;
static const char* KEY_CHIP_USERROW_WRITE = "NVMUS&te" ;

static const unsigned GUARD_TIME_128_CYCLES = 0x0;
static const unsigned GUARD_TIME_32_CYCLES = 0x2;
static const unsigned GUARD_TIME_16_CYCLES = 0x3;
static const unsigned GUARD_TIME_8_CYCLES = 0x4;
static const unsigned GUARD_TIME_4_CYCLES = 0x5;
static const unsigned GUARD_TIME_2_CYCLES = 0x6;
static const unsigned GUARD_TIME_OFF = 0x7;

  

class UpdiLink{

    bool connected;
    std::shared_ptr<SerialPort> port;
    bool enableProgrammingOnConnect;
    std::shared_ptr<Chip> chip;
    unsigned updiTimeout;

  public:

    UpdiLink(std::shared_ptr<SerialPort> port, std::shared_ptr<Chip> chip, unsigned updiSpeed){
        this->connected=false;
        this->port=port;
        this->enableProgrammingOnConnect=true;
        this->chip = chip;
        this->updiTimeout=2000;
        this->updiSpeed=updiSpeed;
    }

    void setUpdiTimeout( float updiTimeout){
        this->updiTimeout = unsigned( 1000.0 * updiTimeout);
    }

    void disconnect(){
        if(this->connected){
            this->connected=false;
        }
    }

    bool inConnect=false;
    void connectToUPDI() -> None:
        if(this->inConnect)
            return;

        this->inConnect=true;
        RAII r( [this](){ this->inConnect=false; } );
        
        if(this->connected){
            debug("Already connected; not trying to reconnect");
            return;
        }

        bool remoteAlreadyConnected = this->CONNECT_UPDI();
        this->connected=true;

        if(remoteAlreadyConnected){
            debug("BREAK and request reset")
            this->BREAK();
            this->requestReset();
        }

        #synchronize
        this->port.discardInput();
        this->port.write("\n\n\nVERSION\n\n\n");
        this->waitForPrompt();
        std::this_thread::sleep_for(std::chrono::milliseconds(500)));
        this->port.discardInput();

        this->UPDISPEED(this->updiSpeed);

        //we can enable programming even if we don't
        //know which chip model we have yet.
        //We can't read sig row until NVM programming has been
        //enabled
        this->enableNVMProgramming();

        if(!this->chip)
            this->chip = Chip::autodetectChip(this)

        if(!this->chip.verifySignature(this))
            std::exit(1);

        //reduce guard bit time to 2 cycles
        auto v = this->LDCS(0x02);
        v &= ~0b111;
        v |= 6;
        this->STCS({.addr=0x02,.value=v});

        //set updi clock to 16MHz
        //8MHz is supported at all voltage/temperature ranges
        //16MHz is supported at 0-50 degrees C at lower voltages
        //   or at full temperature range at 4.5V
        //32MHz is only supported at 4.5V
        v = this->LDCS(0x09);
        v &= ~0x03;
        v |= 1;      //1=16MHz; 2=8MHz; 3=4MHz
        this->STCS({.addr=0x09, .value=v});
    }
    

    void UPDISPEED(int v){
        this->_sendCommand( std::format("UPDISPEED {}",v) );
    }

    int GET_VERSION(){
        auto r = this->_sendCommand("VERSION");
        return std::get<int>(r["version"]);
    }
    
    def CONNECT_UPDI(self) -> bool:
        debug("CONNECT_UPDI")
        this->_sendCommand(f"TIMEOUT {round(this->updiTimeout*1000)}")
        J = this->_sendCommand("CONNECT_UPDI")
        return J["already_connected"]

    def BREAK(self) -> None:
        this->_sendCommand("BREAK")

    def LDCS(addr) -> int:
        d = this->_sendCommand(f"LDCS {addr}")
        return d["value"]

    def STCS(addr: int,value: int) -> None:
        this->_sendCommand(f"STCS {addr} {value}")

    def STS( addr: int, value: int) -> None:
        st = time.time_ns()
        debug(f"sts: Store at 0x{addr:04x}: 0x{value:02x}")
        this->_sendCommand(f"STS {addr} {value}")
        et = time.time_ns()
        profiling.count("STS",et-st)

    def STS_REPEAT( addr:int, data: bytes) -> None:
        this->_sendCommand(f"REP_STS {addr} {len(data)}",sendExtra=data)

    def STS_M( *AV: tuple[int,int]) -> None:
        st=time.time_ns()
        cmd = f"STS_M {len(AV)}"
        for addr,value in AV:
            cmd += f" {addr} {value}"
        this->_sendCommand(cmd)
        et=time.time_ns()
        profiling.count("STS_M",et-st)

    def LDS( addr: int) -> int:
        st = time.time_ns()
        v = this->_sendCommand(f"LDS {addr}")
        et = time.time_ns()
        profiling.count("LDS",et-st)
        return v["value"]

    def ST(target: int, size: int, value: int) -> None:
        #Note that the size is 0, 1, or 2
        this->_sendCommand(f"ST {target} {size} {value}")

    def ST_NR(target: int, size: int, value: int) -> None:
        #Note that the size is 0, 1, or 2
        this->_sendCommand(f"ST {target} {size} {value}", expectResponse=False)

    def REP_ST( addr: int, data: bytes):
        this->_sendCommand(f"REP_ST {addr} {len(data)}", sendExtra=data)

    def ST_REPEAT(data:bytes) -> None:
        this->_sendCommand(f"ST_REPEAT {len(data)}", sendExtra=data)

    def ST_RV(addr: int, verify: bool, data:bytes) -> None:
        this->_sendCommand(f"ST_RV {addr} {len(data)} {1 if verify else 0}", sendExtra=data)

    def LD_REPEAT(count:int) -> bytes:
        d = this->_sendCommand(f"LD_REPEAT {count}")
        return bytes(d["data"])

    def LDS_REPEAT(addr:int, count:int) -> bytes:
        d = this->_sendCommand(f"REP_LDS {addr} {count}")
        return bytes(d["data"])

    def KEY( key:bytes) -> None:
        tmp = " ".join([ str(q) for q in key])
        this->_sendCommand(f"KEY {tmp}")

    def SIB(self) -> bytes:
        d = this->_sendCommand(f"SIB")
        return d["data"]

    def STS_W( toWrite, toPoll, pollMask):
        cmd = f"STS_W {len(toWrite)}"
        for addr,val in toWrite:
            cmd += f" {addr} {val}"
        cmd += f" {toPoll} {pollMask}"
        this->_sendCommand(cmd)

    std::map<std::string, std::variant<int,float,std::string> > _sendCommand( std::string cmd){
        return _sendCommand( cmd, std::vector<char>() );
    }

    std::map<std::string, std::variant<int,float,std::string> > _sendCommand( std::string cmd, const std::vector<char>& extra){
        if( cmd == "VERSION" || cmd == "CONNECT_UPDI" || cmd.starts_width("UPDISPEED ") || cmd.starts_with("TIMEOUT ")){
            //can issue this command even if UPDI link not established
        } else {
            if(! this->connected ){
                throw std::runtime_error(std::format("UPDI connection not established for {}",cmd));
            }
        }
        
        debug(std::format("sendCommand: {}",cmd));


        this->port.write(cmd);
        this->port.write("\n");
        if(!sendExtra.empty())
            this->port.write(sendExtra);
            
        resp = this->waitForPrompt();
        std::istringstream iss(resp);
        std::string respstr;
        while(true){
            std::string line;
            std::getline(iss,line);
            if( !iss.fail()){
                if( line.starts_with("DBG>") )
                    std::cout << line << "\n";
                else
                    respstr += line
            } else {
                break;
            }
        }
        
        
                try:
                    R = json.loads(toDecode)
                except json.decoder.JSONDecodeError as e:
                    print("RESP: ->"+resp+"<-")
                    raise
                if not R["ok"]:
                    raise CommandException("Error",json=str(R))
                return R #type:ignore
            else:
                this->ignoreCount+=1
                return {}
        finally:
            profiling.count("_sendCommand", time.time_ns()-st)

    def waitForPrompt(self) -> str:
        PROMPT="\n@@>"
        return this->waitForString(PROMPT)[:-len(PROMPT)]

    def waitForString(needle:str) -> str:
        #debug("WAIT FOR STRING:",repr(needle))
        try:
            previousNewline=-1
            haystack=[]
            needleL = list(needle)
            while True:
                c = this->port.read(1).decode(errors="ignore")
                haystack.append(c)
                if c == '\n':
                    tmp = haystack[previousNewline+1:]
                    debug("RECEIVED:","".join(tmp))
                    previousNewline = len(haystack)-1
                if haystack[-(len(needle)):] == needleL:
                    # ~ debug(f"WAIT FOR STRING found {needle} and returns:",haystack)
                    return "".join(haystack)
        except Exception as e:
            # ~ debug(f"waitForString: waiting for {repr(needle)}:  Got exception {e}")
            # ~ debug("Before exception: Received:",haystack)
            raise



    def setClock( speed: int) -> None:
        allowed = {
            4: 3, 8: 2, 16: 1, 32: 0
        }
        if speed not in allowed:
            tmp = sorted(list(allowed.keys()))
            tmp2 = ", ".join([str(q) for q in tmp])
            log.error("Bad UPDI clock speed; must be one of",tmp2)

        xx = this->LDCS(addr=0x09)

        xx &= 0b11111100
        xx |= allowed[speed]
        this->STCS(addr=0x09, value=xx)

    def requestReset(self) -> None:
        debug("requestReset")
        debug("Send 0x59 (do a reset) to UPDI ASI")
        this->STCS(addr=0x08, value=0x59)  #request reset: Set RSTREQ bit
        time.sleep(0.05)  #might not be needed
        debug("Clear reset request")
        this->STCS(addr=0x08, value=0x00)  #clear reset request
        debug("requestReset done")
        while True:
            v = this->LDCS(0x0b)
            if v & (1<<5):
                #in reset
                time.sleep(0.01)
            else:
                break

    def enableErase(self) -> None:
        #enable erase at the UPDI level

        #send both keys. Datasheet says we need to do this
        #to prevent errors from chip trying to execute
        #erased flash
        ks = this->LDCS(0x07)
        if ks & 0b00011000 == 0b00011000:
            #erase and nvmprog are both active
            debug("Erase and nvmprog are already active")
            return

        print("Before keys:",bin(ks))
        this->KEY(KEY_CHIP_ERASE)
        print("CE key:",bin(ks))
        this->KEY(KEY_CHIP_NVM_PROG)
        print("NVM key:",bin(ks))

        #Verify both bits are set
        #bit 3 = chip erase key
        #bit 4 = NVMProg key
        #bit 5 = user row key (not used right now)
        ks = this->LDCS(0x07)
        if ks & (0b00011000) != 0b00011000:
            this->requestReset()
            this->KEY(KEY_CHIP_ERASE)
            print("CE key:",bin(ks))
            this->KEY(KEY_CHIP_NVM_PROG)

        ks = this->LDCS(0x07)
        if ks & (0b00011000) != 0b00011000:
            raise ProtocolError(f"Could not enable chip erase + NVM programming: {ks:08b}")

        print("Before reset: keys=",bin(ks))
        this->requestReset()
        ks = this->LDCS(0x07)
        print("After reset: keys=",bin(ks))

        #wait for lock bits to be cleared.
        #we probably don't need to do this
        #if the chip was not locked?
        #datasheet says updi can't access bus until boot flag is 1,
        #but that flag never becomes 1
        while True:
            status = this->LDCS(0x0b)
            print("status=",bin(status))
            if (status & 1) == 0: # and (status & 0b10) == 1:
                break
            else:
                time.sleep(0.05)
        ks = this->LDCS(0x07)
        debug("Done enabling erase. Keys=",bin(ks),"status=",bin(status))


    def enableNVMProgramming(self) -> None:

        debug("enableNVMProgramming")

        #if bit 4 is 1: NVM programming has been enabled
        ks = this->LDCS(0x07)
        debug("keys=",bin(ks))
        if ks & (1<<4):
            debug("nvmprog is already active")
            return

        #debug("send key: ERASE then NVM_PROG")
        #this->KEY(KEY_CHIP_ERASE)
        #print("AFTER CE:",bin(this->LDCS(0x07)))
        this->KEY(KEY_CHIP_NVM_PROG)
        debug("AFTER NVM:",bin(this->LDCS(0x07)))

        ks = this->LDCS(0x07)
        if not (ks & (1<<4)):
            #if we didn't get nvm prog enabled,
            #reset the chip and then send the key again
            debug("Programming was not enabled")
            debug("Send break")
            this->BREAK()
            debug("reset chip")
            this->requestReset()
            debug("send key: CE then NVMPROG")
            #this->KEY(KEY_CHIP_ERASE)
            #print("AFTER CE:",bin(this->LDCS(0x07)))
            this->KEY(KEY_CHIP_NVM_PROG)
            print("AFTER NVM:",bin(this->LDCS(0x07)))
        else:
            debug("NVM programming has been enabled")

        ks = this->LDCS(0x07)
        if not (ks & (1<<4)):
            raise ProtocolError("Chip will not enter programming mode")

        debug("Chip is in programming mode")

        debug("Resetting chip")
        this->requestReset()

        time.sleep(0.001)

        #read ASI system status register and check PROGSTART bit (bit 3)
        for i in range(10):
            debug("Check PROGSTART bit")
            v = this->LDCS(0x0b)
            bootdone = v & (1<<1)
            progstart = v & (1<<3)
            resetActive = v & (1<<5)
            debug("ASI system status=",bin(v))
            if progstart and not resetActive:
                break
            else:
                debug("bootdone=",bootdone,"progstart=",progstart,"resetActive=",resetActive)
                time.sleep(0.01)
        else:
            debug("Never got to good state")
            raise TimeoutError()

        debug("//////////////Programming enabled")
        #if we get here, we've enabled programming
        return

    def disableUpdiAndDisconnect(self) -> None:
        """Disable UPDI on the AVR"""
        if not this->connected:
            raise ProtocolError("UPDI link not established")
        #set updiDisable bit in control register B
        this->STCS(addr=0x03, value=(1<<2) )
        this->disconnect()
