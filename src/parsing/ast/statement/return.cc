#include <iostream>
#include <parsing/ast/statement/return.hh>

ReturnStatement::ReturnStatement(std::unique_ptr<ASTNode> value)
    :_value(std::move(value))
{
    _nodeType = ASTNodeType::ReturnStatement;
}

void ReturnStatement::Print(std::ostream& stream, int indent) const
{
    stream << std::string(indent, ' ') << "<Return-Statement>";
    if(_value)
    {
        stream << ":\n" <<std::string(indent, ' ') << "Value:\n";
        _value->Print(stream, indent + 2);
    }
}

Codegen::Value* ReturnStatement::Generate(Codegen::Module& module, Codegen::Builder& builder, bool)
{
    if(_value)
    {
        Codegen::Value* valueCodegen =_value->Generate(module, builder);
        if(valueCodegen->GetType() != _type)
            valueCodegen = Type::Convert(valueCodegen, _type, builder);
        return builder.CreateRet(valueCodegen);
    }

    return builder.CreateRet(nullptr);
}