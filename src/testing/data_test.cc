#include "src/testing/data.h"

#include <sstream>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "src/form.h"

using ::dub::Form;
using ::dub::dsl::List;

TEST(ListTest, Basic) {
  EXPECT_EQ(List(0, 1, 2), dub::List::Make(Form(0), Form(1), Form(2)));

  EXPECT_EQ(Form(dub::dsl::List(0, 1)), Form(dub::List::Make(Form(0), Form(1))));

  const auto& list = List(0, 1);
  EXPECT_EQ(list.Head(), Form(0));

  std::stringstream ss;
  ss << list;
  EXPECT_EQ(ss.str(), "(0 1)");
}
