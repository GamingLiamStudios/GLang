# GLC Parser Generator

"I used the stones to destroy the stones." - A very wise man

## Grammar

```txt
Root ::= (Rule)* EOF

Rule ::= IDENT '::=' (Terminal | NonTerminal)+

Terminal ::= STRING
NonTerminal ::= IDENT
```
