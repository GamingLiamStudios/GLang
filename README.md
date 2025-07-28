# GLang

what.

## Language Syntax

```txt
TODO: Traits
TODO: impl (Member Functions)
Program ::= (ExternDecl | FunctionDecl | StructDecl | EnumDecl | ConstDecl)* EOF

ExternDecel ::= "extern" "fn" IDENTIFIER "(" ParamList? ")" ("->" Type)? ";"
FunctionDecl ::= "fn" IDENTIFIER "(" ParamList? ")" ("->" Type)? Block
StructDecl ::= "struct" IDENTIFIER "{" ParamList? "}"
EnumDecl ::= "enum" IDENTIFIER "{" EnumList? "}"
ConstDecl ::= "const" IDENTIFIER ":" Type "=" Expr ";"

ParamList ::= Param ("," Param)*
Param ::= IDENTIFIER ":" Type

EnumList ::= EnumItem ("," EnumItem)*
EnumItem ::= IDENTIFIER "(" Type ("," Type)* ")"
           | IDENTIFIER "{" Param ("," Param)* "}"
           | IDENTIFIER

TODO: Generics
Type ::= IDENTIFIER

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

Equality ::= Comparison (("==" | "!=") Comparison)*
Comparison ::= Term (("<" | ">" | "<=" | ">=") Term)*
Term ::= Factor (("+" | "-") Factor)*
Factor ::= Unary (("*" | "/") Unary)*
Unary ::= ("!" | "-" | "~") Call
          | Call

Call ::= Primary ("(" ArgList? ")")*
Primary ::= NUMBER | STRING | IDENTIFIER | "(" Expr ")"

ArgList ::= Expr ("," Expr)*
NamedArgList ::= NamedArg ("," NamedArg)*
NamedArg ::= IDENTIFIER (":" Expr)?
```
