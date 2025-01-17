// Copyright 2024 solar-mist

#include "Test.h"

#include "parser/Parser.h"

#include "parser/ast/expression/BooleanLiteral.h"

#define CREATE_PARSER_INTROSPECTOR(MemberName) auto Get##Parser##_##MemberName () { return &parser::Parser::MemberName; }
#define CREATE_PARSER_OBJECT_INTROSPECTOR_REF(NodeType, MemberName) auto& Get##NodeType##_##MemberName (parser:: NodeType* n) { return n-> MemberName; }

struct ASTNodeIntrospector
{
    CREATE_PARSER_INTROSPECTOR(parseType);
    CREATE_PARSER_INTROSPECTOR(parseExpression);
    CREATE_PARSER_INTROSPECTOR(parsePrimary);
    CREATE_PARSER_INTROSPECTOR(parseFunction);
    CREATE_PARSER_INTROSPECTOR(parseReturnStatement);
    CREATE_PARSER_INTROSPECTOR(parseVariableDeclaration);
    CREATE_PARSER_INTROSPECTOR(parseIfStatement);
    CREATE_PARSER_INTROSPECTOR(parseIntegerLiteral);
    CREATE_PARSER_INTROSPECTOR(parseVariableExpression);
    CREATE_PARSER_INTROSPECTOR(parseCallExpression);

    
    CREATE_PARSER_OBJECT_INTROSPECTOR_REF(ASTNode, mType);
    CREATE_PARSER_OBJECT_INTROSPECTOR_REF(ASTNode, mScope);

    CREATE_PARSER_OBJECT_INTROSPECTOR_REF(Function, mName);
    CREATE_PARSER_OBJECT_INTROSPECTOR_REF(Function, mBody);
    CREATE_PARSER_OBJECT_INTROSPECTOR_REF(Function, mOwnScope);

    CREATE_PARSER_OBJECT_INTROSPECTOR_REF(ReturnStatement, mReturnValue);

    CREATE_PARSER_OBJECT_INTROSPECTOR_REF(VariableDeclaration, mInitValue);

    CREATE_PARSER_OBJECT_INTROSPECTOR_REF(IfStatement, mCondition);
    CREATE_PARSER_OBJECT_INTROSPECTOR_REF(IfStatement, mBody);
    CREATE_PARSER_OBJECT_INTROSPECTOR_REF(IfStatement, mElseBody);
    CREATE_PARSER_OBJECT_INTROSPECTOR_REF(IfStatement, mOwnScope);

    CREATE_PARSER_OBJECT_INTROSPECTOR_REF(IntegerLiteral, mValue);

    CREATE_PARSER_OBJECT_INTROSPECTOR_REF(BooleanLiteral, mValue);

    CREATE_PARSER_OBJECT_INTROSPECTOR_REF(VariableExpression, mName);

    CREATE_PARSER_OBJECT_INTROSPECTOR_REF(BinaryExpression, mLeft);
    CREATE_PARSER_OBJECT_INTROSPECTOR_REF(BinaryExpression, mRight);
    CREATE_PARSER_OBJECT_INTROSPECTOR_REF(BinaryExpression, mOperator);

    CREATE_PARSER_OBJECT_INTROSPECTOR_REF(UnaryExpression, mOperand);
    CREATE_PARSER_OBJECT_INTROSPECTOR_REF(UnaryExpression, mOperator);
};

namespace ParserTests
{
    struct TokenConstructor
    {
        TokenConstructor(lexer::TokenType tokenType, std::string text = "")
            : tokenType(tokenType)
            , text(text)
        {
        }

        lexer::TokenType tokenType;
        std::string text;
    };

    struct TestCase
    {
        TestCase()
        {
            Type::Init();
        }

        std::vector<TokenConstructor> tokens;
        ASTNodeIntrospector introspector;

        TestCase& setTokens(const std::vector<TokenConstructor>& newTokens) { tokens = newTokens; return *this; }
    };
    using TestCases = std::vector<TestCase>;

