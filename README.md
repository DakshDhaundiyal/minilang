# MiniLang — Mini Interpreter & Dashboard

A complete mini programming language interpreter built from scratch in C++17 with a React-based visual dashboard. System Programming course project demonstrating core interpreter/language processing concepts.

## 📋 Problem Statement

Design and implement a mini interpreter for a custom programming language (MiniLang) that demonstrates fundamental concepts of system programming: lexical analysis, parsing, abstract syntax trees, symbol table management, and interpretation — all built without external libraries.

## 🎯 Objectives

1. Build a hand-written **lexer** using finite-state/char-by-char scanning
2. Implement a **recursive-descent parser** producing a real AST
3. Create a **semantic analysis** pass for static error detection
4. Build an **AST-walking interpreter** with a symbol table
5. Output structured JSON for programmatic consumption
6. Provide a **React dashboard** that visualizes every phase of interpretation

## ✨ Features

- **Complete Interpreter Pipeline**: Source → Lexer → Parser → Semantic → Interpreter → Output
- **Error Recovery**: Lexer continues after errors; parser uses panic-mode recovery
- **Rich Error Reporting**: Errors include phase, type, message, line, and column
- **JSON Output**: Single JSON object with tokens, AST, symbols, errors, and phase status
- **Visual Dashboard**: CodeMirror editor with syntax highlighting, expandable AST tree, token table, symbol table, error panel with click-to-jump, grammar reference
- **Safety**: 3-second timeout, 100KB source limit, 100K loop iteration limit

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────┐
│                    Frontend                      │
│  React + TypeScript + Vite + Tailwind           │
│  CodeMirror 6 Editor + Dashboard Panels         │
│                                                  │
│  POST /api/execute ──────────┐                  │
└──────────────────────────────┼──────────────────┘
                               │
┌──────────────────────────────┼──────────────────┐
│                    Backend   │                   │
│  Node.js + Express + TS     │                   │
│  Spawns binary, no shell    ▼                   │
│  Timeout + size limits    ┌─────────────┐       │
│                           │ minilang.exe│       │
│                           │ (stdin/out) │       │
│                           └─────────────┘       │
└─────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────┐
│              C++ Interpreter Pipeline            │
│                                                  │
│  Source ─→ Lexer ─→ Parser ─→ Semantic ─→ Interp│
│            │         │         │           │     │
│            ▼         ▼         ▼           ▼     │
│         Tokens     AST      Errors     Output    │
│                                      Symbols     │
│                                                  │
│  All results → single JSON to stdout             │
└─────────────────────────────────────────────────┘
```

## 📝 MiniLang Syntax

### Statements
```
SET variable = expression
PRINT expression
IF expression
  ...statements...
ELSE
  ...statements...
END
WHILE expression
  ...statements...
END
# This is a comment
```

### Data Types
| Type | Description | Example |
|------|-------------|---------|
| `int` | 64-bit signed integer | `42`, `-7` |
| `float` | Double-precision float | `3.14` |
| `string` | String literal | `"hello"` |
| `bool` | Boolean | `TRUE`, `FALSE` |

### Operators (by precedence, high → low)
| Level | Operators | Associativity |
|-------|-----------|---------------|
| 1 | `( )` | — |
| 2 | unary `-` | Right |
| 3 | `*` `/` `%` | Left |
| 4 | `+` `-` | Left |
| 5 | `>` `<` `>=` `<=` `==` `!=` | Left |
| 6 | `NOT` | Right |
| 7 | `AND` | Left |
| 8 | `OR` | Left |

## 📐 Grammar (Context-Free Grammar)

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
factor      → INT | FLOAT | STRING | TRUE | FALSE | ID | '(' expr ')'
```

## 🔤 Lexer

Hand-written, character-by-character scanner using a finite-state approach.

**Token types**: `KEYWORD`, `IDENTIFIER`, `INTEGER`, `FLOAT`, `STRING`, `OPERATOR`, `DELIMITER`, `COMMENT`, `NEWLINE`, `EOF`, `ERROR`

