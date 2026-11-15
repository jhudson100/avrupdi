#include "Json.h"
#include <format>
#include <cassert>

bool startswith(const std::string& s, std::size_t i, const std::string& s2);

Json::Json(){
    this->value = Json::Undefined();
}

const Json& Json::operator[](std::string key) const {
    auto& d = this->as_dict();
    auto it = d.find(key);
    if( it == d.end() )
        throw std::runtime_error(std::format("No key {}",key));
    return it->second;
}

bool Json::as_bool() const {
    return std::get<bool>(this->value);
}

const std::string& Json::as_string() const{
    return std::get<std::string>(this->value);
}

double Json::as_double() const {
    return std::get<double>(this->value);
}

const std::vector<Json>& Json::as_array() const{
    return std::get<std::vector<Json> >(this->value);
}

const std::map<std::string, Json>& Json::as_dict() const {
    return std::get<std::map<std::string,Json>>(this->value);
}

bool Json::is_undefined() const {
    return std::holds_alternative<Undefined>(this->value);
}

bool Json::is_null() const {
    return std::holds_alternative<Null>(this->value);
}

Json::Json(const std::string& s) : value(s) {}
Json::Json(double d) : value(d) {}
Json::Json( const std::vector<Json>& v) : value(v) {}
Json::Json( const std::map<std::string, Json>& m): value(m) {}
Json::Json(bool b) : value(b) {}
Json::Json(Undefined b): value(b) {}
Json::Json(Null b): value(b) {}

    
void skipws(const std::string& s, std::size_t& i){
    while( i < s.length() && isspace( s[i] ) )
        i++;
}

void expect( const std::string& s, std::size_t& i, const std::string& expected){
    skipws(s,i);
    if( !startswith(s,i,expected) ){
        throw Json::error(std::format( "Expected {} at index {}",expected,i) );
    }
    i += expected.length();
}

bool startswith(const std::string& s, std::size_t i, const std::string& s2){
    for(std::size_t j=0;j<s2.length();++j,++i){
        if( i >= s.length() )
            return false;
        if( s2[j] != s[i] )
            return false;
    }
    return true;
}
        
    
std::string readString(const std::string& s, std::size_t& i){
    assert( i < s.length() );
    expect(s,i,"\"");
    bool nextIsLiteral=false;
    std::string res;
    while(true){
        if( i == s.length() )
            throw Json::error("String lacks closing \"");
        if( nextIsLiteral ){
            res += s[i];
            nextIsLiteral=false;
        } else if( s[i] == '\\' ){
            nextIsLiteral=true;
        } else if( s[i] == '"' ){
            i++;
            return res;
        } else {
            res += s[i];
        }
        i++;
    }
}

bool readBool(const std::string& s, std::size_t& i ){
    skipws(s,i);
    if( startswith(s,i,"true")){
        expect(s,i,"true");
        return true;
    } 
    if( startswith(s,i,"false")){
        expect(s,i,"false");
        return false;
    }
    
    throw Json::error(std::format("Expected boolean but did not find one at index {}",i));
}


//the index is *not* a reference here
bool couldBeNumber(const std::string s, std::size_t i ){
    skipws(s,i);
    if( s[i] == '-' || (s[i]>='0' && s[i] <= '9') || s[i] == '.' )
        return true;
    else
        return false;
}

double readNumber(const std::string s, std::size_t& i ){
    skipws(s,i);
    char* endp=nullptr;
    const char* startp = s.c_str()+i;
    double d = strtod(startp, &endp);
    if( endp == startp )
        throw Json::error(std::format("Malformed number at index {}; expected number",i));
    i += endp-startp;
    return d;
}

Json::Undefined readUndefined(const std::string& s, std::size_t& i){
    expect(s,i,"undefined");
    return Json::Undefined();
}

Json::Null readNull(const std::string& s, std::size_t& i){
    expect(s,i,"null");
    return Json::Null();
}

std::vector<Json> Json::readList(const std::string& s, std::size_t& i){

    
    skipws(s,i);
    
    if( i >= s.length() )
        throw Json::error(std::format("Malformed json at index {}; expected list",i));
    if( s[i] != '[' )
        throw Json::error(std::format("Expected [ at index {}",i));

    i++;

    std::vector<Json> v;

    if( s[i] == ']' ){
        i++;
        return v;
    }

    while(true){
        if( i == s.length() )
            throw Json::error(std::format("Expected ] at index {}",i));
        v.push_back( parse(s,i) );
        skipws(s,i);
        if( i == s.length() )
            throw Json::error(std::format("Expected ] at index {}",i));
        if( s[i] == ']' ){
            expect(s,i,"]");
            return v;
        } else if( s[i] == ',' ){
            expect(s,i,",");
        } else {
            throw Json::error(std::format("Expected ] or , at index {}",i));
        }
    }
}


std::map<std::string, Json> Json::readDict(const std::string& s, std::size_t& i){

    expect(s,i,"{");
    
    std::map<std::string,Json> v;

    if( s[i] == '}' ){
        expect(s,i,"}");
        return v;
    }

    while(true){

        std::string key = readString(s,i);

        skipws(s,i);
        expect(s,i,":");
        skipws(s,i);

        v[key] = Json::parse(s,i);

        skipws(s,i);
        
        if( i == s.length() )
            throw Json::error(std::format("Expected }} at index {}",i));

        if( s[i] == ',' ){
            expect(s,i,",");
        } else if( s[i] == '}' ){
            expect(s,i,"}");
            return v;
        } else {
            throw Json::error(std::format("Malformed json at index {}; expected comma or }}",i));
        }
    }
}


Json Json::parse(const std::string& s, std::size_t& i){
    skipws(s,i);
    if( s[i] == '"' ){
        return Json( readString(s,i) );
    } else if( s[i] == '[' ){
        return Json( readList(s,i) );
    } else if( s[i] == '{' ){
        return Json( readDict(s,i) );
    } else if( couldBeNumber(s,i) ){
        return Json(readNumber(s,i));
    } else if( startswith(s,i,"true") || startswith(s,i,"false")){
        return Json(readBool(s,i));
    } else if( startswith(s,i,"undefined") ){
        return Json(readUndefined(s,i));
    } else if( startswith(s,i,"null") ){
        return Json(readNull(s,i));
    } else {
        throw Json::error(std::format( "Malformed json at index {}; not a valid JSON entity; character was {}",i,s[i]));
    }
}

Json Json::parse(const std::string& s){
    std::size_t i=0;
    return Json::parse(s,i);
}