    struct TestCaseResult
    {
        parser::ASTNodePtr ast;
        diagnostic::Diagnostics diag;
        ScopePtr globalScope;
    };


    template <typename... OtherArgs>
    TestCaseResult RunTestCase(const TestCase& testCase, auto function, OtherArgs&&... otherArgs)
    {
        std::vector<lexer::Token> tokens;
        for (const auto& tokenConstructor : testCase.tokens)
        {
            tokens.emplace_back(tokenConstructor.text, tokenConstructor.tokenType, lexer::SourceLocation(), lexer::SourceLocation());
        }
        tokens.emplace_back("", lexer::TokenType::EndOfFile, lexer::SourceLocation(), lexer::SourceLocation());

        diagnostic::Diagnostics diag; // TODO: Add fake diagnostics to pass in
        ScopePtr scope = std::make_unique<Scope>(nullptr, "", true);
        parser::Parser parser(tokens, diag, scope.get());

        auto f = std::bind(function, parser, otherArgs...);
        return {f(), diag, std::move(scope)};
    }

    TEST(Functions, ParserTests)
    {
        TestCase testCase = TestCase()
            .setTokens({
                TokenConstructor(lexer::TokenType::FuncKeyword),
                TokenConstructor(lexer::TokenType::Identifier, "name"),
                TokenConstructor(lexer::TokenType::LeftParen),
                TokenConstructor(lexer::TokenType::RightParen),
                TokenConstructor(lexer::TokenType::RightArrow),
                TokenConstructor(lexer::TokenType::TypeKeyword, "void"),
                TokenConstructor(lexer::TokenType::LeftBrace),
                TokenConstructor(lexer::TokenType::RightBrace),
            });

        auto result = RunTestCase(testCase, testCase.introspector.GetParser_parseFunction(), false, false);

        auto& globalNode = result.ast;
        auto functionNode = dynamic_cast<parser::Function*>(globalNode.get());
        REQUIRE(functionNode != nullptr);

        auto name = testCase.introspector.GetFunction_mName(functionNode);
        REQUIRE(name == "name");

        auto type = testCase.introspector.GetASTNode_mType(functionNode);
        FunctionType* expectedType = FunctionType::Create(Type::Get("void"), {});
        REQUIRE(type == expectedType);

        auto& body = testCase.introspector.GetFunction_mBody(functionNode);
        REQUIRE(body.empty() == true);

        auto& scope = testCase.introspector.GetFunction_mOwnScope(functionNode);
        REQUIRE(scope->parent == result.globalScope.get());
    }

    TEST(IntegerLiterals, ParserTests)
    {
        TestCase testCase = TestCase()
            .setTokens({
                TokenConstructor(lexer::TokenType::IntegerLiteral, "12"),
            });

        auto result = RunTestCase(testCase, testCase.introspector.GetParser_parseIntegerLiteral());

        auto integerLiteral = dynamic_cast<parser::IntegerLiteral*>(result.ast.get());
        REQUIRE(integerLiteral != nullptr);
        
        auto integerLiteralValue = testCase.introspector.GetIntegerLiteral_mValue(integerLiteral);
        REQUIRE(integerLiteralValue == 12);
    }

    TEST(NoValueReturnStatement, ParserTests)
    {
        TestCase testCase = TestCase()
            .setTokens({
                TokenConstructor(lexer::TokenType::ReturnKeyword),
                TokenConstructor(lexer::TokenType::Semicolon),
            });

        auto result = RunTestCase(testCase, testCase.introspector.GetParser_parseReturnStatement());

        auto returnStatement = dynamic_cast<parser::ReturnStatement*>(result.ast.get());
        REQUIRE(returnStatement != nullptr);

        auto& returnValue = testCase.introspector.GetReturnStatement_mReturnValue(returnStatement);
        REQUIRE(returnValue == nullptr);
    }

