# MiniLang — Project Explanation & System Programming Syllabus Mapping

> **Course**: System Programming (SP)
> **Project**: MiniLang — A Mini Programming Language Interpreter with React Dashboard
> **GitHub**: https://github.com/DakshDhaundiyal/minilang

---

## Table of Contents

1. [What is MiniLang?](#1-what-is-minilang)
2. [Architecture Overview](#2-architecture-overview)
3. [How the Interpreter Works — Phase by Phase](#3-how-the-interpreter-works--phase-by-phase)
4. [The Backend API Bridge](#4-the-backend-api-bridge)
5. [The Frontend Dashboard](#5-the-frontend-dashboard)
6. [Data Flow — End to End](#6-data-flow--end-to-end)
7. [System Programming Syllabus Coverage](#7-system-programming-syllabus-coverage)
8. [MiniLang Language Reference](#8-minilang-language-reference)
9. [File Structure](#9-file-structure)

---

## 1. What is MiniLang?

**MiniLang** is a fully hand-written, from-scratch programming language — including its own:

- **Lexer** — breaks raw text into tokens
- **Parser** — builds a tree structure from tokens
- **Semantic Analyzer** — catches logical errors before running
- **Interpreter** — walks the tree and executes the program

No external parser/interpreter libraries are used. No `eval()`. Every component is built from first principles in **C++17**.

The project is split into three layers:

| Layer | Technology | Role |
|-------|-----------|------|
| `interpreter/` | C++17, compiled to `.exe` | The brain — all language logic |
| `backend/` | Node.js + Express + TypeScript | The bridge — safely spawns the C++ binary |
| `frontend/` | React + Vite + Tailwind + CodeMirror 6 | The face — interactive IDE dashboard |

---

## 2. Architecture Overview

```
+----------------------------------------------------------+
|                    BROWSER (port 5173)                    |
|   +------------------------------------------------------+|
|   |  React Dashboard (CodeMirror 6 editor)               ||
|   |  Tabs: Output | Tokens | AST | Symbol Table          ||
|   +----------------------------+-------------------------+|
+--------------------------------|-------------------------+
                                 | POST /api/execute { source }
                                 v
+----------------------------------------------------------+
|              NODE.JS API SERVER (port 3001)               |
|   Validates input (100KB max)                             |
|   Spawns minilang.exe via child_process.spawn()           |
|   Enforces 3s timeout + SIGKILL                           |
|   Passes source via stdin, reads JSON from stdout         |
+----------------------------+-----------------------------+
                             | stdin: source code
                             v
+----------------------------------------------------------+
|           C++ INTERPRETER (minilang.exe)                  |
|                                                           |
|  Source -> Lexer -> Parser -> Semantic -> Runtime         |
|             |          |           |           |          |
|          Tokens       AST       Errors      Output        |
|                                                           |
|  Outputs one JSON object to stdout containing:            |
|  { success, output[], tokens[], ast{}, symbols[],        |
|    errors[], phases{}, executionTimeMs }                  |
+----------------------------------------------------------+
```

**Key design decision**: The C++ interpreter is completely standalone. It reads source from `stdin` and writes a single JSON object to `stdout`. It never touches the filesystem, network, or shell. The Node.js backend safely bridges the browser and the binary.

---

## 3. How the Interpreter Works — Phase by Phase

The interpreter runs four sequential phases. Each phase feeds its output into the next. If a phase fails, subsequent phases are skipped.

```
Source Code
    |
    v  Phase 1
  Lexer -----------------> Token Stream  (+ lex errors)
    |
    v  Phase 2
  Parser ----------------> Abstract Syntax Tree  (+ parse errors)
    |
    v  Phase 3
  Semantic Analyzer ------> Validated AST  (+ semantic errors)
    |
    v  Phase 4
  Interpreter -----------> Output Lines + Symbol Table  (+ runtime errors)
```

---

### Phase 1: Lexical Analysis (Lexer)

**Files**: `interpreter/src/lexer.cpp`, `lexer.h`

The **Lexer** is the very first phase. It takes raw source code (a string of characters) and converts it into a flat list of **tokens** — the smallest meaningful units of the language.

#### What it does

The lexer scans the source character by character, maintaining:
- `pos_` — current position in the source string
- `line_` / `col_` — current line and column for error reporting

For each character, it decides what kind of token to produce:

| Input | Token Type | Example |
|-------|-----------|---------|
| `42`, `3.14` | `INTEGER`, `FLOAT` | `42` -> `{type: INTEGER, lexeme: "42"}` |
| `"hello"` | `STRING` | `"hi"` -> `{type: STRING, lexeme: "hi"}` |
| `SET`, `PRINT`, `IF`... | `KEYWORD` | `SET` -> `{type: KEYWORD, lexeme: "SET"}` |
| `x`, `myVar` | `IDENTIFIER` | `x` -> `{type: IDENTIFIER, lexeme: "x"}` |
| `+`, `-`, `*`, `>`, `==`... | `OPERATOR` | `==` -> `{type: OPERATOR, lexeme: "=="}` |
| `(`, `)` | `LPAREN`, `RPAREN` | |
| `=` | `EQUALS` | Assignment only |
| `\n` | `NEWLINE` | Statement separator |
| `# comment` | `COMMENT` | Skipped |

#### Key techniques used

- **Single-pass scanning** — reads each character exactly once (O(n))
- **Lookahead (`peek()`)** — looks one character ahead without consuming (e.g., to distinguish `=` from `==`, `>` from `>=`)
- **Maximal munch** — always takes the longest valid token (e.g., `>=` not `>` + `=`)
- **Error recovery** — records lex errors and continues scanning; the `errors_` vector collects all problems

#### Example

```
Input:  SET x = 42 + 1
Tokens: [KEYWORD:"SET"] [IDENTIFIER:"x"] [EQUALS:"="]
        [INTEGER:"42"] [OPERATOR:"+"] [INTEGER:"1"] [NEWLINE]
```

---

### Phase 2: Syntax Analysis (Parser)

**Files**: `interpreter/src/parser.cpp`, `parser.h`

The **Parser** takes the flat token stream and builds a tree that reflects the grammatical structure of the program — the **Abstract Syntax Tree (AST)**.

#### Parsing technique: Recursive Descent

MiniLang uses a **hand-written recursive-descent parser** — the most fundamental and transparent parsing technique. Each grammar rule is a separate function:

```
parseStatement()
  +-- parseSetStatement()         SET x = expr
  +-- parsePrintStatement()       PRINT expr
  +-- parseIfStatement()          IF expr ... ELSE ... END
  +-- parseWhileStatement()       WHILE expr ... END

parseExpression()    (entry point for all expressions)
  +-- parseOr()                   OR
      +-- parseAnd()              AND
          +-- parseNot()          NOT (unary)
              +-- parseComparison()     > < >= <= == !=
                  +-- parseArithmetic()    + -
                      +-- parseTerm()         * / %
                          +-- parseUnary()        unary -
                              +-- parseFactor()       literal, var, (expr)
```

The operator precedence is encoded directly in the call hierarchy — lower in the tree = higher precedence.

#### AST Node Types

Defined in `ast.h`:

| Node Class | Represents | Key Fields |
|-----------|-----------|------------|
| `ProgramNode` | Entire program | `statements[]` |
| `AssignNode` | `SET x = expr` | `name`, `value` |
| `PrintNode` | `PRINT expr` | `expression` |
| `IfNode` | `IF...ELSE...END` | `condition`, `thenBranch[]`, `elseBranch[]` |
| `WhileNode` | `WHILE...END` | `condition`, `body[]` |
| `BinaryExprNode` | `left op right` | `op`, `left`, `right` |
| `UnaryExprNode` | `NOT x`, `-x` | `op`, `operand` |
| `LiteralNode` | `42`, `"hi"`, `TRUE` | `value` |
| `VariableNode` | `x` | `name` |

All nodes inherit from `ASTNode` and implement `toJSON()` — so the entire tree can be serialized and displayed in the frontend's AST viewer.

#### Error Recovery

The parser uses **panic mode recovery** via `synchronize()`. When a parse error occurs, it skips tokens until it finds a safe recovery point (a newline, `END`, `ELSE`), then continues parsing.

---

### Phase 3: Semantic Analysis

**Files**: `interpreter/src/semantic.cpp`, `semantic.h`

The **Semantic Analyzer** walks the AST and checks for logical correctness — things the parser cannot catch because they require context.

#### What it checks

- **Undeclared variable use** — catching `PRINT y` when `y` was never `SET`
- **Variable declaration tracking** — maintains a `declaredVars_` set (`std::unordered_set<string>`)
- Visits every node in the AST recursively before execution begins

#### Why this phase matters

The parser only checks *structure* (syntax). The semantic analyzer checks *meaning*:

```
# Parser says: OK (syntactically valid)
# Semantic analyzer says: ERROR -- x is undeclared
PRINT x
```

This is a **static analysis** pass — it runs before any code executes.

---

### Phase 4: AST-Walking Interpreter (Runtime)

**Files**: `interpreter/src/interpreter.cpp`, `interpreter.h`

The **Interpreter** walks the validated AST and executes it. This is an **AST-walking (tree-walking) interpreter**.

#### Execution model

```
execute(ProgramNode)
  +-- for each statement:
        executeStatement(node)
          +-- AssignNode  -> evaluateExpression(value) -> store in symbols_
          +-- PrintNode   -> evaluateExpression(expr)  -> push to output_
          +-- IfNode      -> evaluateExpression(cond)  -> execute branch
          +-- WhileNode   -> loop: evaluateExpression(cond) -> execute body
```

#### Symbol Table

The runtime maintains a flat `std::unordered_map<string, Value>` as its **symbol table**:

```cpp
std::unordered_map<std::string, Value> symbols_;
```

Each `Value` is a tagged union holding one of: `int64_t`, `double`, `std::string`, `bool`.

#### Type system

MiniLang is **dynamically typed** at runtime:

| Operation | Rule |
|-----------|------|
| `int + int` | `int` result |
| `int + float` | `float` result (int promoted) |
| `string + string` | string concatenation |
| `int + string` | Runtime error |
| `x / 0` | Runtime error (division by zero) |

#### Loop safety

A guard (`loopIterations_` counter, max 100,000) prevents infinite loops from hanging the server.

---

## 4. The Backend API Bridge

**File**: `backend/src/index.ts`

The Node.js Express server is a thin, secure bridge between the browser and the C++ binary.

### Endpoint: `POST /api/execute`

```
Request:  { "source": "SET x = 5\nPRINT x" }
Response: { success, output[], tokens[], ast{}, symbols[], errors[], phases{}, executionTimeMs }
```

### Security and Safety

| Protection | How it's implemented |
|-----------|---------------------|
| Source size cap | Rejects requests > 100KB |
| Output size cap | Kills process if stdout > 1MB |
| Execution timeout | 3-second `setTimeout` + `child.kill('SIGKILL')` |
| No shell injection | Uses `child_process.spawn()` with `shell: false` |
| No filesystem access | C++ binary only reads stdin and writes stdout |

### Process lifecycle

```typescript
const child = spawn(INTERPRETER_PATH, [], { shell: false, stdio: ['pipe','pipe','pipe'] });
child.stdin.write(source);   // Feed source code
child.stdin.end();            // Signal EOF
// Collect stdout -> JSON.parse -> send to browser
```

---

## 5. The Frontend Dashboard

**Files**: `frontend/src/App.tsx`, `frontend/src/api.ts`

The React dashboard is a full interactive IDE with four tabs — all powered by **real interpreter output**:

| Tab | Shows |
|-----|-------|
| **Output** | Lines printed by `PRINT` statements |
| **Tokens** | Every token the lexer produced (type, lexeme, line, col) |
| **AST** | Collapsible tree view of the Abstract Syntax Tree |
| **Symbol Table** | All variables with name, type, current value, scope |

The editor uses **CodeMirror 6** with a custom MiniLang syntax highlighter (`minilang-lang.ts`).

---

## 6. Data Flow — End to End

```
1. User types code in CodeMirror editor (browser)
   |
   v
2. React sends POST /api/execute { source: "SET x = 5\nPRINT x" }
   |
   v
3. Express validates (size check) -> spawns minilang.exe
   |
   v
4. minilang.exe receives source via stdin
   |
   +- Lexer:    "SET x = 5\nPRINT x" -> tokens[]
   +- Parser:   tokens[] -> AST
   +- Semantic: AST -> validated (x is declared before PRINT)
   +- Runtime:  executes -> output: ["5"], symbols: [{name:"x", type:"int", value:"5"}]
   |
   v
5. minilang.exe prints JSON to stdout:
   {
     "success": true,
     "output": ["5"],
     "tokens": [...],
     "ast": { "type": "Program", "statements": [...] },
     "symbols": [{ "name": "x", "type": "int", "value": "5", "scope": "global" }],
     "errors": [],
     "phases": { "lexer": "ok", "parser": "ok", "semantic": "ok", "runtime": "ok" },
     "executionTimeMs": 0.82
   }
   |
   v
6. Express JSON.parses stdout -> forwards to browser
   |
   v
7. React renders all four tabs with live data
```

---

## 7. System Programming Syllabus Coverage

---

### Unit 1: Lexical Analysis

**Files**: `lexer.h`, `lexer.cpp`

| SP Concept | MiniLang Implementation |
|-----------|------------------------|
| **Token** | `struct Token { TokenType type; string lexeme; int line; int col; }` |
| **Token types** | `enum class TokenType { INTEGER, FLOAT, STRING, KEYWORD, IDENTIFIER, OPERATOR, ... }` |
| **Lexeme** | Raw matched text stored in `Token::lexeme` |
| **Pattern matching** | Hand-written character-by-character scanning in `scanString()`, `scanNumber()`, `scanIdentifierOrKeyword()` |
| **Finite Automaton (DFA)** | Implicit in the character-scanning logic; `peek()` implements 1-character lookahead |
| **Keyword recognition** | `std::unordered_set<string> KEYWORDS` — O(1) lookup to distinguish `SET` from identifier |
| **Error handling** | Lex errors stored in `errors_` with line/col; scanning continues (error recovery) |
| **Comment handling** | `#` causes the lexer to skip until end of line |

---

### Unit 2: Syntax Analysis (Parsing)

**Files**: `parser.h`, `parser.cpp`

| SP Concept | MiniLang Implementation |
|-----------|------------------------|
| **Context-Free Grammar (CFG)** | MiniLang grammar formally defined in `docs/grammar.md` |
| **Recursive Descent Parsing** | Each grammar rule = one C++ function (`parseStatement`, `parseOr`, `parseAnd`, ...) |
| **LL(1) parsing** | Parser peeks 1 token ahead (`peek()`) to decide which rule to apply — no backtracking |
| **Operator precedence** | Encoded in the call hierarchy: `Or -> And -> Not -> Comparison -> Arithmetic -> Term -> Unary -> Factor` |
| **Abstract Syntax Tree** | `ASTNode` hierarchy (`ProgramNode`, `AssignNode`, `IfNode`, `WhileNode`, `BinaryExprNode`, ...) |
| **Error recovery** | `synchronize()` implements panic-mode recovery — skips to next safe token |
| **Block parsing** | `parseBlock()` reads statements until a terminator keyword (`END`, `ELSE`) |

---

### Unit 3: Semantic Analysis

**Files**: `semantic.h`, `semantic.cpp`

| SP Concept | MiniLang Implementation |
|-----------|------------------------|
| **Symbol table (analysis)** | `std::unordered_set<string> declaredVars_` — tracks all declared variable names |
| **Declaration before use** | Checks every `VariableNode` reference against `declaredVars_`; error if not found |
| **Scope analysis** | Single flat (global) scope — all variables visible throughout the program |
| **Static analysis** | Runs *before* execution — finds errors at "compile time" not "runtime" |
| **Tree traversal** | `analyzeNode()` walks the AST recursively, dispatching to `analyzeStatement()` / `analyzeExpression()` |

---

### Unit 4: Symbol Table

**File**: `interpreter.h` (runtime symbol table)

| SP Concept | MiniLang Implementation |
|-----------|------------------------|
| **Symbol table structure** | `std::unordered_map<string, Value> symbols_` — hash map for O(1) lookup |
| **Symbol entry** | `struct SymbolEntry { string name; string type; string value; string scope; }` |
| **Type information** | `Value::type` — tracks `Int`, `Float`, `String`, `Bool` per variable |
| **Symbol table display** | Exported in JSON output; rendered in the Symbol Table dashboard tab |
| **Scope** | All symbols are `"global"` scope in this implementation |

---

### Unit 5: Intermediate Code & Runtime Execution

**Files**: `interpreter.h`, `interpreter.cpp`

| SP Concept | MiniLang Implementation |
|-----------|------------------------|
| **Intermediate representation** | The AST itself serves as the IR — no bytecode compiled |
| **AST-walking interpreter** | `execute()` -> `executeStatement()` -> `evaluateExpression()` recursive traversal |
| **Expression evaluation** | `evalBinary()` handles all binary ops; `evalUnary()` handles `NOT`, unary `-` |
| **Control flow** | `IfNode` -> conditional branch; `WhileNode` -> loop with condition re-evaluation |
| **Type coercion** | `int` promoted to `float` in mixed arithmetic; string concatenation via `+` |
| **Runtime error handling** | `InterpreterException` thrown on division by zero, type mismatch, etc. |
| **Loop safety** | `loopIterations_` counter with `MAX_LOOP_ITERATIONS = 100000` |

---

### Unit 6: Compiler/Interpreter Phases & Pipeline

**File**: `main.cpp`

| SP Concept | MiniLang Implementation |
|-----------|------------------------|
| **Multi-phase pipeline** | `main.cpp` orchestrates Lexer -> Parser -> Semantic -> Runtime in sequence |
| **Phase status reporting** | `phases: { lexer, parser, semantic, runtime }` — each shows `"ok"`, `"error"`, or `"skipped"` |
| **Error phase attribution** | Every error carries a `phase` field so users know exactly where it failed |
| **Execution timing** | `std::chrono::steady_clock` measures and reports total execution time in ms |
| **JSON output protocol** | Interpreter outputs a structured JSON object — a clean API contract |
| **stdin/stdout IPC** | Source via stdin, result via stdout — Unix inter-process communication |

---

### Unit 7: Process Management & System Calls (Backend)

**File**: `backend/src/index.ts`

| SP Concept | MiniLang Implementation |
|-----------|------------------------|
| **Process creation** | `child_process.spawn()` creates a new OS process for the interpreter |
| **IPC (Inter-Process Communication)** | `stdin`/`stdout` pipes between Node.js and the C++ child process |
| **Process isolation** | `shell: false` — C++ process cannot access the filesystem |
| **Process termination** | `child.kill('SIGKILL')` on timeout — forceful termination |
| **Resource limits** | 3-second timeout, 100KB input cap, 1MB output cap |
| **Exit codes** | `child.on('close', code)` handles normal exit vs killed process |
| **stdio redirection** | `stdio: ['pipe', 'pipe', 'pipe']` — all three streams piped and controlled |

---

### Unit 8: Data Structures in System Software

| SP Concept | MiniLang Data Structure | Used In |
|-----------|------------------------|---------|
| **Hash map** | `std::unordered_map<string, Value>` | Runtime symbol table |
| **Hash set** | `std::unordered_set<string>` | Keyword lookup, declared vars |
| **Dynamic array** | `std::vector<Token>`, `std::vector<ASTNodePtr>` | Token stream, AST children |
| **Shared pointer tree** | `std::shared_ptr<ASTNode>` (ASTNodePtr) | AST memory management |
| **Stack (implicit)** | C++ call stack during recursive descent | Parser, interpreter tree walk |
| **Tagged union** | `struct Value { ValueType type; int64_t; double; string; bool; }` | Runtime values |

---

### Unit 9: Error Handling & Reporting

| Error Type | Phase | Example | Recovery |
|-----------|-------|---------|----------|
| **Lex error** | Lexer | Unterminated string `"hello` | Skip to next line; continue |
| **Parse error** | Parser | Missing `END` | Panic mode: skip to safe token |
| **Semantic error** | Semantic | Using undeclared variable | Collect all; block runtime |
| **Runtime error** | Interpreter | `SET x = 5 / 0` | Throw `InterpreterException` |
| **Timeout error** | Backend API | Infinite loop | SIGKILL after 3 seconds |

All errors include `{ phase, type, message, line, col }` — exact location for the user.

---

## 8. MiniLang Language Reference

### Statements

```
SET <variable> = <expression>      # Assignment
PRINT <expression>                 # Print to output
IF <expression>                    # Conditional
  ...statements...
ELSE                               # Optional
  ...statements...
END
WHILE <expression>                 # Loop
  ...statements...
END
```

### Types

| Type | Examples | Notes |
|------|---------|-------|
| `int` | `42`, `-7`, `0` | 64-bit signed integer |
| `float` | `3.14`, `-2.5` | 64-bit double |
| `string` | `"hello"`, `"world"` | Double-quoted |
| `bool` | `TRUE`, `FALSE` | Keywords |

### Operators (by precedence, lowest first)

| Level | Operators | Description |
|-------|-----------|-------------|
| 1 | `OR` | Logical or |
| 2 | `AND` | Logical and |
| 3 | `NOT` | Logical not (unary) |
| 4 | `> < >= <= == !=` | Comparison |
| 5 | `+ -` | Addition, subtraction |
| 6 | `* / %` | Multiplication, division, modulo |
| 7 | unary `-` | Negation |
| 8 | `( )`, literals, variables | Atoms |

### Comments

```
# This is a comment -- everything after # is ignored
```

### Example Programs

**Fibonacci sequence:**
```
SET a = 0
SET b = 1
SET i = 0
WHILE i < 10
  PRINT a
  SET temp = a + b
  SET a = b
  SET b = temp
  SET i = i + 1
END
```

**FizzBuzz:**
```
SET i = 1
WHILE i <= 20
  IF i % 15 == 0
    PRINT "FizzBuzz"
  ELSE
    IF i % 3 == 0
      PRINT "Fizz"
    ELSE
      IF i % 5 == 0
        PRINT "Buzz"
      ELSE
        PRINT i
      END
    END
  END
  SET i = i + 1
END
```

---

## 9. File Structure

```
sp/
+-- interpreter/                  # C++17 -- the interpreter
|   +-- src/
|   |   +-- token.h / token.cpp   # Token definitions & keyword set
|   |   +-- lexer.h / lexer.cpp   # Lexical analyzer (Phase 1)
|   |   +-- ast.h / ast.cpp       # AST node classes + Value type
|   |   +-- parser.h / parser.cpp # Recursive-descent parser (Phase 2)
|   |   +-- semantic.h / semantic.cpp  # Semantic analyzer (Phase 3)
|   |   +-- interpreter.h / interpreter.cpp  # Tree-walking interpreter (Phase 4)
|   |   +-- json.h / json.cpp     # JSON string escaping utility
|   |   +-- main.cpp              # Pipeline orchestrator & JSON output
|   +-- tests/
|   |   +-- test_main.cpp         # 38 unit tests (lexer, parser, runtime)
|   +-- build.bat                 # Build script (g++ / MinGW)
|
+-- backend/                      # Node.js -- the API bridge
|   +-- src/
|   |   +-- index.ts              # Express server, process spawning
|   +-- package.json
|   +-- tsconfig.json
|
+-- frontend/                     # React -- the dashboard
|   +-- src/
|   |   +-- App.tsx               # Main IDE component, all tabs
|   |   +-- api.ts                # HTTP client for /api/execute
|   |   +-- types.ts              # TypeScript interfaces matching C++ JSON
|   |   +-- examples.ts           # Built-in example programs
|   |   +-- minilang-lang.ts      # CodeMirror 6 syntax highlighter
|   |   +-- index.css             # Dark theme, CSS design tokens
|   |   +-- main.tsx              # React entry point
|   +-- package.json
|   +-- vite.config.ts
|
+-- docs/
|   +-- grammar.md                # Formal BNF grammar of MiniLang
|   +-- project-explanation.md   # This document
|
+-- README.md                     # Setup, build, usage instructions
```

---

*MiniLang was built entirely from scratch -- no parser generators (yacc/bison/ANTLR), no lexer generators (flex/lex), no interpreter libraries. Every algorithm from character scanning to AST evaluation was hand-coded as a direct application of System Programming theory.*
