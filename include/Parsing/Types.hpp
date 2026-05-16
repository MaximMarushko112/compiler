#pragma once

#include "ASTVisitor.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Parsing {

//class ASTVisitor;

// ---------------------------------------------------------------------
// Base Type
// ---------------------------------------------------------------------
class Type {
public:
    virtual ~Type() = 0;
    virtual void accept(ASTVisitor& visitor) const = 0;
    virtual std::unique_ptr<Type> clone() const = 0;
};

// ---------------------------------------------------------------------
// Concrete Types
// ---------------------------------------------------------------------
class VoidType : public Type {
public:
    void accept(ASTVisitor& visitor) const override;
    std::unique_ptr<Type> clone() const override;
};

class IntType : public Type {
public:
    void accept(ASTVisitor& visitor) const override;
    std::unique_ptr<Type> clone() const override;
};

class UnsignedType : public Type {
public:
    void accept(ASTVisitor& visitor) const override;
    std::unique_ptr<Type> clone() const override;
};

class FloatType : public Type {
public:
    void accept(ASTVisitor& visitor) const override;
    std::unique_ptr<Type> clone() const override;
};

class BoolType : public Type {
public:
    void accept(ASTVisitor& visitor) const override;
    std::unique_ptr<Type> clone() const override;
};

class StringType : public Type {
public:
    void accept(ASTVisitor& visitor) const override;
    std::unique_ptr<Type> clone() const override;
};

class NamedType : public Type {
public:
    std::string name;
    explicit NamedType(std::string n) : name(std::move(n)) {}
    void accept(ASTVisitor& visitor) const override;
    std::unique_ptr<Type> clone() const override;
};

class PointerType : public Type {
public:
    std::unique_ptr<Type> pointee;
    explicit PointerType(std::unique_ptr<Type> p) : pointee(std::move(p)) {}
    void accept(ASTVisitor& visitor) const override;
    std::unique_ptr<Type> clone() const override;
};

class ArrayType : public Type {
public:
    std::unique_ptr<Type> elementType;
    std::optional<int> size;
    ArrayType(std::unique_ptr<Type> elem, std::optional<int> s = std::nullopt)
        : elementType(std::move(elem)), size(s) {}
    void accept(ASTVisitor& visitor) const override;
    std::unique_ptr<Type> clone() const override;
};

class FunctionType : public Type {
public:
    std::vector<std::unique_ptr<Type>> parameterTypes;
    std::vector<std::unique_ptr<Type>> returnTypes;
    bool variadic = false;
    void accept(ASTVisitor& visitor) const override;
    std::unique_ptr<Type> clone() const override;
};

class TupleType : public Type {
public:
    std::vector<std::unique_ptr<Type>> types;
    void accept(ASTVisitor& visitor) const override;
    std::unique_ptr<Type> clone() const override;
};

// ---------------------------------------------------------------------
// Inline implementations
// ---------------------------------------------------------------------
inline Type::~Type() = default;

inline void VoidType::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline std::unique_ptr<Type> VoidType::clone() const { return std::make_unique<VoidType>(*this); }

inline void IntType::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline std::unique_ptr<Type> IntType::clone() const { return std::make_unique<IntType>(*this); }

inline void UnsignedType::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline std::unique_ptr<Type> UnsignedType::clone() const { return std::make_unique<UnsignedType>(*this); }

inline void FloatType::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline std::unique_ptr<Type> FloatType::clone() const { return std::make_unique<FloatType>(*this); }

inline void BoolType::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline std::unique_ptr<Type> BoolType::clone() const { return std::make_unique<BoolType>(*this); }

inline void StringType::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline std::unique_ptr<Type> StringType::clone() const { return std::make_unique<StringType>(*this); }

inline void NamedType::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline std::unique_ptr<Type> NamedType::clone() const { return std::make_unique<NamedType>(name); }

inline void PointerType::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline std::unique_ptr<Type> PointerType::clone() const {
    return std::make_unique<PointerType>(pointee ? pointee->clone() : nullptr);
}

inline void ArrayType::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline std::unique_ptr<Type> ArrayType::clone() const {
    return std::make_unique<ArrayType>(elementType ? elementType->clone() : nullptr, size);
}

inline void FunctionType::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline std::unique_ptr<Type> FunctionType::clone() const {
    auto cloned = std::make_unique<FunctionType>();
    cloned->variadic = variadic;
    for (const auto& p : parameterTypes)
        cloned->parameterTypes.push_back(p ? p->clone() : nullptr);
    for (const auto& r : returnTypes)
        cloned->returnTypes.push_back(r ? r->clone() : nullptr);
    return cloned;
}

inline void TupleType::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline std::unique_ptr<Type> TupleType::clone() const {
    auto cloned = std::make_unique<TupleType>();
    for (const auto& t : types)
        cloned->types.push_back(t ? t->clone() : nullptr);
    return cloned;
}

} // namespace Parsing