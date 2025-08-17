# GLang

what.

## Purpose of subprojects

### Parser Generator

Exactly as you would think; to generate the parser for the bootstrap compiler.

### Bootstrap

A Minimal Implementation of the GLang Compiler, written in C. Should support the minimum amount of the language to compile the Self-Hosted Compiler.

## Language Grammar

```txt
IDENTIFIER ::= [_A-Za-z]* [_A-Za-z0-9]*

TODO: Generics
Type ::= IDENTIFIER

TODO: Traits
TODO: impl (Member Functions)
Program ::= (ExternDecl | FunctionDecl | StructDecl | EnumDecl | ConstDecl)* EOF

ConstDecl ::= "const" IDENTIFIER ":" Type "=" Expr ";"

ParamList ::= Param ("," Param)*
Param ::= IDENTIFIER ":" Type

ExternDecel ::= "extern" "fn" IDENTIFIER "(" ParamList? ")" ("->" Type)? ";"
FunctionDecl ::= "fn" IDENTIFIER "(" ParamList? ")" ("->" Type)? Block
StructDecl ::= "struct" IDENTIFIER "{" ParamList? "}"


EnumList ::= EnumItem ("," EnumItem)*
EnumItem ::= IDENTIFIER "(" Type ("," Type)* ")"
           | IDENTIFIER "{" Param ("," Param)* "}"
           | IDENTIFIER
EnumDecl ::= "enum" IDENTIFIER "{" EnumList? "}"


Stmt ::= LetStmt | IfExpr | LoopExpr | WhileExpr | Break | Continue | Return | ExprStmt

LetStmt ::= "let" IDENTIFIER (":" Type)? "=" Expr ";"
ExprStmt ::= Expr ";"
Break ::= "break" Expr? ";"
Return ::= "return" Expr? ";"
Continue ::= "continue" ";"

Expr ::= Assign | Equality | Lambda | LoopExpr | IfExpr | WhileExpr | Struct | Cast | Block
Assign ::= IDENTIFIER ("=" | "+=" | "*=" | "-=" | "/=" | "|=" | "&=") Expr
Struct ::= IDENTIFIER "{" NamedArgList? "}"
Cast ::= Expr "as" Type
Block ::= "{" Stmt* Expr "}"

Lambda ::= "|" ParamList? "|" ("->" Type)? Block
LoopExpr ::= "loop" Block
IfExpr ::= "if" Expr Block ("else" (IfExpr | Block))?
WhileExpr ::= "while" Expr Block

LambdaList ::= LambdaParam ("," LambdaParam)*
LambdaParam ::= IDENTIFIER (":" Type)?

TODO: Bitwise Operators
Equality ::= Sum (("<" | ">" | "<=" | ">=" |"==" | "!=") Sum)*
Sum ::= Product (("+" | "-") Product)*
Product ::= Unary (("*" | "/") Unary)*
Unary ::= ("!" | "-" | "~")? Call

Call ::= Primary ("(" ArgList? ")")?
Primary ::= NUMBER | STRING | IDENTIFIER | "(" Expr ")"

ArgList ::= Expr ("," Expr)*
NamedArgList ::= NamedArg ("," NamedArg)*
NamedArg ::= IDENTIFIER (":" Expr)?
```
