#include "src/lib.h"

#include <iostream>
#include <optional>
#include <variant>

class Base {
public:
private:
  virtual int Id() const noexcept = 0;
};

class Derived : public Base {
  int Id() const noexcept override {
    return 42;
  }
};

int main() { return 0; }