**Regular expressions**:
| Token | Pattern |
|-------|---------|
| IDENTIFIER | `[A-Za-z_][A-Za-z0-9_]*` |
| INTEGER | `[0-9]+` |
| FLOAT | `[0-9]+\.[0-9]+` |
| STRING | `"[^"\n]*"` |
| COMMENT | `#.*` |

**Error recovery**: On invalid characters, malformed numbers (`1.`, `1.2.3`, `12abc`), or unterminated strings, the lexer records the error and continues scanning.

## 🌳 Parser (Recursive Descent)

Produces an AST with node types: `Program`, `Assign`, `Print`, `If`, `While`, `BinaryExpr`, `UnaryExpr`, `Literal`, `Variable`. Each node carries `line` and `col` for error reporting.

**Error recovery**: Panic-mode — on encountering an unexpected token, the parser skips to the next newline (statement boundary) and continues, allowing multiple errors per run. Missing `END` reports the line of the opening `IF`/`WHILE`.

## 📊 AST

Real tree structure serialized to JSON. The interpreter walks this tree — it never re-reads the source text.

## 🔍 Semantic Analysis

Static pass before execution:
- **Undeclared variable detection**: Using a variable name with no `SET` earlier in source order

## 📦 Symbol Table

Flat table with entries: `{name, type, value, scope}`. Updated on every `SET`. Type can change on reassignment (dynamic typing). Single global scope.

## ⚡ Interpreter (AST Walker)

Walks the AST executing each node. Runtime errors:
- **Undefined variable** (runtime check in addition to semantic)
- **Type errors**: wrong operand types for operators
- **Division/modulo by zero**
- **Non-boolean condition** in `IF`/`WHILE`
- **Execution limit**: > 100,000 total loop iterations

## 🚨 Error System

Errors carry `{phase, type, message, line, col}` where phase ∈ {lexer, parser, semantic, runtime, api}.

Phase execution: later phases are skipped if an earlier one fails, but partial results (tokens, AST) are still returned when available.

## 🖥️ API Response Format

```json
{
  "success": true|false,
  "output": ["line1", "line2"],
  "tokens": [{"index":0, "type":"KEYWORD", "lexeme":"SET", "line":1, "col":1}],
  "ast": { "type": "Program", "body": [...] },
  "symbols": [{"name":"x", "type":"int", "value":"42", "scope":"global"}],
  "errors": [{"phase":"runtime", "type":"...", "message":"...", "line":1, "col":1}],
  "phases": {"lexer":"ok", "parser":"ok", "semantic":"ok", "runtime":"ok"},
  "executionTimeMs": 0.5
}
```

## 🛠️ Installation & Running

### Prerequisites
- C++ compiler with C++17 support (GCC, MSVC, Clang)
- Node.js 18+
- npm 9+

### Build the Interpreter
```bash
cd interpreter
# Using the build script (Windows)
build.bat
# Or manually:
mkdir build
g++ -std=c++17 -O2 -c -I src src/*.cpp -o build/*.o
g++ build/*.o -o build/minilang.exe
```

### Run Tests
```bash
cd interpreter
.\build\minilang_tests.exe
```

### Start the Backend
```bash
cd backend
npm install
npm run dev
# Server runs on http://localhost:3001
```

### Start the Frontend
```bash
cd frontend
npm install
npm run dev
# Dashboard at http://localhost:5173
```

### Test the CLI Directly
```bash
echo 'PRINT "Hello World"' | .\interpreter\build\minilang.exe
```

## 📁 Project Structure