    TEST(ValueReturnStatement, ParserTests)
    {
        TestCase testCase = TestCase()
            .setTokens({
                TokenConstructor(lexer::TokenType::ReturnKeyword),
                TokenConstructor(lexer::TokenType::IntegerLiteral, "5"),
            });

        auto result = RunTestCase(testCase, testCase.introspector.GetParser_parseReturnStatement());

        auto returnStatement = dynamic_cast<parser::ReturnStatement*>(result.ast.get());
        REQUIRE(returnStatement != nullptr);

        auto& returnValue = testCase.introspector.GetReturnStatement_mReturnValue(returnStatement);
        auto integerLiteral = dynamic_cast<parser::IntegerLiteral*>(returnValue.get());
        REQUIRE(integerLiteral != nullptr);
        
        auto integerLiteralValue = testCase.introspector.GetIntegerLiteral_mValue(integerLiteral);
        REQUIRE(integerLiteralValue == 5);
    }

    TEST(VariableDeclarationNoInit, ParserTests)
    {
        TestCase testCase = TestCase()
            .setTokens({
                TokenConstructor(lexer::TokenType::LetKeyword),
                TokenConstructor(lexer::TokenType::Identifier, "name"),
                TokenConstructor(lexer::TokenType::Colon),
                TokenConstructor(lexer::TokenType::TypeKeyword, "i32"),
                TokenConstructor(lexer::TokenType::Semicolon),
            });

        auto result = RunTestCase(testCase, testCase.introspector.GetParser_parseVariableDeclaration());

        auto variableDeclaration = dynamic_cast<parser::VariableDeclaration*>(result.ast.get());
        REQUIRE(variableDeclaration != nullptr);

        auto type = testCase.introspector.GetASTNode_mType(variableDeclaration);
        REQUIRE(type == Type::Get("i32"));

        auto& initValue = testCase.introspector.GetVariableDeclaration_mInitValue(variableDeclaration);
        REQUIRE(initValue == nullptr);
    }

    TEST(VariableDeclarationInit, ParserTests)
    {
        TestCase testCase = TestCase()
            .setTokens({
                TokenConstructor(lexer::TokenType::LetKeyword),
                TokenConstructor(lexer::TokenType::Identifier, "name"),
                TokenConstructor(lexer::TokenType::Colon),
                TokenConstructor(lexer::TokenType::TypeKeyword, "i32"),
                TokenConstructor(lexer::TokenType::Equal),
                TokenConstructor(lexer::TokenType::IntegerLiteral, "12"),
                TokenConstructor(lexer::TokenType::Semicolon),
            });

        auto result = RunTestCase(testCase, testCase.introspector.GetParser_parseVariableDeclaration());

        auto variableDeclaration = dynamic_cast<parser::VariableDeclaration*>(result.ast.get());
        REQUIRE(variableDeclaration != nullptr);

        auto type = testCase.introspector.GetASTNode_mType(variableDeclaration);
        REQUIRE(type == Type::Get("i32"));

        auto& initValue = testCase.introspector.GetVariableDeclaration_mInitValue(variableDeclaration);
        auto integerLiteral = dynamic_cast<parser::IntegerLiteral*>(initValue.get());
        REQUIRE(integerLiteral != nullptr);

        auto integerLiteralValue = testCase.introspector.GetIntegerLiteral_mValue(integerLiteral);
        REQUIRE(integerLiteralValue == 12);
    }

