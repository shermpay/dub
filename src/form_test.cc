#include "src/form.h"

#include <sstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "llvm/Support/FormatVariadic.h"

#include "src/form_format.h"
#include "src/symbol.h"

using ::dub::Form;
using ::dub::List;
using ::dub::Symbol;

TEST(FormTest, IntEquals) { EXPECT_EQ(Form(0), Form(0)); }

TEST(FormTest, SymbolEquals) {
  EXPECT_EQ(Symbol::Get("foo"), Symbol::Get("foo"));
  EXPECT_EQ(&Symbol::Get("bar"), &Symbol::Get("bar"));
  EXPECT_EQ(Form(&Symbol::Get("baz")), Form(&Symbol::Get("baz")));
  // Check !=
  EXPECT_NE(Form(&Symbol::Get("qux")), Form(&Symbol::Get("baz")));
}

TEST(FormTest, SymbolValue) {
  EXPECT_EQ(Symbol::Get("foo").value(), "foo");
}

TEST(ListTest, Equals) {
  List x = List::Make(Form(0), Form(1));
  List y = List::Make(Form(0), Form(1));
  EXPECT_EQ(x, y);
}

TEST(ListTest, Empty) {
  List x = List::Make();
  List y = List::Make();
  EXPECT_EQ(x, y);
}

TEST(ListTest, InsertStringStream) {
  std::stringstream ss;
  List x = List::Make(Form(0), Form(1), Form(2));
  ss << x;
  EXPECT_EQ(ss.str(), "(0 1 2)");
}

TEST(MutableListTest, ToList) {
  dub::MutableList mut_list;
  mut_list.Append(Form(42));

  auto list1 = List(mut_list);

  EXPECT_EQ(list1, List::Make(Form(42)));

  for (const auto& form : list1) {
    EXPECT_EQ(form, Form(42));
  }
  std::stringstream ss1;
  ss1 << list1;
  EXPECT_EQ(ss1.str(), "(42)");
}

TEST(MutableListTest, ToListCopiesData) {
  dub::MutableList mut_list;

  auto list1 = List(mut_list);
  EXPECT_EQ(list1, List::Make());

  mut_list.Append(Form(42));
  EXPECT_EQ(List(mut_list), List::Make(Form(42)));
  EXPECT_EQ(list1, List::Make());
}

TEST(MutableListTest, ToListTransfersOwnership) {
  // Ownership transferred
  auto f = []{
    dub::MutableList mlist;
    mlist.Append(Form(Symbol::Get("foo")));
    return List(mlist);
  };

  const auto& list2 = f();
  EXPECT_EQ(list2, List::Make(Form(Symbol::Get("foo"))));

  std::stringstream ss2;
  ss2 << list2;
  EXPECT_EQ(ss2.str(), "(foo)");
}

TEST(ListTest, Tail) {
  List list = List::Make(Form(0), Form(1), Form(2));

  List list2 = list.Tail();
  EXPECT_EQ(list.Head(), Form(0));
  EXPECT_EQ(list2.Size(), 2);
  EXPECT_EQ(list2.Head(), Form(1));
}

// TEST(FormatTest, AbslFormatSymbol) {
//   auto& symbol = Symbol::Get("the-sym");

//   EXPECT_EQ(absl::StrFormat("=%v=", symbol), "=the-sym=");

//   EXPECT_EQ(absl::StrFormat("=%v=", Form(symbol)), "=the-sym=");
// }

// TEST(FormatTest, AbslFormatStrForm) {
//   auto str  = Form("hey you!");

//   EXPECT_EQ(absl::StrFormat("=%v=", str), "=hey you!=");
// }

// TEST(FormatTest, AbslFormatIntForm) {
//   EXPECT_EQ(absl::StrFormat("_%v_", Form(42)), "_42_");
// }

// TEST(FormatTest, AbslFormatList) {
//   List list = List::Make(Form(0), Form(1));
//   EXPECT_EQ(absl::StrFormat("~%v~", list), "~(0 1)~");
// }


TEST(FormatTest, LLVMFormatSymbol) {
  auto& symbol = Symbol::Get("the-sym");

  EXPECT_EQ(llvm::formatv(true, "={0}=", symbol).str(), "=the-sym=");

  EXPECT_EQ(llvm::formatv("={0}=", Form(symbol)).str(), "=the-sym=");
}

TEST(FormatTest, LLVMFormatStrForm) {
  auto str  = Form("hey you!");

  EXPECT_EQ(llvm::formatv("={0}=", str).str(), "=hey you!=");
}

TEST(FormatTest, LLVMFormatIntForm) {
  EXPECT_EQ(llvm::formatv("_{0}_", Form(42)).str(), "_42_");
}

TEST(FormatTest, LLVMFormatList) {
  List list = List::Make(Form(0), Form(1));
  EXPECT_EQ(llvm::formatv("~{0}~", list).str(), "~(0 1)~");
}
