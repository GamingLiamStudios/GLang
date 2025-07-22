load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
#load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_binary(
    name = "glc",
    srcs = [
        "ansi.h",
        "entrypoint.c",
        "log.h",
    ],
    copts = ["-std=c11"],
)