```
sp/
├── interpreter/              # C++17 interpreter (no dependencies)
│   ├── CMakeLists.txt        # CMake build configuration
│   ├── build.bat             # Windows build script
│   ├── src/
│   │   ├── token.h/cpp       # Token types and definitions
│   │   ├── lexer.h/cpp       # Character-by-character lexer
│   │   ├── ast.h/cpp         # AST node types with JSON serialization
│   │   ├── parser.h/cpp      # Recursive-descent parser
│   │   ├── semantic.h/cpp    # Static semantic analysis
│   │   ├── interpreter.h/cpp # AST-walking interpreter
│   │   ├── json.h/cpp        # JSON string escaping utility
│   │   └── main.cpp          # CLI: stdin → JSON stdout
│   └── tests/
│       └── test_main.cpp     # Comprehensive test suite (38 tests)
├── backend/                  # Node.js + Express + TypeScript
│   ├── package.json
│   ├── tsconfig.json
│   └── src/
│       └── index.ts          # API server with process spawning
├── frontend/                 # React + TypeScript + Vite + Tailwind
│   ├── package.json
│   ├── vite.config.ts
│   └── src/
│       ├── App.tsx           # Main dashboard component
│       ├── api.ts            # Backend API client
│       ├── types.ts          # TypeScript type definitions
│       ├── examples.ts       # Example programs
│       ├── minilang-lang.ts  # CodeMirror language support
│       └── index.css         # Global styles and theme
├── docs/
│   └── grammar.md            # Complete grammar documentation
└── README.md                 # This file
```

## 💡 Examples

### Hello World
```
PRINT "Hello, World!"
```

### Arithmetic
```
SET a = 10
SET b = 3
PRINT a + b      # 13
PRINT a / b      # 3 (truncating)
PRINT 10.0 / 3.0 # 3.33333
```

### Conditional
```
SET age = 20
IF age >= 18
  PRINT "Adult"
ELSE
  PRINT "Minor"
END
```

### Loop
```
SET sum = 0
SET i = 1
WHILE i <= 10
  SET sum = sum + i
  SET i = i + 1
END
PRINT sum  # 55
```

### Error Examples
```
# Undefined variable (semantic error)
PRINT x

# Missing END (syntax error)
IF TRUE
  PRINT "no end"

# Invalid character (lexical error)
PRINT @invalid

# Infinite loop (execution limit)
WHILE TRUE
  SET x = 1
END
```

## 📚 Syllabus Mapping

| Concept | Syllabus Unit | Implementation |
|---------|---------------|----------------|
| **Interpreter & Language Processing** | Unit 1: Language Processors | Complete interpreter pipeline |
| **Data Types & Variables** | Unit 5: Data types, variables, scope | int, float, string, bool; dynamic typing; global scope; symbol table |
| **Translation Phases** | Unit 6: Phases of translation | Lexer → Parser → Semantic → Interpreter |
| **Lexical Analysis** | Unit 6: Lexical analysis | Hand-written char-by-char lexer |
| **Regular Expressions** | Unit 6: Regular expressions | Token patterns documented and shown in UI |
| **Finite Automata** | Unit 6: Finite automata concepts | Finite-state lexer design |
| **Context-Free Grammar** | Unit 6: CFG, parsing | Documented CFG with precedence, shown in Grammar tab |
| **Parsing** | Unit 6: Parsing techniques | Recursive-descent parser |
| **Practical 7** | Practicals | Design and implement a lexical analyzer |
| **Practical 11** | Practicals | Study and implement a simple interpreter |

## ⚠️ Limitations

- **Single global scope** — no functions, closures, or block scope
- **No functions or procedures** — only built-in `PRINT`
- **No arrays or data structures** — only scalar types
- **No string escape sequences** — strings are literal
- **No file I/O** — the interpreter is sandboxed
- **No import/module system**
- **Limited to 100,000 loop iterations** (safety limit)

> **Note**: This project implements an **interpreter** only. It does not include an assembler, macro processor, loader, or linker.

## 🔮 Future Work

- Function definitions and calls
- Block scope and local variables
- Arrays and basic data structures
- String escape sequences (`\n`, `\t`, etc.)
- For loops and break/continue
- More built-in functions (input, len, type conversion)
- Source mapping for better error messages
- Step-by-step debugger in the dashboard
- WASM compilation for browser-only mode

## 📄 License

MIT — Academic project for System Programming course.
