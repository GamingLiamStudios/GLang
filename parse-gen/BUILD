#load("@rules_cc//cc:cc_library.bzl", "cc_library")

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

cc_binary(
    name = "glc_pg",
    srcs = [
        "ansi.h",
        "error.h",
        "log.c",
        "log.h",
        "main.c",
        "token.c",
        "token.h",
        "tree.c",
        "tree.h",
    ],
    copts = [
        "-std=c11",
    ],
)
