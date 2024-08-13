// Copyright (c) 2019-2024 ISciences, LLC.
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

#include "processor.h"
#include "map_feature.h"

namespace exactextract {

void
Processor::write_result(const Feature& f_in)
{
    if (m_unnest) {
        return write_result_unnested(f_in);
    }

    auto f_out = m_output.create_feature();
    if (m_include_geometry) {
        f_out->set_geometry(f_in.geometry());
    }
    for (const auto& col : m_include_cols) {
        f_out->set(col, f_in);
    }
    for (const auto& op : m_operations) {
        op->set_result(m_reg, f_in, *f_out);
    }
    m_output.write(*f_out);
    m_reg.flush_feature(f_in);
}

static std::size_t
field_length(const Feature& f, const std::string& field)
{
    auto typ = f.field_type(field);
    switch (typ) {
        case Feature::ValueType::STRING:
        case Feature::ValueType::DOUBLE:
        case Feature::ValueType::INT:
        case Feature::ValueType::INT64:
            return 1;
        case Feature::ValueType::DOUBLE_ARRAY:
            return f.get_double_array(field).size;
        case Feature::ValueType::INT_ARRAY:
            return f.get_integer_array(field).size;
        case Feature::ValueType::INT64_ARRAY:
            return f.get_integer64_array(field).size;
            break;
    }

    throw std::runtime_error("field_length: unhandled field type");
}

void
Processor::write_result_unnested(const Feature& f_in)
{
    MapFeature temp;
    for (const auto& op : m_operations) {
        op->set_result(m_reg, f_in, temp);
    }

    std::size_t n = 1;

    for (const auto& field : temp.fields()) {
        auto len = field_length(temp, std::string(field));
        if (len != 1) {
            if (n == 1) {
                n = len;
            } else if (n != len) {
                throw std::runtime_error("Inconsistent array lengths.");
            }
        }
    }

    for (std::size_t i = 0; i < n; i++) {
        auto f_out = m_output.create_feature();
        if (m_include_geometry) {
            f_out->set_geometry(f_in.geometry());
        }
        for (const auto& col : m_include_cols) {
            f_out->set(col, f_in);
        }

        for (const auto& op : m_operations) {
            auto typ = temp.field_type(op->name);

            switch (typ) {
                case Feature::ValueType::STRING:
                    f_out->set(op->name, temp.get_string(op->name));
                    break;
                case Feature::ValueType::DOUBLE:
                    f_out->set(op->name, temp.get_double(op->name));
                    break;
                case Feature::ValueType::INT:
                    f_out->set(op->name, temp.get_int(op->name));
                    break;
                case Feature::ValueType::INT64:
                    f_out->set(op->name, temp.get_int64(op->name));
                    break;
                case Feature::ValueType::INT_ARRAY:
                    f_out->set(op->name, temp.get_integer_array(op->name).data[i]);
                    break;
                case Feature::ValueType::INT64_ARRAY:
                    f_out->set(op->name, temp.get_integer64_array(op->name).data[i]);
                    break;
                case Feature::ValueType::DOUBLE_ARRAY:
                    f_out->set(op->name, temp.get_double_array(op->name).data[i]);
                    break;
            }
        }

        m_output.write(*f_out);
        m_reg.flush_feature(f_in);
    }
}

}
