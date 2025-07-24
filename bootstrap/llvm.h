#pragma once

#include <llvm-c/Core.h>

struct llvm_context
{
    LLVMContextRef context;
    LLVMBuilderRef builder;
    LLVMModuleRef  module;
};

int ast_generate_ir(/*struct ast_program *program,*/ struct llvm_context ctx)
{
    ctx.builder = LLVMCreateBuilderInContext(ctx.context);

    // LLVMModule; Functions & Global Variables
    // LLVM works in SSAs; Immutable by default

    // LLVMBuild handles operations (instructions); Returns SSA allocation
    // LLVMConst handles constants (duh)
    // LLVMFunctionType builds the function type

    // LLVMAddFunction adds the function definition to the LLVMModule
    // LLVMGetFirst/NextParam allows iteration over parameters of function

    // 'BasicBlock' runs code, and *must* return from a control flow
    // Functions, If blocks, etc; all would be structured with 'BasicBlock's
    // 'phi node' is essentially a version selector for a branch result
}