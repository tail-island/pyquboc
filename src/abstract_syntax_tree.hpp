#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include <boost/functional/hash.hpp>

namespace pyquboc {
  // TODO: std::variantを使用して、std::shared_ptrとか継承とかを使用せずに実装できるか検討する。Pythonからの参照で落とし穴にはまるような気がするけど。。。

  enum class expression_type {
    add_operator,
    mul_operator,
    binary_variable,
    spin_variable,
    place_holder_variable,
    with_penalty,
    constraint,
    numeric_literal
  };

  class expression {
  public:
    virtual pyquboc::expression_type expression_type() const noexcept = 0;

    virtual std::string to_string() const noexcept = 0;

    virtual std::size_t hash() const noexcept = 0;

    virtual bool equals(const std::shared_ptr<const expression>& other) const noexcept {
      return expression_type() == other->expression_type();
    };

    friend std::hash<expression>;
  };
}

namespace std {
  template <>
  struct hash<pyquboc::expression> {
    auto operator()(const pyquboc::expression& expression) const noexcept {
      return expression.hash();
    }
  };
}

namespace pyquboc {
  class binary_operator : public expression {
    std::shared_ptr<const expression> _lhs;
    std::shared_ptr<const expression> _rhs;

  protected:
    binary_operator(const std::shared_ptr<const expression>& lhs, const std::shared_ptr<const expression>& rhs) noexcept : _lhs(lhs), _rhs(rhs) {
      ;
    }

  public:
    const auto& lhs() const noexcept {
      return _lhs;
    }

    const auto& rhs() const noexcept {
      return _rhs;
    }

    std::size_t hash() const noexcept override {
      auto result = static_cast<std::size_t>(0);

      boost::hash_combine(result, std::hash<expression>()(*_lhs));
      boost::hash_combine(result, std::hash<expression>()(*_rhs));

      return result;
    }

    bool equals(const std::shared_ptr<const expression>& other) const noexcept override {
      return expression::equals(other) && _lhs->equals(std::static_pointer_cast<const binary_operator>(other)->_lhs) && _rhs->equals(std::static_pointer_cast<const binary_operator>(other)->_rhs);
    }
  };

  class add_operator final : public binary_operator {
  public:
    add_operator(const std::shared_ptr<const expression>& lhs, const std::shared_ptr<const expression>& rhs) noexcept : binary_operator(lhs, rhs) {
      ;
    }

    pyquboc::expression_type expression_type() const noexcept {
      return expression_type::add_operator;
    }

    std::string to_string() const noexcept override {
      return "(" + lhs()->to_string() + " + " + rhs()->to_string() + ")";
    }

    std::size_t hash() const noexcept override {
      auto result = binary_operator::hash();

      boost::hash_combine(result, "+");

      return result;
    }
  };

  class mul_operator final : public binary_operator {
  public:
    mul_operator(const std::shared_ptr<const expression>& lhs, const std::shared_ptr<const expression>& rhs) noexcept : binary_operator(lhs, rhs) {
      ;
    }

    pyquboc::expression_type expression_type() const noexcept {
      return expression_type::mul_operator;
    }

    std::string to_string() const noexcept override {
      return "(" + lhs()->to_string() + " * " + rhs()->to_string() + ")";
    }

    std::size_t hash() const noexcept override {
      auto result = binary_operator::hash();

      boost::hash_combine(result, "*");

      return result;
    }
  };

  class variable : public expression {
    std::string _name;

  protected:
    variable(const std::string& name) noexcept : _name(name) {
      ;
    }

  public:
    const auto& name() const noexcept {
      return _name;
    }

    std::size_t hash() const noexcept override {
      return std::hash<std::string>()(_name);
    }

    bool equals(const std::shared_ptr<const expression>& other) const noexcept override {
      return expression::equals(other) && _name == std::static_pointer_cast<const variable>(other)->_name;
    }
  };

  class binary_variable final : public variable {
  public:
    binary_variable(const std::string& name) noexcept : variable(name) {
      ;
    }

    pyquboc::expression_type expression_type() const noexcept {
      return expression_type::binary_variable;
    }

    std::string to_string() const noexcept override {
      return "Binary('" + name() + "')";
    }

    std::size_t hash() const noexcept override {
      auto result = variable::hash();

      boost::hash_combine(result, "binary_variable");

      return result;
    }
  };

  class spin_variable final : public variable {
  public:
    spin_variable(const std::string& name) noexcept : variable(name) {
      ;
    }

    pyquboc::expression_type expression_type() const noexcept {
      return expression_type::spin_variable;
    }

    std::string to_string() const noexcept override {
      return "Spin('" + name() + "')";
    }

    std::size_t hash() const noexcept override {
      auto result = variable::hash();

      boost::hash_combine(result, "spin_variable");

      return result;
    }
  };

  class placeholder_variable final : public variable {
  public:
    placeholder_variable(const std::string& name) noexcept : variable(name) {
      ;
    }

    pyquboc::expression_type expression_type() const noexcept {
      return expression_type::place_holder_variable;
    }

    std::string to_string() const noexcept override {
      return "Placeholder('" + name() + "')";
    }

    std::size_t hash() const noexcept override {
      auto result = variable::hash();

      boost::hash_combine(result, "placeholder_variable");

      return result;
    }
  };

  class constraint final : public variable {
    std::shared_ptr<const expression> _expression;
    std::function<bool(double)> _condition;

