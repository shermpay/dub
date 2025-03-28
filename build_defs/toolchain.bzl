def _cc_toolchain_impl(ctx):
    toolchain_info = platform_common.ToolchainInfo(
        ccinfo = CcInfo(
            compiler_path = ctx.attr.compiler_path,
            system_lib = ctx.attr.system_lib,
            arch_flags = ctx.attr.arch_flags,
        ),
    )
    return [toolchain_info]

cc_toolchain = rule(
    implementation = _cc_toolchain_impl,
    attrs = {
        "compiler_path": attr.string(),
        "system_lib": attr.string(),
        "arch_flags": attr.string_list(),
    },
)
