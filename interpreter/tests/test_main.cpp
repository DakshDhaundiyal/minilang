#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "interpreter.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <string>
#include <vector>
#include <sstream>
#include <functional>

static int testsPassed = 0;
static int testsFailed = 0;

static void runTest(const std::string& name, std::function<void()> fn) {
    std::cout << "  Running: " << name << "... " << std::flush;
    try {
        fn();
        std::cout << "PASSED" << std::endl;
        testsPassed++;
    } catch (const std::exception& e) {
        std::cout << "FAILED: " << e.what() << std::endl;
        testsFailed++;
    } catch (...) {
        std::cout << "FAILED (unknown exception)" << std::endl;
        testsFailed++;
    }
}

#define ASSERT(cond) \
    if (!(cond)) throw std::runtime_error(std::string("Assertion failed: ") + #cond + " at line " + std::to_string(__LINE__))

#define ASSERT_EQ(a, b) \
    if (std::string(a) != std::string(b)) throw std::runtime_error(std::string("Expected '") + std::string(b) + "' but got '" + std::string(a) + "' at line " + std::to_string(__LINE__))

// Helper: run full pipeline and return results
struct RunResult {
    std::vector<Token> tokens;
    ASTNodePtr ast;
    std::vector<std::string> output;
    std::vector<SymbolEntry> symbols;
    bool lexerOk;
    bool parserOk;
    bool semanticOk;
    bool runtimeOk;
    std::vector<std::string> runtimeErrors;
};

RunResult runSource(const std::string& source) {
    RunResult r;
    Lexer lexer(source);
    r.tokens = lexer.tokenize();
    r.lexerOk = !lexer.hasErrors();

    if (r.lexerOk) {
        Parser parser(r.tokens);
        r.ast = parser.parse();
        r.parserOk = !parser.hasErrors();
    } else {
        r.parserOk = false;
    }

    if (r.parserOk && r.ast) {
        SemanticAnalyzer sem;
        sem.analyze(r.ast);
        r.semanticOk = !sem.hasErrors();
    } else {
        r.semanticOk = false;
    }

    if (r.semanticOk && r.ast) {
        MiniLangInterpreter interp;
        interp.execute(r.ast);
        r.output = interp.getOutput();
        r.symbols = interp.getSymbols();
        r.runtimeOk = !interp.hasErrors();
        for (const auto& e : interp.getErrors()) {
            r.runtimeErrors.push_back(e.message);
        }
    } else {
        r.runtimeOk = false;
    }

    return r;
}

int main() {
    std::cout << "\n=== MiniLang Test Suite ===" << std::endl;

    // ==================== LEXER TESTS ====================
    std::cout << "\n--- Lexer Tests ---" << std::endl;

    runTest("lexer_integer", []() {
        Lexer lex("42\n");
        auto tokens = lex.tokenize();
        ASSERT(!lex.hasErrors());
        ASSERT(tokens.size() >= 2);
        ASSERT(tokens[0].type == TokenType::INTEGER);
        ASSERT_EQ(tokens[0].lexeme, "42");
    });

    runTest("lexer_float", []() {
        Lexer lex("3.14\n");
        auto tokens = lex.tokenize();
        ASSERT(!lex.hasErrors());
        ASSERT(tokens[0].type == TokenType::FLOAT);
        ASSERT_EQ(tokens[0].lexeme, "3.14");
    });

    runTest("lexer_string", []() {
        Lexer lex("\"hello world\"\n");
        auto tokens = lex.tokenize();
        ASSERT(!lex.hasErrors());
        ASSERT(tokens[0].type == TokenType::STRING);
        ASSERT_EQ(tokens[0].lexeme, "\"hello world\"");
    });

    runTest("lexer_identifier", []() {
        Lexer lex("myVar\n");
        auto tokens = lex.tokenize();
        ASSERT(!lex.hasErrors());
        ASSERT(tokens[0].type == TokenType::IDENTIFIER);
        ASSERT_EQ(tokens[0].lexeme, "myVar");
    });

    runTest("lexer_keywords", []() {
        Lexer lex("SET PRINT IF ELSE END WHILE AND OR NOT TRUE FALSE\n");
        auto tokens = lex.tokenize();
        ASSERT(!lex.hasErrors());
        for (int i = 0; i < 11; i++) {
            ASSERT(tokens[i].type == TokenType::KEYWORD);
        }
    });

    runTest("lexer_operators", []() {
        Lexer lex("+ - * / % > < >= <= == !=\n");
        auto tokens = lex.tokenize();
        ASSERT(!lex.hasErrors());
        ASSERT_EQ(tokens[0].lexeme, "+");
        ASSERT_EQ(tokens[1].lexeme, "-");
        ASSERT_EQ(tokens[2].lexeme, "*");
        ASSERT_EQ(tokens[3].lexeme, "/");
        ASSERT_EQ(tokens[4].lexeme, "%");
        ASSERT_EQ(tokens[5].lexeme, ">");
        ASSERT_EQ(tokens[6].lexeme, "<");
        ASSERT_EQ(tokens[7].lexeme, ">=");
        ASSERT_EQ(tokens[8].lexeme, "<=");
        ASSERT_EQ(tokens[9].lexeme, "==");
        ASSERT_EQ(tokens[10].lexeme, "!=");
    });

    runTest("lexer_comment", []() {
        Lexer lex("# this is a comment\n");
        auto tokens = lex.tokenize();
        ASSERT(!lex.hasErrors());
        ASSERT(tokens[0].type == TokenType::COMMENT);
    });

    runTest("lexer_parens", []() {
        Lexer lex("( )\n");
        auto tokens = lex.tokenize();
        ASSERT(!lex.hasErrors());
        ASSERT(tokens[0].type == TokenType::LPAREN);
        ASSERT(tokens[1].type == TokenType::RPAREN);
    });

    runTest("lexer_invalid_char", []() {
        Lexer lex("@\n");
        auto tokens = lex.tokenize();
        ASSERT(lex.hasErrors());
        ASSERT_EQ(lex.getErrors()[0].type, "invalid_character");
    });

    runTest("lexer_malformed_number_trailing_dot", []() {
        Lexer lex("1.\n");
        auto tokens = lex.tokenize();
        ASSERT(lex.hasErrors());
        ASSERT_EQ(lex.getErrors()[0].type, "malformed_number");
    });

    runTest("lexer_malformed_number_double_dot", []() {
        Lexer lex("1.2.3\n");
        auto tokens = lex.tokenize();
        ASSERT(lex.hasErrors());
    });

    runTest("lexer_malformed_number_alpha", []() {
        Lexer lex("12abc\n");
        auto tokens = lex.tokenize();
        ASSERT(lex.hasErrors());
    });

    runTest("lexer_unterminated_string", []() {
        Lexer lex("\"hello\n");
        auto tokens = lex.tokenize();
        ASSERT(lex.hasErrors());
        ASSERT_EQ(lex.getErrors()[0].type, "unterminated_string");
    });

    runTest("lexer_line_col_tracking", []() {
        Lexer lex("SET x = 5\nPRINT x\n");
        auto tokens = lex.tokenize();
        ASSERT(tokens[0].line == 1);
        ASSERT(tokens[0].col == 1);
        bool found = false;
        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i].lexeme == "PRINT") {
                ASSERT(tokens[i].line == 2);
                found = true;
                break;
            }
        }
        ASSERT(found);
    });

    // ==================== PARSER TESTS ====================
    std::cout << "\n--- Parser Tests ---" << std::endl;

    runTest("parser_set_statement", []() {
        auto r = runSource("SET x = 42\n");
        ASSERT(r.parserOk);
        ASSERT(r.ast != nullptr);
        auto prog = std::dynamic_pointer_cast<ProgramNode>(r.ast);
        ASSERT(prog->statements.size() == 1);
        ASSERT(prog->statements[0]->nodeType == NodeType::Assign);
    });

    runTest("parser_print_statement", []() {
        auto r = runSource("PRINT 42\n");
        ASSERT(r.parserOk);
        auto prog = std::dynamic_pointer_cast<ProgramNode>(r.ast);
        ASSERT(prog->statements[0]->nodeType == NodeType::Print);
    });

    runTest("parser_if_else", []() {
        auto r = runSource("IF TRUE\nPRINT 1\nELSE\nPRINT 2\nEND\n");
        ASSERT(r.parserOk);
        auto prog = std::dynamic_pointer_cast<ProgramNode>(r.ast);
        ASSERT(prog->statements[0]->nodeType == NodeType::If);
        auto ifNode = std::dynamic_pointer_cast<IfNode>(prog->statements[0]);
        ASSERT(ifNode->thenBranch.size() == 1);
        ASSERT(ifNode->elseBranch.size() == 1);
    });

    runTest("parser_while", []() {
        auto r = runSource("SET x = 0\nWHILE x < 3\nSET x = x + 1\nEND\n");
        ASSERT(r.parserOk);
        auto prog = std::dynamic_pointer_cast<ProgramNode>(r.ast);
        ASSERT(prog->statements[1]->nodeType == NodeType::While);
    });

    runTest("parser_precedence", []() {
        auto r = runSource("PRINT 2 + 3 * 4\n");
        ASSERT(r.parserOk);
        ASSERT(r.output.size() == 1);
        ASSERT_EQ(r.output[0], "14");
    });

    runTest("parser_missing_end", []() {
        Lexer lex("IF TRUE\nPRINT 1\n");
        auto tokens = lex.tokenize();
        Parser parser(tokens);
        parser.parse();
        ASSERT(parser.hasErrors());
        bool foundMissingEnd = false;
        for (size_t i = 0; i < parser.getErrors().size(); i++) {
            if (parser.getErrors()[i].type == "missing_end") foundMissingEnd = true;
        }
        ASSERT(foundMissingEnd);
    });

    runTest("parser_malformed_expression", []() {
        Lexer lex("PRINT +\n");
        auto tokens = lex.tokenize();
        Parser parser(tokens);
        parser.parse();
        ASSERT(parser.hasErrors());
    });

    // ==================== INTERPRETER TESTS ====================
    std::cout << "\n--- Interpreter Tests ---" << std::endl;

    runTest("interp_arithmetic_int", []() {
        auto r = runSource("PRINT 2 + 3\nPRINT 10 - 4\nPRINT 3 * 5\nPRINT 10 / 3\nPRINT 10 % 3\n");
        ASSERT(r.runtimeOk);
        ASSERT(r.output.size() == 5);
        ASSERT_EQ(r.output[0], "5");
        ASSERT_EQ(r.output[1], "6");
        ASSERT_EQ(r.output[2], "15");
        ASSERT_EQ(r.output[3], "3");
        ASSERT_EQ(r.output[4], "1");
    });

    runTest("interp_arithmetic_float", []() {
        auto r = runSource("PRINT 10.0 / 3.0\n");
        ASSERT(r.runtimeOk);
        ASSERT(r.output.size() == 1);
        double val = std::stod(r.output[0]);
        ASSERT(std::abs(val - 3.33333) < 0.001);
    });

    runTest("interp_strings", []() {
        auto r = runSource("PRINT \"hello\" + \" \" + \"world\"\n");
        ASSERT(r.runtimeOk);
        ASSERT_EQ(r.output[0], "hello world");
    });

    runTest("interp_comparisons", []() {
        auto r = runSource("PRINT 5 > 3\nPRINT 3 >= 3\nPRINT 2 < 1\n");
        ASSERT(r.runtimeOk);
        ASSERT_EQ(r.output[0], "true");
        ASSERT_EQ(r.output[1], "true");
        ASSERT_EQ(r.output[2], "false");
    });

    runTest("interp_booleans", []() {
        auto r = runSource("PRINT TRUE AND FALSE\nPRINT TRUE OR FALSE\nPRINT NOT TRUE\n");
        ASSERT(r.runtimeOk);
        ASSERT_EQ(r.output[0], "false");
        ASSERT_EQ(r.output[1], "true");
        ASSERT_EQ(r.output[2], "false");
    });

    runTest("interp_variables", []() {
        auto r = runSource("SET x = 42\nPRINT x\n");
        ASSERT(r.runtimeOk);
        ASSERT_EQ(r.output[0], "42");
        ASSERT(r.symbols.size() >= 1);
    });

    runTest("interp_loop", []() {
        auto r = runSource("SET sum = 0\nSET i = 1\nWHILE i <= 5\nSET sum = sum + i\nSET i = i + 1\nEND\nPRINT sum\n");
        ASSERT(r.runtimeOk);
        ASSERT_EQ(r.output[0], "15");
    });

    runTest("interp_undefined_var", []() {
        auto r = runSource("PRINT x\n");
        ASSERT(!r.semanticOk);
    });

    runTest("interp_div_by_zero", []() {
        auto r = runSource("PRINT 10 / 0\n");
        ASSERT(!r.runtimeOk);
        ASSERT(r.runtimeErrors.size() >= 1);
    });

    runTest("interp_modulo_by_zero", []() {
        auto r = runSource("PRINT 10 % 0\n");
        ASSERT(!r.runtimeOk);
    });

    runTest("interp_execution_limit", []() {
        auto r = runSource("SET x = TRUE\nWHILE x\nSET x = TRUE\nEND\n");
        ASSERT(!r.runtimeOk);
        bool found = false;
        for (size_t i = 0; i < r.runtimeErrors.size(); i++) {
            if (r.runtimeErrors[i].find("limit") != std::string::npos ||
                r.runtimeErrors[i].find("Execution") != std::string::npos) found = true;
        }
        ASSERT(found);
    });

    runTest("interp_if_condition_not_bool", []() {
        auto r = runSource("IF 42\nPRINT 1\nEND\n");
        ASSERT(!r.runtimeOk);
    });

    runTest("interp_empty_program", []() {
        auto r = runSource("");
        ASSERT(r.lexerOk);
        ASSERT(r.parserOk);
        ASSERT(r.output.empty());
    });

    runTest("interp_nested_if", []() {
        auto r = runSource("SET x = 10\nIF x > 5\nIF x > 8\nPRINT \"big\"\nEND\nEND\n");
        ASSERT(r.runtimeOk);
        ASSERT_EQ(r.output[0], "big");
    });

    runTest("interp_unary_minus", []() {
        auto r = runSource("PRINT -5\nPRINT -(3 + 2)\n");
        ASSERT(r.runtimeOk);
        ASSERT_EQ(r.output[0], "-5");
        ASSERT_EQ(r.output[1], "-5");
    });

    runTest("interp_string_equality", []() {
        auto r = runSource("PRINT \"abc\" == \"abc\"\nPRINT \"abc\" != \"def\"\n");
        ASSERT(r.runtimeOk);
        ASSERT_EQ(r.output[0], "true");
        ASSERT_EQ(r.output[1], "true");
    });

    runTest("interp_reassign_type", []() {
        auto r = runSource("SET x = 42\nSET x = \"hello\"\nPRINT x\n");
        ASSERT(r.runtimeOk);
        ASSERT_EQ(r.output[0], "hello");
    });

    // ==================== RESULTS ====================
    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Passed: " << testsPassed << std::endl;
    std::cout << "Failed: " << testsFailed << std::endl;
    std::cout << "Total:  " << (testsPassed + testsFailed) << std::endl;

    return testsFailed > 0 ? 1 : 0;
}
