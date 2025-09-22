load("@bazel_skylib//rules:run_binary.bzl", "run_binary")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

genrule(
    name = "bison",
    srcs = [
        "tree.h",
        "parser.y",
    ],
    outs = [
        "parser.c",
        "parser.h",
    ],
    cmd = """
    cp $(location :tree.h) ./tree.h
    bison -o $(location :parser.c) $(location :parser.y) -Wcounterexamples
    cp parser.h $(location :parser.h)
    """,
)

genrule(
    name = "flex",
    srcs = [
        "tokens.l",
    ],
    outs = [
        "lexer.c",
        "lexer.h",
    ],
    cmd = "flex -o $(location :lexer.c) --header-file=$(location :lexer.h) $(location :tokens.l)",
)

cc_library(
    name = "glc_parser",
    srcs = [
        "tree.h",
        ":bison",
        ":flex",
    ],
    hdrs = [
        "lexer.h",
        "parser.h",
    ],
    copts = ["-std=c11"],
    deps = [
        ":glc_log",
    ],
)

cc_library(
    name = "glc_log",
    srcs = [
        "log.c",
    ],
    hdrs = [
        "ansi.h",
        "log.h",
    ],
    copts = ["-std=c11"],
)

# TODO: Figure out how to do bazel deps properly (LLVM)
cc_binary(
    name = "glc",
    srcs = [
        "llvm.c",
        "llvm.h",
        "main.c",
        "tree.c",
        "tree.h",
    ],
    copts = [
        "-std=c11",
        "-lc",
    ],
    deps = [
        ":glc_log",
        ":glc_parser",
        #"@llvm",
    ],
)
