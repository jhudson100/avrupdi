//ELF parsing code

#include <fstream>

class ElfParser{
    std::fstream in;
public:
    ElfParser(std::fstream&& in_);

};
