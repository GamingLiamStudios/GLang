#load("@rules_cc//cc:cc_library.bzl", "cc_library")

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

# TODO: Figure out how to do bazel deps properly
cc_binary(
    name = "glc",
    srcs = [
        "ansi.h",
        "entrypoint.c",
        "llvm.c",
        "llvm.h",
        "log.h",
        "token.c",
        "token.h",
        "tree.c",
        "tree.h",
    ],
    copts = [
        "-std=c11",
        "-lLLVM",
    ],
)
