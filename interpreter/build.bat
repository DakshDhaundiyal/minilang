@echo off
echo Building MiniLang interpreter...

if not exist build mkdir build

echo Compiling library sources...
g++ -std=c++17 -O2 -c -I src src/token.cpp -o build/token.o
g++ -std=c++17 -O2 -c -I src src/json.cpp -o build/json.o
g++ -std=c++17 -O2 -c -I src src/lexer.cpp -o build/lexer.o
g++ -std=c++17 -O2 -c -I src src/ast.cpp -o build/ast.o
g++ -std=c++17 -O2 -c -I src src/parser.cpp -o build/parser.o
g++ -std=c++17 -O2 -c -I src src/semantic.cpp -o build/semantic.o
g++ -std=c++17 -O2 -c -I src src/interpreter.cpp -o build/interpreter.o

echo Linking main executable...
g++ -std=c++17 -O2 -c -I src src/main.cpp -o build/main.o
g++ build/token.o build/json.o build/lexer.o build/ast.o build/parser.o build/semantic.o build/interpreter.o build/main.o -o build/minilang.exe -static

echo Building tests...
g++ -std=c++17 -O2 -c -I src tests/test_main.cpp -o build/test_main.o
g++ build/token.o build/json.o build/lexer.o build/ast.o build/parser.o build/semantic.o build/interpreter.o build/test_main.o -o build/minilang_tests.exe -static

echo Done! Binaries in build/
