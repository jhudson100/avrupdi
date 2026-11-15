#pragma once

#include <functional>

class RAII{
    std::function<void()> f;
  public:
    RAII(std::function<void()> f){
        this->f=f;
    }
    ~RAII(){
        f();
    }
};

