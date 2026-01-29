load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")

package(
    default_visibility = ["//visibility:public"],
)

cc_binary(
    name = "dubc",
    srcs = ["dubc.cc"],
    deps = [
        ":reader",
        "//src/compiler",
        "//src/compiler:parser",
        "//src/compiler:semantic",
        "@absl//absl/flags:flag",
        "@absl//absl/flags:parse",
        "@absl//absl/flags:usage",
    ],
)

cc_binary(
    name = "repl",
    srcs = ["repl.cc"],
    deps = [
        ":reader",
        "//src/compiler",
        "//src/compiler:parser",
        "//src/compiler:type",
    ],
)

cc_library(
    name = "reader",
    srcs = ["reader.cc"],
    hdrs = ["reader.h"],
    deps = [
        ":form",
        ":source_info",
        ":symbol",
    ],
)

cc_test(
    name = "reader_test",
    srcs = ["reader_test.cc"],
    deps = [
        ":form",
        ":reader",
        ":symbol",
        "@googletest//:gtest_main",
        "@googletest//:gtest",
        "@llvm-project//llvm:Support",
        "@llvm-project//llvm:TestingSupport",
    ],
)

cc_library(
    name = "source_info",
    srcs = ["source_info.cc"],
    hdrs = ["source_info.h"],
    deps = [
        ":form",
        "@llvm-project//llvm:Support",
    ],
)

cc_library(
    name = "form",
    srcs = [
        "form.cc",
        "form_format.cc",
    ],
    hdrs = [
        "form.h",
        "form_format.h",
    ],
    deps = [
        ":symbol",
        "@llvm-project//llvm:Support",
    ],
)

cc_test(
    name = "form_test",
    srcs = ["form_test.cc"],
    deps = [
        ":form",
        "@googletest//:gtest_main",
        "@googletest//:gtest",
        "@llvm-project//llvm:Support",
    ],
)

cc_library(
    name = "symbol",
    srcs = ["symbol.cc"],
    hdrs = ["symbol.h"],
    deps = [
        "@llvm-project//llvm:Support",
    ],
)

cc_test(
    name = "symbol_test",
    srcs = ["symbol_test.cc"],
    deps = [
        ":symbol",
        "@googletest//:gtest_main",
        "@googletest//:gtest",
        "@llvm-project//llvm:Support",
    ],
)

cc_library(
    name = "base",
    hdrs = [
        "errors.h",
        "formattable.h"
    ],
    deps = [
        "@llvm-project//llvm:Support",
    ],
)

cc_library(
    name = "lib",
    srcs = ["lib.cc"],
    hdrs = ["lib.h"],
    deps = [
    ],
)

cc_binary(
    name = "a",
    srcs = ["a.cc"],
    deps = [
        ":lib",
    ],
)
