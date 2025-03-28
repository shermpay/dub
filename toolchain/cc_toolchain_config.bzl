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

all_compile_actions = [
    # ACTION_NAMES.c_compile,
    ACTION_NAMES.cpp_compile,
    ACTION_NAMES.cc_flags_make_variable,
    ACTION_NAMES.cpp_header_parsing,
    ACTION_NAMES.assemble,
    ACTION_NAMES.preprocess_assemble,
    ACTION_NAMES.linkstamp_compile,
    ACTION_NAMES.cpp_module_compile,
    ACTION_NAMES.cpp_module_codegen,
    ACTION_NAMES.lto_backend,
    ACTION_NAMES.clif_match,
]

all_link_actions = [
    ACTION_NAMES.cpp_link_executable,
    ACTION_NAMES.cpp_link_dynamic_library,
    ACTION_NAMES.cpp_link_nodeps_dynamic_library,
]

def _my_impl(ctx):
    tool_paths = [
        tool_path(
            name = "gcc",
            path = "/usr/local/bin/clang",
        ),
        tool_path(
            name = "g++",
            path = "/usr/local/bin/clang++",
        ),
        tool_path(
            name = "ld",
            path = "/usr/local/bin/lld",
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
                    actions = all_compile_actions,
                    flag_groups = ([
                        flag_group(
                            flags = [
                                "-std=c++20",
                            ],
                        ),
                    ]),
                ),
            ],
        ),
        feature(
            name = "warning_compiler_flags",
            enabled = False,
            flag_sets = [
                flag_set(
                    actions = all_compile_actions,
                    flag_groups = ([
                        flag_group(
                            flags = [
                                "-Wall",
                                "-Wextra",
                                "-Wmissing-declarations",
                                "-Wreturn-type",
                            ],
                        ),
                    ]),
                ),
            ],
        ),
        feature(
            name = "default_linker_flags",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = all_link_actions,
                    flag_groups = ([
                        flag_group(
                            flags = [
                                "-lstdc++",
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
            # "/usr/lib/llvm-9/lib/clang/9.0.1/include",
            # "/usr/local/include/c++/v1",
            "/usr/local/lib/clang/19/include",
            "/usr/local/include",
            "/usr/include",
        ],
        toolchain_identifier = "k8-toolchain",
        host_system_name = "local",
        target_system_name = "local",
        target_cpu = "k8",
        target_libc = "unknown",
        compiler = "clang",
        abi_version = "unknown",
        abi_libc_version = "unknown",
        tool_paths = tool_paths,
    )

def _impl(ctx):
    tool_paths = [
        tool_path(name = "ar", path = "/usr/bin/ar"),
        tool_path(name = "ld", path = "/usr/local/bin/ld.lld"),
        tool_path(name = "cpp", path = "/usr/local/bin/clang"),
        tool_path(name = "gcc", path = "/usr/local/bin/clang"),
        tool_path(name = "dwp", path = "/usr/bin/dwp"),
        tool_path(name = "gcov", path = "/dev/null"),
        tool_path(name = "nm", path = "/usr/bin/nm"),
        tool_path(name = "objcopy", path = "/usr/bin/objcopy"),
        tool_path(name = "objdump", path = "/usr/bin/objdump"),
        tool_path(name = "strip", path = "/usr/bin/strip"),
    ]

    cxx_builtin_include_directories = [
        # "/usr/local/include",
        # "/usr/local/lib/clang/8.0.0/include",
        # "/usr/include/x86_64-linux-gnu",
        # "/usr/include",
        # "/usr/include/c++/4.9",
        # "/usr/include/x86_64-linux-gnu/c++/4.9",
        # "/usr/include/c++/4.9/backward",
        "/usr/local/include/c++/v1",
        "/usr/local/lib/clang/19/include",
        "/usr/include",
    ]

    action_configs = []

    compile_flags = [
        "-U_FORTIFY_SOURCE",
        "-fstack-protector",
        "-Wall",
        "-Wthread-safety",
        "-Wself-assign",
        "-fcolor-diagnostics",
        "-fno-omit-frame-pointer",
        "-isystem", "/usr/local/include/c++/v1",
        "-isystem", "/usr/local/lib/clang/19/include",
        "-isystem", "/usr/include",
    ]

    dbg_compile_flags = [
        # "-g",
    ]

    opt_compile_flags = [
        # "-g0",
        # "-O2",
        # "-D_FORTIFY_SOURCE=1",
        # "-DNDEBUG",
        # "-ffunction-sections",
        # "-fdata-sections",
    ]

    cxx_flags = [
        "-std=c++20",
    ]

    link_flags = [
        # "-fuse-ld=gold",
        # "-Wl,-no-as-needed",
        # "-Wl,-z,relro,-z,now",
        # "-B/usr/local/bin",
        "-lstdc++",
        # "-lm",
    ]

    opt_link_flags = [
        # "-Wl,--gc-sections",
    ]

    unfiltered_compile_flags = [
        # "-no-canonical-prefixes",
        # "-Wno-builtin-macro-redefined",
        # "-D__DATE__=\"redacted\"",
        # "-D__TIMESTAMP__=\"redacted\"",
        # "-D__TIME__=\"redacted\"",
    ]

    targets_windows_feature = feature(
        name = "targets_windows",
        implies = ["copy_dynamic_libraries_to_binary"],
        enabled = True,
    )

    copy_dynamic_libraries_to_binary_feature = feature(name = "copy_dynamic_libraries_to_binary")

    gcc_env_feature = feature(
        name = "gcc_env",
        enabled = True,
        env_sets = [
            env_set(
                actions = [
                    ACTION_NAMES.c_compile,
                    ACTION_NAMES.cpp_compile,
                    ACTION_NAMES.cpp_module_compile,
                    ACTION_NAMES.cpp_module_codegen,
                    ACTION_NAMES.cpp_header_parsing,
                    ACTION_NAMES.assemble,
                    ACTION_NAMES.preprocess_assemble,
                    ACTION_NAMES.cpp_link_executable,
                    ACTION_NAMES.cpp_link_dynamic_library,
                    ACTION_NAMES.cpp_link_nodeps_dynamic_library,
                    ACTION_NAMES.cpp_link_static_library,
                ],
                env_entries = [
                    env_entry(key = "PATH", value = "NOT_USED"),
                ],
            ),
        ],
    )

    windows_features = [
        targets_windows_feature,
        copy_dynamic_libraries_to_binary_feature,
        gcc_env_feature,
    ]

    coverage_feature = feature(
        name = "coverage",
        provides = ["profile"],
        flag_sets = [
            flag_set(
                actions = [
                    ACTION_NAMES.preprocess_assemble,
                    ACTION_NAMES.c_compile,
                    ACTION_NAMES.cpp_compile,
                    ACTION_NAMES.cpp_header_parsing,
                    ACTION_NAMES.cpp_module_compile,
                ],
                flag_groups = [
                    flag_group(flags = ["--coverage"]),
                ],
            ),
            flag_set(
                actions = [
                    ACTION_NAMES.cpp_link_dynamic_library,
                    ACTION_NAMES.cpp_link_nodeps_dynamic_library,
                    ACTION_NAMES.cpp_link_executable,
                ],
                flag_groups = [
                    flag_group(flags = ["--coverage"]),
                ],
            ),
        ],
    )

    supports_pic_feature = feature(
        name = "supports_pic",
        enabled = True,
    )
    supports_start_end_lib_feature = feature(
        name = "supports_start_end_lib",
        enabled = True,
    )

    default_compile_flags_feature = feature(
        name = "default_compile_flags",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = [
                    ACTION_NAMES.assemble,
                    ACTION_NAMES.preprocess_assemble,
                    ACTION_NAMES.linkstamp_compile,
                    ACTION_NAMES.c_compile,
                    ACTION_NAMES.cpp_compile,
                    ACTION_NAMES.cpp_header_parsing,
                    ACTION_NAMES.cpp_module_compile,
                    ACTION_NAMES.cpp_module_codegen,
                    ACTION_NAMES.lto_backend,
                    ACTION_NAMES.clif_match,
                ],
                flag_groups = ([flag_group(flags = compile_flags)] if compile_flags else []),
            ),
            flag_set(
                actions = [
                    ACTION_NAMES.assemble,
                    ACTION_NAMES.preprocess_assemble,
                    ACTION_NAMES.linkstamp_compile,
                    ACTION_NAMES.c_compile,
                    ACTION_NAMES.cpp_compile,
                    ACTION_NAMES.cpp_header_parsing,
                    ACTION_NAMES.cpp_module_compile,
                    ACTION_NAMES.cpp_module_codegen,
                    ACTION_NAMES.lto_backend,
                    ACTION_NAMES.clif_match,
                ],
                flag_groups = ([flag_group(flags = dbg_compile_flags)] if dbg_compile_flags else []),
                with_features = [with_feature_set(features = ["dbg"])],
            ),
            flag_set(
                actions = [
                    ACTION_NAMES.assemble,
                    ACTION_NAMES.preprocess_assemble,
                    ACTION_NAMES.linkstamp_compile,
                    ACTION_NAMES.c_compile,
                    ACTION_NAMES.cpp_compile,
                    ACTION_NAMES.cpp_header_parsing,
                    ACTION_NAMES.cpp_module_compile,
                    ACTION_NAMES.cpp_module_codegen,
                    ACTION_NAMES.lto_backend,
                    ACTION_NAMES.clif_match,
                ],
                flag_groups = ([flag_group(flags = opt_compile_flags)] if opt_compile_flags else []),
                with_features = [with_feature_set(features = ["opt"])],
            ),
            flag_set(
                actions = [
                    ACTION_NAMES.linkstamp_compile,
                    ACTION_NAMES.cpp_compile,
                    ACTION_NAMES.cpp_header_parsing,
                    ACTION_NAMES.cpp_module_compile,
                    ACTION_NAMES.cpp_module_codegen,
                    ACTION_NAMES.lto_backend,
                    ACTION_NAMES.clif_match,
                ],
                flag_groups = ([flag_group(flags = cxx_flags)] if cxx_flags else []),
            ),
        ],
    )

    default_link_flags_feature = feature(
        name = "default_link_flags",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = all_link_actions,
                flag_groups = ([flag_group(flags = link_flags)] if link_flags else []),
            ),
            flag_set(
                actions = all_link_actions,
                flag_groups = ([flag_group(flags = opt_link_flags)] if opt_link_flags else []),
                with_features = [with_feature_set(features = ["opt"])],
            ),
        ],
    )

    dbg_feature = feature(name = "dbg")

    opt_feature = feature(name = "opt")

    sysroot_feature = feature(
        name = "sysroot",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = [
                    ACTION_NAMES.preprocess_assemble,
                    ACTION_NAMES.linkstamp_compile,
                    ACTION_NAMES.c_compile,
                    ACTION_NAMES.cpp_compile,
                    ACTION_NAMES.cpp_header_parsing,
                    ACTION_NAMES.cpp_module_compile,
                    ACTION_NAMES.cpp_module_codegen,
                    ACTION_NAMES.lto_backend,
                    ACTION_NAMES.clif_match,
                    ACTION_NAMES.cpp_link_executable,
                    ACTION_NAMES.cpp_link_dynamic_library,
                    ACTION_NAMES.cpp_link_nodeps_dynamic_library,
                ],
                flag_groups = [
                    flag_group(
                        flags = ["--sysroot=%{sysroot}"],
                        expand_if_available = "sysroot",
                    ),
                ],
            ),
        ],
    )

    fdo_optimize_feature = feature(
        name = "fdo_optimize",
        flag_sets = [
            flag_set(
                actions = [ACTION_NAMES.c_compile, ACTION_NAMES.cpp_compile],
                flag_groups = [
                    flag_group(
                        flags = [
                            "-fprofile-use=%{fdo_profile_path}",
                            "-fprofile-correction",
                        ],
                        expand_if_available = "fdo_profile_path",
                    ),
                ],
            ),
        ],
        provides = ["profile"],
    )

    supports_dynamic_linker_feature = feature(name = "supports_dynamic_linker", enabled = True)

    user_compile_flags_feature = feature(
        name = "user_compile_flags",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = [
                    ACTION_NAMES.assemble,
                    ACTION_NAMES.preprocess_assemble,
                    ACTION_NAMES.linkstamp_compile,
                    ACTION_NAMES.c_compile,
                    ACTION_NAMES.cpp_compile,
                    ACTION_NAMES.cpp_header_parsing,
                    ACTION_NAMES.cpp_module_compile,
                    ACTION_NAMES.cpp_module_codegen,
                    ACTION_NAMES.lto_backend,
                    ACTION_NAMES.clif_match,
                ],
                flag_groups = [
                    flag_group(
                        flags = ["%{user_compile_flags}"],
                        iterate_over = "user_compile_flags",
                        expand_if_available = "user_compile_flags",
                    ),
                ],
            ),
        ],
    )

    unfiltered_compile_flags_feature = feature(
        name = "unfiltered_compile_flags",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = [
                    ACTION_NAMES.assemble,
                    ACTION_NAMES.preprocess_assemble,
                    ACTION_NAMES.linkstamp_compile,
                    ACTION_NAMES.c_compile,
                    ACTION_NAMES.cpp_compile,
                    ACTION_NAMES.cpp_header_parsing,
                    ACTION_NAMES.cpp_module_compile,
                    ACTION_NAMES.cpp_module_codegen,
                    ACTION_NAMES.lto_backend,
                    ACTION_NAMES.clif_match,
                ],
                flag_groups = ([flag_group(flags = unfiltered_compile_flags)] if unfiltered_compile_flags else []),
            ),
        ],
    )

    features = [
        supports_pic_feature,
        supports_start_end_lib_feature,
        coverage_feature,
        default_compile_flags_feature,
        default_link_flags_feature,
        fdo_optimize_feature,
        supports_dynamic_linker_feature,
        dbg_feature,
        opt_feature,
        user_compile_flags_feature,
        sysroot_feature,
        unfiltered_compile_flags_feature,
    ]

    artifact_name_patterns = [
    ]

    make_variables = []

    return cc_common.create_cc_toolchain_config_info(
        ctx = ctx,
        features = features,
        action_configs = action_configs,
        artifact_name_patterns = artifact_name_patterns,
        cxx_builtin_include_directories = cxx_builtin_include_directories,
        toolchain_identifier = "linux_gnu_x86",
        host_system_name = "i686-unknown-linux-gnu",
        target_system_name = "x86_64-unknown-linux-gnu",
        target_cpu = "k8",
        target_libc = "glibc_2.19",
        compiler = "clang",
        abi_version = "clang",
        abi_libc_version = "glibc_2.19",
        tool_paths = tool_paths,
        make_variables = make_variables,
        builtin_sysroot = "",
        cc_target_os = None,
    )


cc_toolchain_config = rule(
    implementation = _my_impl,
    attrs = {},
    provides = [CcToolchainConfigInfo],
)
