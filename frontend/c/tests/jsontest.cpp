#include "Json.h"
#include <cassert>
#include <fstream>


int main(int argc, char* argv[])
{
    std::ifstream in(argv[1]);
    std::string s;
    getline(in,s,'\0');
    auto J = Json::parse(s);
    assert( J["ok"].as_bool() == true );
    auto v = J["data"].as_array();
    assert(v.size() == 5);
    assert(v[0].as_double() == 2.0);
    assert(v[1].as_double() == 4.0);
    assert(v[2].as_double() == 5.0);
    assert(v[3].as_double() == 6.0);
    assert(v[4].as_double() == 8.0);
    assert(J["message"].as_string() == "blah");
    return 0;
}
    