    TEST(IfStatementNoElse, ParserTests)
    {
        TestCase testCase = TestCase()
            .setTokens({
                TokenConstructor(lexer::TokenType::IfKeyword),
                TokenConstructor(lexer::TokenType::LeftParen),
                TokenConstructor(lexer::TokenType::TrueKeyword),
                TokenConstructor(lexer::TokenType::RightParen),
                TokenConstructor(lexer::TokenType::IntegerLiteral, "0"),
            });

        auto result = RunTestCase(testCase, testCase.introspector.GetParser_parseIfStatement());

        auto ifStatement = dynamic_cast<parser::IfStatement*>(result.ast.get());
        REQUIRE(ifStatement != nullptr);

        auto& condition = testCase.introspector.GetIfStatement_mCondition(ifStatement);
        auto& body = testCase.introspector.GetIfStatement_mBody(ifStatement);
        auto& elseBody = testCase.introspector.GetIfStatement_mElseBody(ifStatement);
        REQUIRE(body != nullptr);
        REQUIRE(elseBody == nullptr);

        auto booleanLiteral = dynamic_cast<parser::BooleanLiteral*>(condition.get());
        REQUIRE(booleanLiteral != nullptr);
        auto booleanLiteralValue = testCase.introspector.GetBooleanLiteral_mValue(booleanLiteral);
        REQUIRE(booleanLiteralValue == true);

        auto integerLiteral = dynamic_cast<parser::IntegerLiteral*>(body.get());
        REQUIRE(integerLiteral != nullptr);

        auto integerLiteralValue = testCase.introspector.GetIntegerLiteral_mValue(integerLiteral);
        REQUIRE(integerLiteralValue == 0);

        auto& scope = testCase.introspector.GetIfStatement_mOwnScope(ifStatement);
        REQUIRE(scope->parent == result.globalScope.get());
    }

    TEST(IfStatementElse, ParserTests)
    {
        TestCase testCase = TestCase()
            .setTokens({
                TokenConstructor(lexer::TokenType::IfKeyword),
                TokenConstructor(lexer::TokenType::LeftParen),
                TokenConstructor(lexer::TokenType::TrueKeyword),
                TokenConstructor(lexer::TokenType::RightParen),
                TokenConstructor(lexer::TokenType::IntegerLiteral, "0"),
                TokenConstructor(lexer::TokenType::Semicolon),
                TokenConstructor(lexer::TokenType::ElseKeyword),
                TokenConstructor(lexer::TokenType::IntegerLiteral, "1"),
            });

        auto result = RunTestCase(testCase, testCase.introspector.GetParser_parseIfStatement());

        auto ifStatement = dynamic_cast<parser::IfStatement*>(result.ast.get());
        REQUIRE(ifStatement != nullptr);

        auto& condition = testCase.introspector.GetIfStatement_mCondition(ifStatement);
        auto& body = testCase.introspector.GetIfStatement_mBody(ifStatement);
        auto& elseBody = testCase.introspector.GetIfStatement_mElseBody(ifStatement);
        REQUIRE(body != nullptr);
        REQUIRE(elseBody != nullptr);

        auto booleanLiteral = dynamic_cast<parser::BooleanLiteral*>(condition.get());
        REQUIRE(booleanLiteral != nullptr);
        auto booleanLiteralValue = testCase.introspector.GetBooleanLiteral_mValue(booleanLiteral);
        REQUIRE(booleanLiteralValue == true);

        auto trueIntegerLiteral = dynamic_cast<parser::IntegerLiteral*>(body.get());
        REQUIRE(trueIntegerLiteral != nullptr);
        auto trueIntegerLiteralValue = testCase.introspector.GetIntegerLiteral_mValue(trueIntegerLiteral);
        REQUIRE(trueIntegerLiteralValue == 0);

        auto falseIntegerLiteral = dynamic_cast<parser::IntegerLiteral*>(elseBody.get());
        REQUIRE(falseIntegerLiteral != nullptr);
        auto falseIntegerLiteralValue = testCase.introspector.GetIntegerLiteral_mValue(falseIntegerLiteral);
        REQUIRE(falseIntegerLiteralValue == 1);

        auto& scope = testCase.introspector.GetIfStatement_mOwnScope(ifStatement);
        REQUIRE(scope->parent == result.globalScope.get());
    }

