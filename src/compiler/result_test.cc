#include "result.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <llvm/Support/raw_ostream.h>

TEST(ResultTest, ValueNoError) {
  dub::Result<int> res(42);

  EXPECT_TRUE(res.IsOk());
  EXPECT_EQ(res.Value(), 42);
  EXPECT_EQ(res.value(), 42);
}

TEST(ResultTest, ErrorNoValue) {
  dub::StatusError err(absl::InternalError("oops!"));

  dub::Result<int> res(llvm::make_error<dub::StatusError>(err));
  EXPECT_FALSE(res.IsOk());
}

TEST(ResultTest, ErrorVectorNoValue) {
  dub::StatusError foo(absl::InternalError("foo"));
  dub::StatusError bar(absl::InternalError("bar"));

  std::vector<llvm::Error> errs;
  errs.push_back(llvm::make_error<dub::StatusError>(foo));
  errs.push_back(llvm::make_error<dub::StatusError>(bar));
  dub::Result<int> res(errs);
  EXPECT_FALSE(res.IsOk());

  {
    std::string s;
    llvm::raw_string_ostream(s) << res.errors()[0];
    EXPECT_EQ(s, foo.message());
  }

  {
    std::string s;
    llvm::raw_string_ostream(s) << res.errors()[1];
    EXPECT_EQ(s, bar.message());
  }
}

TEST(ResultTest, AddAllErrors) {
  dub::StatusError foo(absl::InternalError("foo"));

  std::vector<llvm::Error> errs;
  errs.push_back(llvm::make_error<dub::StatusError>(foo));
  dub::Result<int> res(errs);
  EXPECT_FALSE(res.IsOk());

  dub::StatusError bar(absl::InternalError("bar"));
  dub::StatusError baz(absl::InternalError("baz"));
  std::vector<llvm::Error> others;
  others.push_back(llvm::make_error<dub::StatusError>(bar));
  others.push_back(llvm::make_error<dub::StatusError>(baz));

  res.AddAllErrors(others);
  {
    std::string s;
    llvm::raw_string_ostream(s) << res.errors()[0];
    EXPECT_EQ(s, foo.message());
  }

  {
    std::string s;
    llvm::raw_string_ostream(s) << res.errors()[1];
    EXPECT_EQ(s, bar.message());
  }

  {
    std::string s;
    llvm::raw_string_ostream(s) << res.errors()[2];
    EXPECT_EQ(s, baz.message());
  }
}
