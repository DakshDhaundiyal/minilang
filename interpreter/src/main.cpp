#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "interpreter.h"
#include "json.h"
#include <iostream>
#include <sstream>
#include <string>
#include <chrono>
#include <algorithm>

int main() {
    // Read all of stdin
    std::ostringstream ss;
    ss << std::cin.rdbuf();
    std::string source = ss.str();

    auto startTime = std::chrono::steady_clock::now();

    std::string phLexer = "ok", phParser = "skipped", phSemantic = "skipped", phRuntime = "skipped";
    std::vector<Token> tokens;
    ASTNodePtr ast = nullptr;
    std::vector<SymbolEntry> symbols;
    std::vector<std::string> output;
    bool success = true;

    // Collect all errors with phase info
    struct PhasedError {
        std::string phase;
        std::string type;
        std::string message;
        int line;
        int col;
    };
    std::vector<PhasedError> allErrors;

    // --- LEXER ---
    Lexer lexer(source);
    tokens = lexer.tokenize();

    if (lexer.hasErrors()) {
        phLexer = "error";
        success = false;
        for (const auto& e : lexer.getErrors()) {
            allErrors.push_back({"lexer", e.type, e.message, e.line, e.col});
        }
    }

    // --- PARSER ---
    if (phLexer == "ok" || phLexer == "error") {
        // We still attempt parsing even with lex errors (error recovery)
        // But only if lexer didn't produce catastrophic errors
        if (phLexer == "ok") {
            phParser = "ok";
            Parser parser(tokens);
            ast = parser.parse();
            if (parser.hasErrors()) {
                phParser = "error";
                success = false;
                for (const auto& e : parser.getErrors()) {
                    allErrors.push_back({"parser", e.type, e.message, e.line, e.col});
                }
            }
        } else {
            // Lex errors → skip parsing
            phParser = "skipped";
        }
    }

    // --- SEMANTIC ---
    if (phParser == "ok" && ast) {
        phSemantic = "ok";
        SemanticAnalyzer semantic;
        semantic.analyze(ast);
        if (semantic.hasErrors()) {
            phSemantic = "error";
            success = false;
            for (const auto& e : semantic.getErrors()) {
                allErrors.push_back({"semantic", e.type, e.message, e.line, e.col});
            }
        }
    }

    // --- INTERPRETER ---
    if (phSemantic == "ok" && ast) {
        phRuntime = "ok";
        MiniLangInterpreter interp;
        interp.execute(ast);
        output = interp.getOutput();
        symbols = interp.getSymbols();
        if (interp.hasErrors()) {
            phRuntime = "error";
            success = false;
            for (const auto& e : interp.getErrors()) {
                allErrors.push_back({"runtime", e.type, e.message, e.line, e.col});
            }
        }
    }

    auto endTime = std::chrono::steady_clock::now();
    double executionTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    // --- OUTPUT JSON ---
    std::cout << "{\n";
    std::cout << "  \"success\": " << (success ? "true" : "false") << ",\n";

    // Output
    std::cout << "  \"output\": [";
    for (size_t i = 0; i < output.size(); i++) {
        std::cout << jsonEscape(output[i]);
        if (i + 1 < output.size()) std::cout << ", ";
    }
    std::cout << "],\n";

    // Tokens (filter out comments and errors for cleanliness, but include them)
    std::cout << "  \"tokens\": [\n";
    for (size_t i = 0; i < tokens.size(); i++) {
        const auto& t = tokens[i];
        std::cout << "    {\"index\": " << t.index
                  << ", \"type\": " << jsonEscape(t.typeString())
                  << ", \"lexeme\": " << jsonEscape(t.lexeme)
                  << ", \"line\": " << t.line
                  << ", \"col\": " << t.col << "}";
        if (i + 1 < tokens.size()) std::cout << ",";
        std::cout << "\n";
    }
    std::cout << "  ],\n";

    // AST
    std::cout << "  \"ast\": ";
    if (ast) {
        std::cout << ast->toJSON(1);
    } else {
        std::cout << "null";
    }
    std::cout << ",\n";

    // Symbols
    std::cout << "  \"symbols\": [\n";
    for (size_t i = 0; i < symbols.size(); i++) {
        const auto& s = symbols[i];
        std::cout << "    {\"name\": " << jsonEscape(s.name)
                  << ", \"type\": " << jsonEscape(s.type)
                  << ", \"value\": " << jsonEscape(s.value)
                  << ", \"scope\": " << jsonEscape(s.scope) << "}";
        if (i + 1 < symbols.size()) std::cout << ",";
        std::cout << "\n";
    }
    std::cout << "  ],\n";

    // Errors
    std::cout << "  \"errors\": [\n";
    for (size_t i = 0; i < allErrors.size(); i++) {
        const auto& e = allErrors[i];
        std::cout << "    {\"phase\": " << jsonEscape(e.phase)
                  << ", \"type\": " << jsonEscape(e.type)
                  << ", \"message\": " << jsonEscape(e.message)
                  << ", \"line\": " << e.line
                  << ", \"col\": " << e.col << "}";
        if (i + 1 < allErrors.size()) std::cout << ",";
        std::cout << "\n";
    }
    std::cout << "  ],\n";

    // Phases
    std::cout << "  \"phases\": {"
              << "\"lexer\": " << jsonEscape(phLexer) << ", "
              << "\"parser\": " << jsonEscape(phParser) << ", "
              << "\"semantic\": " << jsonEscape(phSemantic) << ", "
              << "\"runtime\": " << jsonEscape(phRuntime)
              << "},\n";

    // Execution time
    std::cout << "  \"executionTimeMs\": " << executionTimeMs << "\n";

    std::cout << "}\n";

    return 0;
}
