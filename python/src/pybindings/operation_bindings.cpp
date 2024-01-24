// Copyright (c) 2023-2024 ISciences, LLC.
// All rights reserved.
//
// This software is licensed under the Apache License, Version 2.0 (the "License").
// You may not use this file except in compliance with the License. You may
// obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0.
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h> // needed to map dict to std::map in Operation::create

#include "feature.h"
#include "operation.h"
#include "operation_bindings.h"
#include "raster_source.h"
#include "utils.h"

namespace py = pybind11;

namespace exactextract {
py::type
as_python_type(Feature::ValueType typ)
{
    switch (typ) {
        case Feature::ValueType::DOUBLE:
            return py::type::of(py::float_(0.0));
        case Feature::ValueType::INT:
        case Feature::ValueType::INT64:
            return py::type::of(py::int_(0));
        case Feature::ValueType::STRING:
            return py::type::of(py::str(""));
        case Feature::ValueType::INT_ARRAY:
        case Feature::ValueType::INT64_ARRAY:
        case Feature::ValueType::DOUBLE_ARRAY:
            return py::type::of(py::array());
    }

    assert(false);
}

py::object
as_numpy_dtype(Feature::ValueType typ)
{
    py::module np = py::module_::import("numpy");

    switch (typ) {
        case Feature::ValueType::DOUBLE:
        case Feature::ValueType::INT:
        case Feature::ValueType::INT64:
        case Feature::ValueType::STRING:
            return py::none();
        case Feature::ValueType::INT_ARRAY:
            return np.attr("int32");
        case Feature::ValueType::INT64_ARRAY:
            return np.attr("int64");
        case Feature::ValueType::DOUBLE_ARRAY:
            return np.attr("float64");
    }

    assert(false);
}

void
bind_operation(py::module& m)
{
    py::class_<Operation>(m, "Operation")
      .def(py::init(&Operation::create))
      .def("grid", &Operation::grid)
      .def("weighted", &Operation::weighted)
      .def_property_readonly("result_type", [](const Operation& op) {
          return as_python_type(op.result_type());
      })
      .def_property_readonly("result_dtype", [](const Operation& op) {
          return as_numpy_dtype(op.result_type());
      })
      .def_readonly("stat", &Operation::stat)
      .def_readonly("name", &Operation::name)
      .def_readonly("values", &Operation::values)
      .def_readonly("weights", &Operation::weights);

    m.def("prepare_operations", py::overload_cast<const std::vector<std::string>&, const std::vector<RasterSource*>&, const std::vector<RasterSource*>&>(&prepare_operations));
}
}
