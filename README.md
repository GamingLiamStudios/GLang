# GLang

what.

## Language Syntax

```txt
TODO: Traits
TODO: impl (Member Functions)
Program ::= (ExternDecl | FunctionDecl | StructDecl | EnumDecl | Stmt)* EOF

ExternDecel ::= "extern" "fn" IDENTIFIER "(" ParamList? ")" ("->" Type)? ";"
FunctionDecl ::= "fn" IDENTIFIER "(" ParamList? ")" ("->" Type)? Block
StructDecl ::= "struct" IDENTIFIER "{" ParamList? "}"
EnumDecl ::= "enum" IDENTIFIER "{" EnumList? "}"

ParamList ::= Param ("," Param)*
Param ::= IDENTIFIER ":" Type

EnumList ::= EnumItem ("," EnumItem)*
EnumItem ::= IDENTIFIER "(" Type ("," Type)* ")"
           | IDENTIFIER "{" Param ("," Param)* "}"
           | IDENTIFIER

TODO: Generics
Type ::= IDENTIFIER

Stmt ::= LetStmt | ExprStmt | Block | IfExpr | LoopExpr | WhileExpr | Break | Continue | Return

LetStmt ::= "let" IDENTIFIER (":" Type)? "=" Expr ";"
ExprStmt ::= Expr ";"
Block ::= "{" Stmt* Expr? "}"
Break ::= "break" Expr? ";"
Continue ::= "continue" ";"
Return ::= "return" Expr? ";"

Expr ::= Assign | Equality | Lambda | LoopExpr | IfExpr | WhileExpr | Struct | Cast
Assign ::= IDENTIFIER ("=" | "+=" | "*=" | "-=" | "/=" | "|=" | "&=") Expr
Struct ::= IDENTIFIER "{" NamedArgList? "}"
Cast ::= Expr "as" Type

Lambda ::= "|" ParamList? "|" ("->" Type) Block
LoopExpr ::= "loop" Block
IfExpr ::= "if" Expr Block ("else" Expr)?
WhileExpr ::= "while" Expr Block

LambdaList ::= LambdaParam ("," LambdaParam)*
LambdaParam ::= IDENTIFIER (":" Type)?

Equality ::= Comparison (("==" | "!=") Comparison)*
Comparison ::= Term (("<" | ">" | "<=" | ">=") Term)*
Term ::= Factor (("+" | "-") Factor)*
Factor ::= Unary (("*" | "/") Unary)*
Unary ::= ("!" | "-" | "~") Unary
          | Call

Call ::= Primary ("(" ArgList? ")")*
Primary ::= NUMBER | STRING | IDENTIFIER | "(" Expr ")"

ArgList ::= Expr ("," Expr)*
NamedArgList ::= NamedArg ("," NamedArg)*
NamedArg ::= IDENTIFIER ":" Expr
```
