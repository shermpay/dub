#include "result.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "absl/status/status.h"


TEST(ErrorTest, CreateStatusError) {
  auto status = absl::UnauthenticatedError("oops");
  dub::StatusError err(status);
  EXPECT_EQ(err.Message(), status.message());
}
