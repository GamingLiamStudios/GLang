#include "parse-gen/token.h"
#include "parse-gen/tree.h"
#include "parse-gen/log.h"

#include <gtest/gtest.h>

#include <stdio.h>
#include <errno.h>

extern "C"
{
    enum LogLevel verbosity = LogLevel::E_DEBUG;
}

// Tests the entire stack; from input syntax, to parse table, to parsed AST
TEST(ExprLang, Lexer)
{
    int ret;

    struct glcpg_grammar grammar = {};

    const char *filepath = "parse-gen/tests/expr.pg";

    // First, tokenize all the files
    FILE *file = fopen(filepath, "r");
    ASSERT_NE(file, nullptr) << "Error while opening file: " << strerror(errno);

    struct glcpg_token *tokens;
    ptrdiff_t           len = glcpg_lexer_file(&tokens, file);
    ASSERT_GT(len, 0) << "Failed to read input file: " << len;

    ret = glcpg_parse(&grammar, tokens);
    ASSERT_GE(ret, 0) << "Failed to parse file: " << ret;

    fclose(file);
    glcpg_grammar_classify(&grammar);

    // TODO: Check against expected

    // Free everything...
    glcpg_grammar_free(&grammar);
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}