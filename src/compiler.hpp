#pragma once

#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>

#include "abstract_syntax_tree.hpp"
#include "model.hpp"

#include <iostream>

namespace pyquboc {
  // Expand to polynomial.

  // TODO: ペナルティを最後ではなく途中で足し合わせられるか検討する。もし途中で足し合わせられるなら、戻り値が一つになって嬉しい。

  class expand final {
    std::unordered_map<std::string, polynomial> _constraints;
    variables* _variables;

  public:
    auto operator()(const std::shared_ptr<const expression>& expression, variables* variables) noexcept {
      _variables = variables;
      _constraints = {};

      auto [polynomial, penalty] = visit<std::tuple<pyquboc::polynomial, pyquboc::polynomial>>(*this, expression);

      return std::tuple{polynomial + penalty, _constraints};
    }

    auto operator()(const std::shared_ptr<const add_operator>& add_operator) noexcept {
      const auto [l_polynomial, l_penalty] = visit<std::tuple<polynomial, polynomial>>(*this, add_operator->lhs());
      const auto [r_polynomial, r_pelalty] = visit<std::tuple<polynomial, polynomial>>(*this, add_operator->rhs());

      return std::tuple{l_polynomial + r_polynomial, l_penalty + r_pelalty};
    }

    auto operator()(const std::shared_ptr<const mul_operator>& mul_operator) noexcept {
      const auto [l_polynomial, l_penalty] = visit<std::tuple<polynomial, polynomial>>(*this, mul_operator->lhs());
      const auto [r_polynomial, r_penalty] = visit<std::tuple<polynomial, polynomial>>(*this, mul_operator->rhs());

      return std::tuple{l_polynomial * r_polynomial, l_penalty + r_penalty};
    }

    auto operator()(const std::shared_ptr<const binary_variable>& binary_variable) noexcept {
      return std::tuple{polynomial{{{_variables->index(binary_variable->name())}, std::make_shared<numeric_literal>(1)}}, polynomial{}};
    }

    auto operator()(const std::shared_ptr<const spin_variable>& spin_variable) noexcept {
      return std::tuple{polynomial{{{_variables->index(spin_variable->name())}, std::make_shared<numeric_literal>(2)}, {{}, std::make_shared<numeric_literal>(-1)}}, polynomial{}};
    }

    auto operator()(const std::shared_ptr<const placeholder_variable>& place_holder_variable) noexcept {
      return std::tuple{polynomial{{{}, place_holder_variable}}, polynomial{}};
    }

    auto operator()(const std::shared_ptr<const with_penalty>& with_penalty) noexcept {
      const auto [e_polynomial, e_penalty] = visit<std::tuple<polynomial, polynomial>>(*this, with_penalty->expression());
      const auto [p_polynomial, p_penalty] = visit<std::tuple<polynomial, polynomial>>(*this, with_penalty->penalty());

      return std::tuple{e_polynomial, e_penalty + p_penalty + p_polynomial};
    }

    auto operator()(const std::shared_ptr<const constraint>& constraint) noexcept {
      const auto [polynomial, penalty] = visit<std::tuple<pyquboc::polynomial, pyquboc::polynomial>>(*this, constraint->expression());

      _constraints.emplace(constraint->name(), polynomial);

      return std::tuple{polynomial, penalty};
    }

    auto operator()(const std::shared_ptr<const numeric_literal>& numeric_literal) noexcept {
      return std::tuple{polynomial{{{}, numeric_literal}}, polynomial{}};
    }
  };

  // Convert to quadratic polynomial.

  inline std::optional<std::pair<int, int>> find_replacing_pair(const pyquboc::polynomial& polynomial) noexcept {
    auto counts = [&]() {
      auto result = std::map<std::pair<int, int>, int>{};

      for (const auto& [product, _] : polynomial) {
        if (std::size(product.indexes()) <= 2) {
          continue;
        }

        for (auto it_1 = std::begin(product.indexes()); it_1 != std::prev(std::end(product.indexes())); ++it_1) {
          for (auto it_2 = std::next(it_1); it_2 != std::end(product.indexes()); ++it_2) {
            const auto [it, emplaced] = result.emplace(std::pair{*it_1, *it_2}, 1);

            if (!emplaced) {
              it->second++;
            }
          }
        }
      }

      return result;
    }();

    if (std::size(counts) == 0) {
      return std::nullopt;
    }

    const auto it = std::max_element(std::begin(counts), std::end(counts), [](const auto& count_1, const auto& count_2) {
      return count_1.second < count_2.second;
    });

    return it->first;
  }

  inline auto convert_to_quadratic(const pyquboc::polynomial& polynomial, double strength, variables* variables) noexcept {
    auto result = polynomial;

    for (;;) {
      const auto replacing_pair = find_replacing_pair(result);

      if (!replacing_pair) {
        break;
      }

      const auto replacing_pair_index = variables->index(variables->name(replacing_pair->first) + " * " + variables->name(replacing_pair->second));

      // replace.

      for (;;) {
        auto it = std::find_if(std::begin(result), std::end(result), [&](const auto& term) {
          return std::binary_search(std::begin(term.first.indexes()), std::end(term.first.indexes()), replacing_pair->first) && std::binary_search(std::begin(term.first.indexes()), std::end(term.first.indexes()), replacing_pair->second);
        });

        if (it == std::end(result)) {
          break;
        }

        const auto indexes = [&]() {
          auto result = pyquboc::indexes{};

          std::copy_if(std::begin(it->first.indexes()), std::end(it->first.indexes()), std::back_inserter(result), [&](const auto& index) {
            return index != replacing_pair->first && index != replacing_pair->second;
          });

          result.emplace_back(replacing_pair_index);

          return result;
        }();
        const auto expression = it->second;

        result.erase(it);
        result.emplace(product(indexes), expression);
      }

      // insert.

      const auto emplace_term = [](pyquboc::polynomial& polynomial, const pyquboc::product& product, const std::shared_ptr<const expression>& coefficient) {
        const auto [it, emplaced] = polynomial.emplace(product, coefficient);

        if (!emplaced) {
          it->second = it->second + coefficient;
        }
      };

      // clang-format off
      emplace_term(result, product{replacing_pair_index                          }, std::make_shared<numeric_literal>(strength *  3));
      emplace_term(result, product{replacing_pair->first,  replacing_pair_index  }, std::make_shared<numeric_literal>(strength * -2));
      emplace_term(result, product{replacing_pair->second, replacing_pair_index  }, std::make_shared<numeric_literal>(strength * -2));
      emplace_term(result, product{replacing_pair->first,  replacing_pair->second}, std::make_shared<numeric_literal>(strength *  1));
      // clang-format on
    }

    return result;
  }

  // Compile.

  inline auto compile(const std::shared_ptr<const expression>& expression, double strength) noexcept {
    auto variables = pyquboc::variables();

    const auto [polynomial, constraints] = expand()(expression, &variables);
    const auto quadratic_polynomial = convert_to_quadratic(polynomial, strength, &variables);

    return model(quadratic_polynomial, constraints, variables);
  }
}
