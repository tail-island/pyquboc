#include <algorithm>
#include <iostream>
#include <map>
#include <vector>

#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <vartypes.hpp>

#include "abstract_syntax_tree.hpp"
#include "compiler.hpp"

namespace py = pybind11;
using namespace py::literals;

PYBIND11_MODULE(cpp_pyquboc, m) {
  m.doc() = "pyquboc C++ binding";

  py::class_<pyquboc::expression, std::shared_ptr<pyquboc::expression>>(m, "Base")
      .def("__add__", [](const std::shared_ptr<const pyquboc::expression>& expression, const std::shared_ptr<const pyquboc::expression>& other) {
        return expression + other;
      })
      .def("__add__", [](const std::shared_ptr<const pyquboc::expression>& expression, double other) {
        return expression + std::make_shared<const pyquboc::numeric_literal>(other);
      })
      .def("__radd__", [](const std::shared_ptr<const pyquboc::expression>& expression, double other) {
        return std::make_shared<const pyquboc::numeric_literal>(other) + expression;
      })
      .def("__sub__", [](const std::shared_ptr<const pyquboc::expression>& expression, const std::shared_ptr<const pyquboc::expression>& other) {
        return expression + std::make_shared<const pyquboc::numeric_literal>(-1) * other;
      })
      .def("__sub__", [](const std::shared_ptr<const pyquboc::expression>& expression, double other) {
        return expression + std::make_shared<const pyquboc::numeric_literal>(-other);
      })
      .def("__rsub__", [](const std::shared_ptr<const pyquboc::expression>& expression, double other) {
        return std::make_shared<const pyquboc::numeric_literal>(other) + std::make_shared<const pyquboc::numeric_literal>(-1) * expression;
      })
      .def("__mul__", [](const std::shared_ptr<const pyquboc::expression>& expression, const std::shared_ptr<const pyquboc::expression>& other) {
        return expression * other;
      })
      .def("__mul__", [](const std::shared_ptr<const pyquboc::expression>& expression, double other) {
        return expression * std::make_shared<const pyquboc::numeric_literal>(other);
      })
      .def("__rmul__", [](const std::shared_ptr<const pyquboc::expression>& expression, double other) {
        return std::make_shared<const pyquboc::numeric_literal>(other) * expression;
      })
      .def("__truediv__", [](const std::shared_ptr<const pyquboc::expression>& expression, double other) {
        if (other == 0) {
          throw std::runtime_error("zero divide error.");
        }

        return expression * std::make_shared<const pyquboc::numeric_literal>(1 / other);
      })
      .def("__pow__", [](const std::shared_ptr<const pyquboc::expression>& expression, int expotent) {
        if (expotent <= 0) {
          throw std::runtime_error("`exponent` should be positive.");
        }

        auto result = expression;

        for (auto i = 1; i < expotent; ++i) {
          result = result * expression;
        }

        return result;
      })
      .def("__neg__", [](const std::shared_ptr<const pyquboc::expression>& expression) {
        return std::make_shared<const pyquboc::numeric_literal>(-1) * expression;
      })
      .def(
          "compile", [](const std::shared_ptr<const pyquboc::expression>& expression, double strength) {
            return pyquboc::compile(expression, strength);
          },
          py::arg("strength") = 5)
      .def("__hash__", [](const pyquboc::expression& expression) { // 必要？
        return std::hash<pyquboc::expression>()(expression);
      })
      .def("__eq__", &pyquboc::expression::equals) // 必要？
      .def("__str__", &pyquboc::expression::to_string)
      .def("__repr__", &pyquboc::expression::to_string);

  py::class_<pyquboc::add_operator, std::shared_ptr<pyquboc::add_operator>, pyquboc::expression>(m, "Add")
      .def("__iadd__", [](std::shared_ptr<pyquboc::add_operator>& add_operator, const std::shared_ptr<const pyquboc::expression>& other) {
        add_operator->add_child(other);

        return add_operator;
      })
      .def("__iadd__", [](std::shared_ptr<pyquboc::add_operator>& add_operator, double other) {
        add_operator->add_child(std::make_shared<const pyquboc::numeric_literal>(other));

        return add_operator;
      });

  py::class_<pyquboc::binary_variable, std::shared_ptr<pyquboc::binary_variable>, pyquboc::expression>(m, "Binary")
      .def(py::init<const std::string&>());

  py::class_<pyquboc::spin_variable, std::shared_ptr<pyquboc::spin_variable>, pyquboc::expression>(m, "Spin")
      .def(py::init<const std::string&>());

  py::class_<pyquboc::placeholder_variable, std::shared_ptr<pyquboc::placeholder_variable>, pyquboc::expression>(m, "Placeholder")
      .def(py::init<const std::string&>());

  py::class_<pyquboc::sub_hamiltonian, std::shared_ptr<pyquboc::sub_hamiltonian>, pyquboc::expression>(m, "SubH")
      .def(py::init<const std::shared_ptr<const pyquboc::expression>&, const std::string&>(), py::arg("hamiltonian"), py::arg("label"));

  py::class_<pyquboc::constraint, std::shared_ptr<pyquboc::constraint>, pyquboc::expression>(m, "Constraint")
      .def(py::init<const std::shared_ptr<const pyquboc::expression>&, const std::string&, const std::function<bool(double)>&>(), py::arg("hamiltonian"), py::arg("label"), py::arg("condition") = py::cpp_function([](double x) { return x == 0; }));

  py::class_<pyquboc::with_penalty, std::shared_ptr<pyquboc::with_penalty>, pyquboc::expression>(m, "WithPenalty")
      .def(py::init<const std::shared_ptr<const pyquboc::expression>&, const std::shared_ptr<const pyquboc::expression>&, const std::string&>());

  py::class_<pyquboc::user_defined_expression, std::shared_ptr<pyquboc::user_defined_expression>, pyquboc::expression>(m, "UserDefinedExpress")
      .def(py::init<const std::shared_ptr<const pyquboc::expression>&>());

  py::class_<pyquboc::numeric_literal, std::shared_ptr<pyquboc::numeric_literal>, pyquboc::expression>(m, "Num")
      .def(py::init<double>());

  py::class_<pyquboc::solution>(m, "DecodedSample")
      .def_property_readonly("sample", &pyquboc::solution::sample)
      .def_property_readonly("energy", &pyquboc::solution::energy)
      .def_property_readonly("subh", [](const pyquboc::solution& solution) {
        auto result = solution.sub_hamiltonians();

        std::transform(std::begin(solution.constraints()), std::end(solution.constraints()), std::inserter(result, std::begin(result)), [](const auto& constraint) {
          return std::pair{constraint.first, constraint.second.second};
        });

        return result;
      })
      .def(
          "constraints", [](const pyquboc::solution& solution, bool only_broken) {
            auto constraints = solution.constraints();

            if (only_broken) {
              return [&]() {
                auto result = std::unordered_map<std::string, std::pair<bool, double>>{};

                for (const auto& [name, value] : constraints) {
                  const auto& [not_broken, energy] = value;

                  if (not_broken) {
                    continue;
                  }

                  result.emplace(name, value);
                }

                return result;
              }();
            }

            return constraints;
          },
          py::arg("only_broken"))
      .def("array", [](const pyquboc::solution& solution, const std::string& name, int index) {
        return solution.sample().at(name + "[" + std::to_string(index) + "]");
      })
      .def("array", [](const pyquboc::solution& solution, const std::string& name, const py::tuple& indexes) {
        const auto name_and_indexes = [&]() {
          auto result = name;

          for (const auto& index : indexes.cast<std::vector<int>>()) {
            result += "[" + std::to_string(index) + "]";
          }

          return result;
        }();

        return solution.sample().at(name_and_indexes);
      });

  // TODO: index_labelに対応する。でも、この機能が必要な理由が分からなくて、しかもコードが汚くなるので、やる気が出ない……。

  py::class_<pyquboc::model>(m, "Model")
      .def(
          "to_bqm", [](const pyquboc::model& model, bool index_label, const std::unordered_map<std::string, double>& feed_dict) {
            const auto [linear, quadratic, offset] = model.to_bqm_parameters(feed_dict);

            return py::module::import("dimod").attr("BinaryQuadraticModel")(linear, quadratic, offset, py::module::import("dimod").attr("Vartype").attr("BINARY")); // dimodのPythonのBinaryQuadraticModelを作成します。cimodのPythonのBinaryQuadraticModelだと、dwave-nealで通らなかった……。
          },
          py::arg("index_label") = false, py::arg("feed_dict") = std::unordered_map<std::string, double>{})
      .def(
          "to_qubo", [](const pyquboc::model& model, bool index_label, const std::unordered_map<std::string, double>& feed_dict) {
            return model.to_bqm(feed_dict, cimod::Vartype::BINARY).to_qubo();

            // cimodのto_qubo()だとテストを通らない。。。cimodは、係数が0の場合はlinearから削除するみたい。これで良いような気もするけど、どうなんだろ？　Python上でdimodにやらせると遅くてベンチマークが悲惨な結果になるので、とりあえずこのままで。

            // const auto [linear, quadratic, offset] = model.to_bqm_parameters(feed_dict);
            // return py::module::import("dimod").attr("BinaryQuadraticModel")(linear, quadratic, offset, py::module::import("dimod").attr("Vartype").attr("BINARY")).attr("to_qubo")();
          },
          py::arg("index_label") = false, py::arg("feed_dict") = std::unordered_map<std::string, double>{})
      .def(
          "to_ising", [](const pyquboc::model& model, bool index_label, const std::unordered_map<std::string, double>& feed_dict) {
            return model.to_bqm(feed_dict, cimod::Vartype::BINARY).to_ising();
          },
          py::arg("index_label") = false, py::arg("feed_dict") = std::unordered_map<std::string, double>{})
      .def("energy", &pyquboc::model::energy, py::arg("sample"), py::arg("vartype"), py::arg("feed_dict") = std::unordered_map<std::string, double>{})
      .def("decode_sample", &pyquboc::model::decode_sample, py::arg("sample"), py::arg("vartype"), py::arg("feed_dict") = std::unordered_map<std::string, double>{})
      .def(
          "decode_sampleset", [](const pyquboc::model& model, const py::object& sampleset, const std::unordered_map<std::string, double>& feed_dict) {
            const auto variables = sampleset.attr("variables").cast<std::vector<std::string>>();

            sampleset.attr("record").attr("sort")("order"_a = "energy");

            const auto array = sampleset.attr("record")["sample"].cast<py::array_t<std::int8_t>>();
            const auto info = array.request();

            if (info.format != py::format_descriptor<int8_t>::format() || info.ndim != 2) {
              throw std::runtime_error("Incompatible buffer format!");
            }

            const auto samples = [&]() {
              auto result = std::vector<std::unordered_map<std::string, int>>(info.shape[0]);

              for (auto i = 0; i < info.shape[0]; ++i) {
                for (auto j = 0; j < info.shape[1]; ++j) {
                  result[i].emplace(variables[j], *array.data(i, j));
                }
              }

              return result;
            }();

            return model.decode_samples(samples, sampleset.attr("vartype").attr("name").cast<std::string>(), feed_dict);
          },
          py::arg("sampleset"), py::arg("feed_dict") = std::unordered_map<std::string, double>{});
}
