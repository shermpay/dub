load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(
    default_visibility = ["//visibility:public"],
    features = ["warning_compile_flags"],
)

cc_library(
    name = "constant",
    hdrs = ["constant.h"],
    deps = [
        "//src:form",
        "//src:symbol",
        "@absl//absl/strings:str_format",
    ],
)

cc_test(
    name = "constant_test",
    srcs = ["constant_test.cc"],
    deps = [
        ":constant",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "expression",
    srcs = ["expression.cc"],
    hdrs = ["expression.h"],
    deps = [
        ":constant",
        ":type",
        "//src:form",
        "@absl//absl/strings:str_format",
        "@absl//absl/types:span",
    ],
)

cc_library(
    name = "expr_info",
    # srcs = ["expr_info.cc"],
    hdrs = ["expr_info.h"],
    deps = [
        ":type",
        "//src:source_info",
        "@absl//absl/strings:str_format",
    ],
)

cc_library(
    name = "type",
    srcs = ["type.cc"],
    hdrs = ["type.h"],
    deps = [
        ":constant",
        "//src:form",
        "//src:symbol",
        "@absl//absl/status:statusor",
        "@absl//absl/strings:str_format",
        "@absl//absl/types:span",
    ],
)

cc_library(
    name = "type_info",
    srcs = ["type_info.cc"],
    hdrs = ["type_info.h"],
    deps = [
        ":type",
        "//src:symbol",
        "@absl//absl/container:node_hash_map",
        "@absl//absl/log:die_if_null",
    ],
)

cc_library(
    name = "module",
    srcs = ["module.cc"],
    hdrs = ["module.h"],
    deps = [
        ":expr_info",
        ":expression",
        "@absl//absl/strings:str_format",
        "@absl//absl/types:span",
    ],
)

cc_library(
    name = "typed_module",
    # srcs = ["typed_module.cc"],
    hdrs = ["typed_module.h"],
    deps = [
        ":expression",
        ":module",
        ":type",
        ":type_info",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/strings:str_format",
        "@absl//absl/types:span",
    ],
)

cc_library(
    name = "compiler",
    srcs = ["compiler.cc"],
    hdrs = ["compiler.h"],
    deps = [
        ":llvm_gen",
        ":parser",
        ":semantic",
        ":target_gen",
        "@absl//absl/status",
        "@llvm-project//llvm:BitWriter",
        "@llvm-project//llvm:Support",
    ],
)

cc_library(
    name = "parser",
    srcs = ["parser.cc"],
    hdrs = ["parser.h"],
    deps = [
        ":expr_info",
        ":expression",
        ":module",
        "@absl//absl/status:statusor",
    ],
)

cc_test(
    name = "parser_test",
    srcs = ["parser_test.cc"],
    deps = [
        ":parser",
        "//src/testing",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "semantic",
    srcs = ["semantic.cc"],
    hdrs = ["semantic.h"],
    deps = [
        ":constant",
        ":expression",
        ":result",
        ":type",
        ":type_info",
        ":typed_module",
        "//src:symbol",
        "@absl//absl/container:node_hash_map",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings:str_format",
    ],
)

cc_test(
    name = "semantic_test",
    srcs = ["semantic_test.cc"],
    deps = [
        ":module",
        ":semantic",
        ":type_info",
        "//src:symbol",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "result",
    srcs = ["result.cc"],
    hdrs = ["result.h"],
    deps = [
        "@absl//absl/status:statusor",
        "@absl//absl/strings:str_format",
    ],
)

cc_test(
    name = "result_test",
    srcs = ["result_test.cc"],
    deps = [
        ":result",
        "@absl//absl/status",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "llvm_gen",
    srcs = ["llvm_gen.cc"],
    hdrs = ["llvm_gen.h"],
    deps = [
        ":expression",
        ":semantic",
        "//src:form",
        "//src:symbol",
        "@llvm-project//llvm:Core",
    ],
)

cc_test(
    name = "llvm_gen_test",
    srcs = ["llvm_gen_test.cc"],
    deps = [
        ":llvm_gen",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "target_gen",
    srcs = ["target_gen.cc"],
    hdrs = ["target_gen.h"],
    deps = [
        "@absl//absl/status:statusor",
        "@llvm-project//llvm:AllTargetsAsmParsers",
        "@llvm-project//llvm:AllTargetsCodeGens",
        # "@llvm//llvm:AllTargetsDisassemblers",
        "@llvm-project//llvm:AllTargetsMCAs",
        "@llvm-project//llvm:CodeGen",
        "@llvm-project//llvm:Core",
        "@llvm-project//llvm:Support",
    ],
)
