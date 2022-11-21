#include <llvm/IR/Instructions.h>
#include <parsing/ast/expression/binaryExpression.hh>
#include <diagnostics.hh>

namespace Parsing
{
    BinaryExpression::BinaryExpression(std::unique_ptr<ASTNode> lhs, Lexing::Token op, std::unique_ptr<ASTNode> rhs)
        :ASTNode(ASTNodeType::BinaryExpression), _lhs(std::move(lhs)), _rhs(std::move(rhs))
    {
        switch(op.GetType())
        {
            case Lexing::TokenType::Plus:
                _operator = BinaryOperator::Addition;
                break;
            case Lexing::TokenType::Minus:
                _operator = BinaryOperator::Subtraction;
                break;
            case Lexing::TokenType::Star:
                _operator = BinaryOperator::Multiplication;
                break;
            case Lexing::TokenType::Slash:
                _operator = BinaryOperator::Division;
                break;
            case Lexing::TokenType::DoubleEquals:
                _operator = BinaryOperator::Equal;
                break;
            case Lexing::TokenType::BangEquals:
                _operator = BinaryOperator::NotEqual;
                break;
            case Lexing::TokenType::LeftAngleBracket:
                _operator = BinaryOperator::LessThan;
                break;
            case Lexing::TokenType::RightAngleBracket:
                _operator = BinaryOperator::GreaterThan;
                break;
            case Lexing::TokenType::Equals:
                _operator = BinaryOperator::Assignment;
            default:
                break;
        }
    }

    std::string BinaryExpression::OperatorToString() const
    {
        switch(_operator)
        {
            case BinaryOperator::Addition:
                return "Addition";
            case BinaryOperator::Subtraction:
                return "Subtraction";
            case BinaryOperator::Multiplication:
                return "Multiplication";
            case BinaryOperator::Division:
                return "Division";
            case BinaryOperator::Equal:
                return "Equal";
            case BinaryOperator::NotEqual:
                return "NotEqual";
            case BinaryOperator::LessThan:
                return "LessThan";
            case BinaryOperator::GreaterThan:
                return "GreaterThan";
            case BinaryOperator::Assignment:
                return "Assignment";
        }
        return "";
    }

    void BinaryExpression::Print(std::ostream& stream, int indent) const
    {
        stream << std::string(indent, ' ') << "<Binary-Expression>:\n";
        stream << std::string(indent, ' ') << "Lhs: ";
        _lhs->Print(stream, indent + 2);
        stream << std::string(indent, ' ') << "\nOperator: " << OperatorToString() << "\n";
        stream << std::string(indent, ' ') << "Rhs: ";
        _rhs->Print(stream, indent + 2);
    }

    llvm::Value* BinaryExpression::Emit(llvm::LLVMContext& ctx, llvm::Module& mod, llvm::IRBuilder<>& builder, std::shared_ptr<Environment> scope)
    {
        llvm::Value* left = _lhs->Emit(ctx, mod, builder, scope);
        llvm::Value* right = _rhs->Emit(ctx, mod, builder, scope);
        
        switch(_operator)
        {
            case BinaryOperator::Addition:
                return builder.CreateAdd(left, right);
            case BinaryOperator::Subtraction:
                return builder.CreateSub(left, right);
            case BinaryOperator::Multiplication:
                return builder.CreateMul(left, right);
            case BinaryOperator::Division:
                return builder.CreateSDiv(left, right);
            case BinaryOperator::Assignment:
            {
                llvm::LoadInst* load = static_cast<llvm::LoadInst*>(left);
                llvm::Value* ptr = load->getPointerOperand();
                load->eraseFromParent();
                return builder.CreateStore(right, ptr);
            }
            default:
                return nullptr;
        }
    }
}