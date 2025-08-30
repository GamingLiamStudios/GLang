#load("@rules_cc//cc:cc_library.bzl", "cc_library")

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "libpg",
    srcs = [
        "ansi.h",
        "error.h",
        "log.c",
        "log.h",
        "table.c",
        "table.h",
        "token.c",
        "token.h",
        "tree.c",
        "tree.h",
    ],
    hdrs = [
        "log.h",
        "table.h",
        "token.h",
        "tree.h",
    ],
    copts = [
        "-std=c11",
    ],
)

cc_binary(
    name = "glc_pg",
    srcs = [
        "main.c",
    ],
    deps = [
        ":libpg",
    ],
)

cc_test(
    name = "libpg_test_expr",
    size = "small",
    srcs = [
        "expr_test/lexer.c",
        "expr_test/main.c",
        "expr_test/token.h",
    ],
    copts = [
        "-std=c11",
    ],
    data = [
        "expr_test/expr.pg",
    ],
    deps = [
        ":libpg",
    ],
)
