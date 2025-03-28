load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "testing",
    hdrs = ["data.h"],
    deps = [
        "//src:form",
        "//src/compiler:expression",
    ],
)

cc_test(
    name = "testing_test",
    srcs = ["data_test.cc"],
    deps = [
        ":testing",
        "//src:form",
        "@googletest//:gtest_main",
    ],
)