  public:
    constraint(const std::shared_ptr<const pyquboc::expression>& expression, const std::string& name, const std::function<bool(double)>& condition) noexcept : variable(name), _expression(expression), _condition(condition) {
      ;
    }

    const auto& expression() const noexcept {
      return _expression;
    }

    const auto& condition() const noexcept {
      return _condition;
    }

    pyquboc::expression_type expression_type() const noexcept {
      return expression_type::constraint;
    }

    std::string to_string() const noexcept override {
      return "Constraint(" + _expression->to_string() + ", '" + name() + "')"; // conditionは文字列化できない……。
    }

    std::size_t hash() const noexcept override {
      auto result = variable::hash();

      boost::hash_combine(result, "constraint");
      boost::hash_combine(result, std::hash<pyquboc::expression>()(*_expression));

      return result;
    }

    bool equals(const std::shared_ptr<const pyquboc::expression>& other) const noexcept override {
      return variable::equals(other) && _expression->equals(std::static_pointer_cast<const constraint>(other)->_expression);
    }
  };

  class with_penalty final : public variable {
    std::shared_ptr<const expression> _expression;
    std::shared_ptr<const expression> _penalty;

  public:
    with_penalty(const std::shared_ptr<const pyquboc::expression>& expression, const std::shared_ptr<const pyquboc::expression>& strength, const std::string& name) noexcept : variable(name), _expression(expression), _penalty(strength) {
      ;
    }

    const auto& expression() const noexcept {
      return _expression;
    }

    const auto& penalty() const noexcept {
      return _penalty;
    }

    pyquboc::expression_type expression_type() const noexcept {
      return expression_type::with_penalty;
    }

    std::string to_string() const noexcept override {
      return "WithPenalty(" + _expression->to_string() + ", " + _penalty->to_string() + ", '" + name() + "')";
    }

    std::size_t hash() const noexcept override {
      auto result = variable::hash();

      boost::hash_combine(result, "with_penalty");
      boost::hash_combine(result, std::hash<pyquboc::expression>()(*_expression));
      boost::hash_combine(result, std::hash<pyquboc::expression>()(*_penalty));

      return result;
    }

    bool equals(const std::shared_ptr<const pyquboc::expression>& other) const noexcept override {
      return variable::equals(other) && _expression->equals(std::static_pointer_cast<const with_penalty>(other)->_expression) && _penalty == std::static_pointer_cast<const with_penalty>(other)->_penalty;
    }
  };

  class numeric_literal final : public expression {
    double _value;

  public:
    numeric_literal(double value) noexcept : _value(value) {
      ;
    }

    auto value() const noexcept {
      return _value;
    }

    pyquboc::expression_type expression_type() const noexcept {
      return expression_type::numeric_literal;
    }

    std::string to_string() const noexcept override {
      return std::to_string(_value);
    }

    std::size_t hash() const noexcept override {
      return std::hash<double>()(_value);
    }

    bool equals(const std::shared_ptr<const expression>& other) const noexcept override {
      return expression::equals(other) && _value == std::static_pointer_cast<const numeric_literal>(other)->_value;
    }
  };

  inline std::shared_ptr<const expression> operator+(const std::shared_ptr<const expression>& lhs, const std::shared_ptr<const expression>& rhs) noexcept {
    if (lhs->expression_type() == expression_type::numeric_literal && rhs->expression_type() == expression_type::numeric_literal) {
      return std::make_shared<numeric_literal>(std::static_pointer_cast<const numeric_literal>(lhs)->value() + std::static_pointer_cast<const numeric_literal>(rhs)->value());
    }

    return std::make_shared<const add_operator>(lhs, rhs);
  }

  inline std::shared_ptr<const expression> operator*(const std::shared_ptr<const expression>& lhs, const std::shared_ptr<const expression>& rhs) noexcept {
    if (lhs->expression_type() == expression_type::numeric_literal && rhs->expression_type() == expression_type::numeric_literal) {
      return std::make_shared<numeric_literal>(std::static_pointer_cast<const numeric_literal>(lhs)->value() * std::static_pointer_cast<const numeric_literal>(rhs)->value());
    }

    return std::make_shared<const mul_operator>(lhs, rhs);
  }

  template <typename Result, typename Functor>
  Result visit(Functor& functor, const std::shared_ptr<const expression>& expression) noexcept {
    switch (expression->expression_type()) {
    case expression_type::add_operator:
      return functor(std::static_pointer_cast<const add_operator>(expression));

    case expression_type::mul_operator:
      return functor(std::static_pointer_cast<const mul_operator>(expression));

    case expression_type::binary_variable:
      return functor(std::static_pointer_cast<const binary_variable>(expression));

    case expression_type::spin_variable:
      return functor(std::static_pointer_cast<const spin_variable>(expression));

    case expression_type::place_holder_variable:
      return functor(std::static_pointer_cast<const placeholder_variable>(expression));

    case expression_type::with_penalty:
      return functor(std::static_pointer_cast<const with_penalty>(expression));

    case expression_type::constraint:
      return functor(std::static_pointer_cast<const constraint>(expression));

    case expression_type::numeric_literal:
      return functor(std::static_pointer_cast<const numeric_literal>(expression));

    default:
      throw std::runtime_error("invalid expression type."); // ここには絶対に来ないはず。
    }
  }
}
