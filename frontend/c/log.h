#pragma once

#include <vector>
#include <string>
#include <sstream>

void debugHelper(std::vector<std::string>& v);

template<typename T, typename... Args){
void debugHelper(std::vector<std::string>& v, const T& A, Args...){
    std::ostringstream& oss;
    oss << A;
    v.push_back(oss.str());
    debugHelper(v,Args...);
}

template<typename... Args>
void debug(Args...){
    std::vector<std::string> v;
    debugHelper(v,Args...);
}

