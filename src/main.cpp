#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#include <Tokenization/Tokenizer.hpp>
#include <Parsing/Parser.hpp>
#include <Parsing/PrintAST.hpp>

// Вспомогательная функция для запуска теста
void runTest(const std::string& source, const std::string& testName) {
    std::cout << "\n========== " << testName << " ==========\n";
    std::cout << "Source code:\n" << source << "\n";

    try {
        std::cout << "tokens" << '\n';
        // Токенизация
        auto tokens = Tokenization::Tokenizer::tokenize(source);

        for (const auto& tok : tokens) {
            std::visit([](const auto& t) { std::cout << "  " << t << "\n"; }, tok.token);
        }
        
        // Парсинг
        auto parsingInfo = Parsing::Parser::parse(tokens);

        if (!parsingInfo.errors.empty()) {
            std::cout << "Parsing errors:\n";
            for (const auto& err : parsingInfo.errors)
                std::cout << "  " << err << "\n";
            return;
        }

        // Успех – выводим AST
        std::cout << "AST:\n";
        Parsing::printAST(parsingInfo.ast, std::cout);
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

    return 0;
}