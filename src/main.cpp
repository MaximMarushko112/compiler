#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#include <Tokenization/Tokenizer.hpp>
#include <Parsing/Parser.hpp>
#include <Parsing/PrintAST.hpp>
#include <Visitors/ScopeVisitor.hpp>
#include <Visitors/Interpreter.hpp>


void runTest(const std::string& source, const std::string& testName) {
    std::cout << "\n========== " << testName << " ==========\n";
    std::cout << "Source code:\n" << source << "\n";

    try {
        std::cout << "Tokens:\n";
        auto tokens = Tokenization::Tokenizer::tokenize(source);
        for (const auto& tok : tokens) {
            std::visit([](const auto& t) { std::cout << "  " << t << "\n"; }, tok.token);
        }
        
        Parsing::Parser parser;
        Parsing::TranslationUnit ast = parser.parse(tokens);

        if (!parser.getErrors().empty()) {
            std::cout << "Parsing errors:\n";
            for (const auto& err : parser.getErrors())
                std::cout << "  " << err << "\n";
            return;
        }

        std::cout << "AST:\n";
        Parsing::PrintVisitor printVisitor(std::cout);
        printVisitor.print(ast);

        // ---------- ScopeVisitor ----------
        std::cout << "\n--- Scope analysis ---\n";
        Parsing::ScopeVisitor scopeVisitor;
        scopeVisitor.build(ast);
        scopeVisitor.printErrors();

        // ---------- Interpreter ----------
        std::cout << "\n--- Interpreter execution ---\n";
        Parsing::Interpreter interpreter;
        int result = interpreter.run(ast);
        std::cout << "Interpreter result: " << result << std::endl;

    } catch (const std::runtime_error& e) {
        std::cout << "Error: " << e.what() << "\n";
    }
}


int main() {
    // Пример 1: функция с множественным возвратом и множественное присваивание
    std::string test1 = R"(
        int, string divide(int a, int b) {
            if (b == 0) return 0, "error";
            return a / b, "ok";
        }
        int main() {
            int q;
            string msg;
            q, msg = divide(10, 3);
            return 0;
        }
    )";
    runTest(test1, "Multiple return and multi-assign");

    // Пример 2: структура и перечисление
    std::string test2 = R"(
        struct Point {
            int x;
            int y;
        };
        enum Color {
            RED,
            GREEN,
            BLUE = 100
        };
        int main() {
            struct Point p = {10, 20};
            enum Color c = GREEN;
            return 0;
        }
    )";
    runTest(test2, "Struct and enum");

    // Пример 3: циклы и условные операторы
    std::string test3 = R"(
        int sum(int n) {
            int s = 0;
            for (int i = 1; i <= n; ++i) {
                s = s + i;
            }
            return s;
        }
    )";
    runTest(test3, "Loops and conditionals");

    // Пример 4: variadic функция
    std::string test4 = R"(
        int sum(int count, ...) {
            int total = 0;
            // реализация с va_list
            return total;
        }
    )";
    runTest(test4, "Variadic function");

    // Пример 5: факториал (интерпретатор)
    std::string test5 = R"(
        int main() {
            int n = 5;
            int f = 1;
            for (int i = 2; i <= n; ++i) {
                f = f * i;
            }
            print(f);
            return f;
        }
    )";
    runTest(test5, "Factorial with interpreter");

    // Пример 6: проверка областей видимости (ошибки)
    std::string test6 = R"(
        int main() {
            int x = 10;
            int x = 20;   // ошибка: повторное объявление
            int y = x + z; // ошибка: z не объявлена
            return 0;
        }
    )";
    runTest(test6, "Scope errors (redeclaration and undeclared variable)");

    // Пример 7: интерпретатор с if-else и вложенными блоками
    std::string test7 = R"(
        int main() {
            int a = 5;
            int b = 3;
            if (a > b) {
                int c = a - b;
                print(c);
                return c;
            } else {
                return 0;
            }
        }
    )";
    runTest(test7, "If-else and nested block");

    return 0;
}