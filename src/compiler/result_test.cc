#include "result.h"

#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <system_error>

TEST(ResultTest, ValueNoError) {
  dub::Result<int> res(42);

  EXPECT_TRUE(res.IsOk());
  EXPECT_EQ(res.Value(), 42);
  EXPECT_EQ(res.value(), 42);
}

TEST(ResultTest, ErrorNoValue) {
  dub::Result<int> res(llvm::createStringError(std::errc::io_error, "oops!"));
  EXPECT_FALSE(res.IsOk());
}

TEST(ResultTest, ErrorVectorNoValue) {
  std::vector<llvm::Error> errs;
  errs.push_back(llvm::createStringError(std::errc::invalid_argument, "foo"));
  errs.push_back(llvm::createStringError(std::errc::invalid_argument, "bar"));
  dub::Result<int> res(errs);
  EXPECT_FALSE(res.IsOk());

  {
    std::string s;
    llvm::raw_string_ostream(s) << res.errors()[0];
    EXPECT_EQ(s, "foo");
  }

  {
    std::string s;
    llvm::raw_string_ostream(s) << res.errors()[1];
    EXPECT_EQ(s, "bar");
  }
}

TEST(ResultTest, AddAllErrors) {
  std::vector<llvm::Error> errs;
  errs.push_back(llvm::createStringError(std::errc::invalid_argument, "foo"));
  dub::Result<int> res(errs);
  EXPECT_FALSE(res.IsOk());

  std::vector<llvm::Error> others;
  others.push_back(llvm::createStringError(std::errc::invalid_argument, "bar"));
  others.push_back(llvm::createStringError(std::errc::invalid_argument, "baz"));

  res.AddAllErrors(others);
  {
    std::string s;
    llvm::raw_string_ostream(s) << res.errors()[0];
    EXPECT_EQ(s, "foo");
  }

  {
    std::string s;
    llvm::raw_string_ostream(s) << res.errors()[1];
    EXPECT_EQ(s, "bar");
  }

  {
    std::string s;
    llvm::raw_string_ostream(s) << res.errors()[2];
    EXPECT_EQ(s, "baz");
  }
}
