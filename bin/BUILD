load("@rules_python//python:defs.bzl", "py_binary")

py_binary(
    name = "dub",
    srcs = ["dub.py"],
    data = [
        "//src:dubc",
    ],
    deps = ["@rules_python//python/runfiles"],
)
