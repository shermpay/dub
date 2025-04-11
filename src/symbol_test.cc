#include "src/symbol.h"

#include <sstream>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "llvm/Support/FormatVariadic.h"

using ::dub::Symbol;

TEST(SymbolTest, GetAndValue) {
  const Symbol& foo = Symbol::Get("foo");
  const Symbol& bar = Symbol::Get("bar");

  EXPECT_EQ(foo, foo);
  EXPECT_EQ(bar, bar);
  EXPECT_NE(foo, bar);

  EXPECT_EQ(foo.value(), "foo");
  EXPECT_EQ(bar.value(), "bar");

  const Symbol& foo2 = Symbol::Get("foo");
  EXPECT_EQ(&foo, &foo2);
}

TEST(SymbolTest, GetReturnsSamePointerAndHash) {
  const std::size_t num_symbols = 512;
  std::string names[num_symbols];
  std::uintptr_t addrs[num_symbols];
  std::size_t hashes[num_symbols];
  for (std::size_t i = 0; i < num_symbols; ++i) {
    std::ostringstream out;
    out << "x" << i;
    auto name = out.str();
    auto& sym = Symbol::Get(name);
    names[i] = name;
    addrs[i] = reinterpret_cast<std::uintptr_t>(&sym);
    hashes[i] = sym.hash();
  }

  for (std::size_t i = 0; i < num_symbols; ++i) {
    auto& sym = Symbol::Get(names[i]);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&sym), addrs[i]);
    EXPECT_EQ(sym.hash(), hashes[i]);
  }
}

TEST(SymbolTest, OStream) {
  const Symbol& sym = Symbol::Get("a-symbol");

  std::ostringstream out;
  out << sym;

  EXPECT_EQ(out.str(), "a-symbol");
}

TEST(SymbolTest, LLVMFormat) {
  const Symbol& sym = Symbol::Get("a-symbol");

  EXPECT_EQ(llvm::formatv(true, "%{0}%", sym).str(), "%a-symbol%");
}
