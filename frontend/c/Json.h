#pragma once

#include <variant>
#include <stdexcept>
#include <vector>
#include <map>


class Json{
  public:

    class error : public std::runtime_error{
        public:
        error(std::string s) : std::runtime_error(s) {}
    };
    
    class Undefined{
        public:
        bool operator==(const Undefined& other) const {
            return true;
        }
        bool operator!=(const Undefined& other) const {
            return !( *this == other );
        }
    };
    class Null{
        public:
        bool operator==(const Null& other) const {
            return true;
        }
        bool operator!=(const Null& other) const {
            return !(*this == other);
        }
    };


    
    typedef std::variant<
        std::string,
        double,
        std::vector<Json>,
        std::map<std::string,Json>,
        bool,
        Undefined,
        Null >
        ValueType;
        
    static Json parse(const std::string& s);
    
    const Json& operator[](std::string key) const;
    bool as_bool() const;
    const std::string& as_string() const;
    double as_double() const;
    const std::vector<Json>& as_array() const;
    const std::map<std::string, Json>& as_dict() const;
    bool is_undefined() const;
    bool is_null() const;

    Json();
    
    private:
    static Json parse(const std::string& s, std::size_t& i);
    static std::map<std::string, Json> readDict(const std::string& s, std::size_t& i);
    static std::vector<Json> readList(const std::string& s, std::size_t& i);
    Json(const std::string& s);
    Json(double d);
    Json( const std::vector<Json>& v);
    Json( const std::map<std::string, Json>& m);
    Json(bool b);
    Json(Undefined b);
    Json(Null b);
    
    ValueType value;
};
            