    TEST(VariableExpression, ParserTests)
    {
        TestCase testCase = TestCase()
            .setTokens({
                TokenConstructor(lexer::TokenType::Identifier, "name"),
            });

        auto result = RunTestCase(testCase, testCase.introspector.GetParser_parseVariableExpression());

        auto variableExpression = dynamic_cast<parser::VariableExpression*>(result.ast.get());
        REQUIRE(variableExpression != nullptr);

        auto name = testCase.introspector.GetVariableExpression_mName(variableExpression);
        REQUIRE(name == "name");
    }

    TEST(BinaryOperators, ParserTests)
    {
        std::vector<std::pair<lexer::TokenType, parser::BinaryExpression::Operator> > tests =
        {
            { lexer::TokenType::Plus,  parser::BinaryExpression::Operator::Add },
            { lexer::TokenType::Minus, parser::BinaryExpression::Operator::Sub },
            { lexer::TokenType::Star,  parser::BinaryExpression::Operator::Mul },
            { lexer::TokenType::Slash, parser::BinaryExpression::Operator::Div },
        };

        for (const auto& test : tests)
        {
            TestCase testCase = TestCase()
                .setTokens({
                    TokenConstructor(lexer::TokenType::IntegerLiteral, "1"),
                    TokenConstructor(test.first),
                    TokenConstructor(lexer::TokenType::IntegerLiteral, "2"),
                });

            auto result = RunTestCase(testCase, testCase.introspector.GetParser_parseExpression(), 1);

            auto binaryExpression = dynamic_cast<parser::BinaryExpression*>(result.ast.get());
            REQUIRE(binaryExpression != nullptr);

            auto op = testCase.introspector.GetBinaryExpression_mOperator(binaryExpression);
            REQUIRE(op == test.second);

            auto& lhs = testCase.introspector.GetBinaryExpression_mLeft(binaryExpression);
            REQUIRE(lhs != nullptr);
            auto left = dynamic_cast<parser::IntegerLiteral*>(lhs.get());
            REQUIRE(left != nullptr);
            auto leftValue = testCase.introspector.GetIntegerLiteral_mValue(left);
            REQUIRE(leftValue == 1);

            auto& rhs = testCase.introspector.GetBinaryExpression_mRight(binaryExpression);
            REQUIRE(rhs != nullptr);
            auto right = dynamic_cast<parser::IntegerLiteral*>(rhs.get());
            REQUIRE(right != nullptr);
            auto rightValue = testCase.introspector.GetIntegerLiteral_mValue(right);
            REQUIRE(rightValue == 2);
        }
    }

    TEST(PrefixUnaryOperators, ParserTests)
    {
        std::vector<std::pair<lexer::TokenType, parser::UnaryExpression::Operator> > tests =
        {
            { lexer::TokenType::Minus,  parser::UnaryExpression::Operator::Negate },
            { lexer::TokenType::Ampersand,  parser::UnaryExpression::Operator::AddressOf },
            { lexer::TokenType::Star, parser::UnaryExpression::Operator::Indirection },
        };

        for (const auto& test : tests)
        {
            TestCase testCase = TestCase()
                .setTokens({
                    TokenConstructor(test.first),
                    TokenConstructor(lexer::TokenType::IntegerLiteral, "2"),
                });

            auto result = RunTestCase(testCase, testCase.introspector.GetParser_parseExpression(), 1);

            auto unaryExpression = dynamic_cast<parser::UnaryExpression*>(result.ast.get());
            REQUIRE(unaryExpression != nullptr);

            auto op = testCase.introspector.GetUnaryExpression_mOperator(unaryExpression);
            REQUIRE(op == test.second);

            auto& operand = testCase.introspector.GetUnaryExpression_mOperand(unaryExpression);
            REQUIRE(operand != nullptr);

            auto integerLiteral = dynamic_cast<parser::IntegerLiteral*>(operand.get());
            REQUIRE(integerLiteral != nullptr);

            auto integerLiteralValue = testCase.introspector.GetIntegerLiteral_mValue(integerLiteral);
            REQUIRE(integerLiteralValue == 2);
        }
    }
}