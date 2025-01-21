cmake_minimum_required(VERSION 3.8)

include(FetchContent)
FetchContent_Declare(vipir
    GIT_REPOSITORY https://github.com/viper-org/vipir
    GIT_TAG master
)
FetchContent_MakeAvailable(vipir)

set(SOURCES
    "source/lexer/Lexer.cpp"
    "source/lexer/Token.cpp"
    "source/lexer/SourceLocation.cpp"

    "source/symbol/Scope.cpp"
    "source/symbol/Mangle.cpp"
    "source/symbol/ImportManager.cpp"
    "source/symbol/Identifier.cpp"

    "source/parser/Parser.cpp"
    "source/parser/SymbolParser.cpp"
    "source/parser/ast/ASTNode.cpp"
    "source/parser/ast/global/Function.cpp"
    "source/parser/ast/global/ClassDeclaration.cpp"
    "source/parser/ast/global/Namespace.cpp"
    "source/parser/ast/statement/ReturnStatement.cpp"
    "source/parser/ast/statement/VariableDeclaration.cpp"
    "source/parser/ast/statement/IfStatement.cpp"
    "source/parser/ast/expression/BinaryExpression.cpp" 
    "source/parser/ast/expression/IntegerLiteral.cpp"
    "source/parser/ast/expression/BooleanLiteral.cpp"
    "source/parser/ast/expression/UnaryExpression.cpp"
    "source/parser/ast/expression/VariableExpression.cpp"
    "source/parser/ast/expression/CastExpression.cpp"
    "source/parser/ast/expression/CallExpression.cpp"
    "source/parser/ast/expression/StringLiteral.cpp"
    "source/parser/ast/expression/MemberAccess.cpp"

    "source/type/Type.cpp"
    "source/type/IntegerType.cpp"
    "source/type/VoidType.cpp"
    "source/type/FunctionType.cpp"
    "source/type/BooleanType.cpp"
    "source/type/PointerType.cpp"
    "source/type/StructType.cpp"
    "source/type/ErrorType.cpp"

    "source/diagnostic/Diagnostic.cpp"
)

set(HEADERS
    "include/lexer/Lexer.h"
    "include/lexer/SourceLocation.h"
    "include/lexer/Token.h"

    "include/symbol/Scope.h"
    "include/symbol/Mangle.h"
    "include/symbol/ImportManager.h"
    "include/symbol/Identifier.h"

    "include/parser/Parser.h"
    "include/parser/SymbolParser.h"
    "include/parser/ast/ASTNode.h"
    "include/parser/ast/global/Function.h"
    "include/parser/ast/global/ClassDeclaration.h"
    "include/parser/ast/global/Namespace.h"
    "include/parser/ast/statement/ReturnStatement.h"
    "include/parser/ast/statement/VariableDeclaration.h"
    "include/parser/ast/statement/IfStatement.h"
    "include/parser/ast/expression/BinaryExpression.h"
    "include/parser/ast/expression/IntegerLiteral.h"
    "include/parser/ast/expression/BooleanLiteral.h"
    "include/parser/ast/expression/UnaryExpression.h"
    "include/parser/ast/expression/VariableExpression.h"
    "include/parser/ast/expression/CastExpression.h"
    "include/parser/ast/expression/CallExpression.h"
    "include/parser/ast/expression/StringLiteral.h"
    "include/parser/ast/expression/MemberAccess.h"

    "include/type/Type.h"
    "include/type/IntegerType.h"
    "include/type/VoidType.h"
    "include/type/FunctionType.h"
    "include/type/BooleanType.h"
    "include/type/PointerType.h"
    "include/type/StructType.h"
    "include/type/ErrorType.h"

    "include/diagnostic/Diagnostic.h"
)

source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${SOURCES} ${HEADERS})

add_library(viper-framework-viper-framework ${SOURCES} ${HEADERS})
add_library(viper::framework ALIAS viper-framework-viper-framework)

target_include_directories(viper-framework-viper-framework PUBLIC include)
target_compile_features(viper-framework-viper-framework PUBLIC cxx_std_20)
target_link_libraries(viper-framework-viper-framework vipir)