def _llvm_config_repo_impl(ctx):
    # Run llvm-config to get flags
    cflags = ctx.execute(["llvm-config", "--cflags"]).stdout.strip().split(" ")
    ldflags = ctx.execute(["llvm-config", "--ldflags"]).stdout.strip().split(" ")
    libs = ctx.execute(["llvm-config", "--libs", "core", "executionengine", "native"]).stdout.strip().split(" ")

    # Generate BUILD file content
    build_file = """cc_library(
        name = "llvm",
        hdrs = glob(["include/**/*.h"]),
        includes = ["include"],
        copts = {cflags},
        linkopts = {linkopts},
        visibility = ["//visibility:public"],
    )""".format(
        cflags = repr(cflags),
        linkopts = repr(ldflags + libs),
    )

    # Write files into the repo
    ctx.file("BUILD", build_file)

llvm_config_repo = repository_rule(
    implementation = _llvm_config_repo_impl,
    environ = ["PATH"],  # so we can find llvm-config
)
