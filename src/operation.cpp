#include "operation.h"

#include <sstream>

namespace exactextract {

template<typename T>
std::string
make_field_name(const std::string& prefix, const T& value)
{
    if constexpr (std::is_floating_point_v<T>) {
        std::stringstream ss;
        ss << prefix << value;
        return ss.str();
    } else {
        return prefix + std::to_string(value);
    }
}

Operation::missing_value_t
Operation::get_missing_value()
{
    const auto& empty_rast = values->read_empty();

    return std::visit([](const auto& r) -> missing_value_t {
        if (r->has_nodata()) {
            return r->nodata();
        }
        return std::numeric_limits<double>::quiet_NaN();
    },
                      empty_rast);
}

const StatsRegistry::RasterStatsVariant&
Operation::empty_stats() const
{
    static const StatsRegistry::RasterStatsVariant ret = std::visit([this](const auto& r) {
        using value_type = typename std::remove_reference_t<decltype(*r)>::value_type;

        return StatsRegistry::RasterStatsVariant{
            RasterStats<value_type>()
        };
    },
                                                                    values->read_empty());

    return ret;
}

void
Operation::set_result(const StatsRegistry& reg, const Feature& f_in, Feature& f_out) const
{
    constexpr bool write_if_missing = true;
    // should we set attribute values if the feature did not intersect the raster?
    if (!write_if_missing && !reg.contains(f_in, *this)) {
        return;
    }

    const auto& stats = reg.contains(f_in, *this) ? reg.stats(f_in, *this) : empty_stats();

    set_result(stats, f_out);
}

void
Operation::set_empty_result(Feature& f_out) const
{
    set_result(empty_stats(), f_out);
}

void
Operation::set_result(const StatsRegistry::RasterStatsVariant& stats, Feature& f_out) const
{
    if (stat == "mean") {
        std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.mean()); }, stats);
    } else if (stat == "sum") {
        std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.sum()); }, stats);
    } else if (stat == "count") {
        std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.count()); }, stats);
    } else if (stat == "weighted_mean") {
        std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.weighted_mean()); }, stats);
    } else if (stat == "weighted_sum") {
        std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.weighted_sum()); }, stats);
    } else if (stat == "min") {
        std::visit([&f_out, this](const auto& x, const auto& m) { f_out.set(m_field_names[0], x.min().value_or(m)); },
                   stats,
                   m_missing);
    } else if (stat == "max") {
        std::visit([&f_out, this](const auto& x, const auto& m) { f_out.set(m_field_names[0], x.max().value_or(m)); },
                   stats,
                   m_missing);
    } else if (stat == "majority" || stat == "mode") {
        std::visit(
          [&f_out, this](const auto& x, const auto& m) { f_out.set(m_field_names[0], x.mode().value_or(m)); },
          stats,
          m_missing);
    } else if (stat == "minority") {
        std::visit([&f_out, this](const auto& x, const auto& m) {
            f_out.set(m_field_names[0], x.minority().value_or(m));
        },
                   stats,
                   m_missing);
    } else if (stat == "variety") {
        std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.variety()); }, stats);
    } else if (stat == "stdev") {
        std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.stdev()); }, stats);
    } else if (stat == "weighted_stdev") {
        std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.weighted_stdev()); }, stats);
    } else if (stat == "variance") {
        std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.variance()); }, stats);
    } else if (stat == "weighted_variance") {
        std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.weighted_variance()); }, stats);
    } else if (stat == "coefficient_of_variation") {
        std::visit([&f_out, this](const auto& x) { f_out.set(m_field_names[0], x.coefficient_of_variation()); },
                   stats);
    } else if (stat == "median") {
        std::visit([&f_out, this](const auto& x, const auto& m) {
            f_out.set(m_field_names[0], x.quantile(0.5).value_or(m));
        },
                   stats,
                   m_missing);
    } else if (stat == "coverage") {
        std::visit([&f_out, this](const auto& s) { f_out.set(m_field_names[0], s.coverage_fractions()); }, stats);
    } else if (stat == "values") {
        std::visit([&f_out, this](const auto& s) { f_out.set(m_field_names[0], s.values()); }, stats);
    } else if (stat == "weights") {
        std::visit([&f_out, this](const auto& s) { f_out.set(m_field_names[0], s.weights()); }, stats);
    } else if (stat == "center_x") {
        std::visit([&f_out, this](const auto& s) { f_out.set(m_field_names[0], s.center_x()); }, stats);
    } else if (stat == "center_y") {
        std::visit([&f_out, this](const auto& s) { f_out.set(m_field_names[0], s.center_y()); }, stats);
    } else if (stat == "cell_id") {
        std::visit([&f_out, this](const auto& s) {
            const auto& x = s.center_x();
            const auto& y = s.center_y();
            std::vector<std::int64_t> cells(x.size());
            for (std::size_t i = 0; i < x.size(); i++) {
                cells[i] = static_cast<std::int64_t>(values->grid().get_cell(x[i], y[i]));
            }
            f_out.set(m_field_names[0], cells);
        },
                   stats);
    } else if (stat == "quantile") {
        std::visit([&f_out, this](const auto& x, const auto& m) {
            for (std::size_t i = 0; i < m_quantiles.size(); i++) {
                f_out.set(m_field_names[i], x.quantile(m_quantiles[i]).value_or(m));
            }
        },
                   stats,
                   m_missing);
    } else if (stat == "frac") {
        std::visit([&f_out](const auto& s) {
            for (const auto& value : s) {
                f_out.set(make_field_name("frac_", value), s.frac(value).value_or(0));
            }
        },
                   stats);
    } else if (stat == "weighted_frac") {
        std::visit([&f_out](const auto& s) {
            for (const auto& value : s) {
                f_out.set(make_field_name("weighted_frac_", value), s.weighted_frac(value).value_or(0));
            }
        },
                   stats);
    } else {
        throw std::runtime_error("Unhandled stat: " + stat);
    }
}

void
Operation::setQuantileFieldNames()
{
    for (double q : m_quantiles) {
        std::stringstream ss;
        int qint = static_cast<int>(100 * q);
        ss << "q_" << qint;
        m_field_names.push_back(ss.str());
    }
}
}
