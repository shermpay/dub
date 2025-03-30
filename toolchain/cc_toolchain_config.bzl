load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "ACTION_NAMES")
load(
    "@bazel_tools//tools/cpp:cc_toolchain_config_lib.bzl",
    "env_entry",
    "env_set",
    "feature",
    "flag_group",
    "flag_set",
    "tool_path",
    "with_feature_set",
)

all_c_compile_actions = [
    ACTION_NAMES.c_compile,
    ACTION_NAMES.assemble,
    ACTION_NAMES.preprocess_assemble,
]

all_cpp_compile_actions = [
    ACTION_NAMES.cpp_compile,
    ACTION_NAMES.linkstamp_compile,
    ACTION_NAMES.cpp_header_parsing,
    ACTION_NAMES.cpp_module_compile,
    ACTION_NAMES.cpp_module_codegen,
]

all_compile_actions = all_c_compile_actions + all_cpp_compile_actions

all_link_actions = [
    ACTION_NAMES.cpp_link_executable,
    ACTION_NAMES.cpp_link_dynamic_library,
    ACTION_NAMES.cpp_link_nodeps_dynamic_library,
]

# Good examples here:
# https://github.com/carbon-language/carbon-lang/blob/9134e36ec0fbbe54fa3f81954c9a23cbfd56f2eb/bazel/cc_toolchains/clang_cc_toolchain_config.bzl
# https://github.com/bazel-contrib/toolchains_llvm
def _impl(ctx):
    tool_paths = [
        tool_path(
            name = "gcc",
            path = "/usr/local/bin/clang",
        ),
        tool_path(
            name = "ld",
            path = "/usr/local/bin/ld.lld",
        ),
        tool_path(
            name = "ar",
            path = "/usr/local/bin/llvm-ar",
        ),
        tool_path(
            name = "cpp",
            path = "/usr/local/bin/clang",
        ),
        tool_path(
            name = "gcov",
            path = "/usr/local/bin/llvm-cov",
        ),
        tool_path(
            name = "nm",
            path = "/usr/local/bin/llvm-nm",
        ),
        tool_path(
            name = "objdump",
            path = "/usr/local/bin/llvm-objdump",
        ),
        tool_path(
            name = "strip",
            path = "/usr/local/bin/llvm-strip",
        ),
    ]
    features = [
        feature(
            name = "default_compiler_flags",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = all_compile_actions + all_link_actions,
                    flag_groups = ([
                        flag_group(
                            flags = [
                                "-no-canonical-prefixes",
                                "-fcolor-diagnostics",
                            ],
                        ),
                    ]),
                ),
                flag_set(
                    actions = all_compile_actions,
                    flag_groups = ([
                        flag_group(
                            flags = [
                                "-Werror",
                                "-Wall",
                                "-Wextra",
                                "-Wmissing-declarations",
                                "-Wreturn-type",
                                "-Wthread-safety",
                                "-Wself-assign",
                                "-Wimplicit-fallthrough",
                                "-Wctad-maybe-unsupported",
                                "-Wextra-semi",
                                "-Wmissing-prototypes",
                                "-Wzero-as-null-pointer-constant",
                                "-Wdelete-non-virtual-dtor",
                                "-Wno-missing-designated-field-initializers",
                                # Don't warn on external code.
                                "--system-header-prefix=absl/",
                                "--system-header-prefix=gmock/",
                                "--system-header-prefix=gtest/",
                                "--system-header-prefix=llvm/",
                                "-c",
                            ],
                        ),
                    ]),
                ),

                flag_set(
                    actions = all_cpp_compile_actions,
                    flag_groups = ([
                        flag_group(
                            flags = [
                                "-std=c++20",
                                "-stdlib=libc++",
                            ],
                        ),
                    ]),
                ),

                flag_set(
                    actions = all_link_actions,
                    flag_groups = ([
                        flag_group(
                            flags = [
                                # "-Wl,-S",
                                "-fuse-ld=lld",
                                "-L/usr/local/lib",
                                "-lc++",
                                "-lc",
                                "-lm",
                                "-l:libc++abi.a",
                            ],
                        ),
                    ]),
                ),
            ],
        ),
    ]
    return cc_common.create_cc_toolchain_config_info(
        ctx = ctx,
        features = features,
        cxx_builtin_include_directories = [
 "/usr/local/include/x86_64-unknown-linux-gnu/c++/v1",
 "/usr/local/include/c++/v1",
 "/usr/local/lib/clang/19/include",
 "/usr/local/include",
 "/usr/include/x86_64-linux-gnu",
 "/usr/include",
        ],
        toolchain_identifier = "k8-toolchain",
        host_system_name = "local",
        target_system_name = "local",
        target_cpu = "k8",
        target_libc = "local",
        compiler = "clang",
        abi_version = "clang",
        abi_libc_version = "local",
        tool_paths = tool_paths,
    )



cc_toolchain_config = rule(
    implementation = _impl,
    attrs = {},
    provides = [CcToolchainConfigInfo],
)
