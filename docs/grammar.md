# MiniLang Grammar Specification

## Context-Free Grammar (BNF)

```
program     → statement*

statement   → SET ID '=' expr NL
            | PRINT expr NL
            | IF expr NL statement* (ELSE NL statement*)? END NL
            | WHILE expr NL statement* END NL

expr        → or
or          → and (OR and)*
and         → not (AND not)*
not         → NOT not | comparison
comparison  → arith ((> | < | >= | <= | == | !=) arith)*
arith       → term ((+ | -) term)*
term        → unary ((* | / | %) unary)*
unary       → '-' unary | factor
factor      → INTEGER | FLOAT | STRING | TRUE | FALSE | ID | '(' expr ')'
```

## Operator Precedence (Highest to Lowest)

| Level | Operators         | Associativity |
|-------|-------------------|---------------|
| 1     | `( )`             | —             |
| 2     | unary `-`         | Right         |
| 3     | `*` `/` `%`       | Left          |
| 4     | `+` `-`           | Left          |
| 5     | `>` `<` `>=` `<=` `==` `!=` | Left |
| 6     | `NOT`             | Right         |
| 7     | `AND`             | Left          |
| 8     | `OR`              | Left          |

## Lexer Token Specification

| Token      | Regex Pattern           | Description                    |
|------------|-------------------------|--------------------------------|
| IDENTIFIER | `[A-Za-z_][A-Za-z0-9_]*`| Variable names                 |
| INTEGER    | `[0-9]+`                | Integer literals               |
| FLOAT      | `[0-9]+\.[0-9]+`        | Floating-point literals        |
| STRING     | `"[^"\n]*"`             | String literals (no escapes)   |
| COMMENT    | `#.*`                   | Line comments                  |

## Keywords (case-sensitive)

`SET` `PRINT` `IF` `ELSE` `END` `WHILE` `AND` `OR` `NOT` `TRUE` `FALSE`

## Data Types

| Type   | Description              | Example      |
|--------|--------------------------|--------------|
| int    | 64-bit signed integer    | `42`, `-7`   |
| float  | Double-precision float   | `3.14`       |
| string | UTF-8 string             | `"hello"`    |
| bool   | Boolean value            | `TRUE`, `FALSE` |

## Type Rules

- **Arithmetic** (`+`, `-`, `*`, `/`): Both operands must be numeric (int or float)
- **int ÷ int** → truncating integer division
- **int op float** or **float op int** → float result
- **Modulo** (`%`): Both operands must be int
- **String concatenation**: `string + string` → string
- **Comparisons** (`>`, `<`, `>=`, `<=`): Both operands must be numeric
- **Equality** (`==`, `!=`): Same type or int/float mixed
- **Logical** (`AND`, `OR`, `NOT`): Operands must be bool
- **Conditions** (`IF`, `WHILE`): Expression must evaluate to bool (no truthiness)
- **Division/modulo by zero**: Runtime error
- **Dynamic typing**: Variables can be reassigned to different types
