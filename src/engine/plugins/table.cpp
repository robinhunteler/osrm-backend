#include "engine/plugins/table.hpp"

#include "engine/api/table_api.hpp"
#include "engine/api/table_parameters.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/string_util.hpp"

#include <cstdlib>

#include <vector>

#include <boost/assert.hpp>

namespace osrm::engine::plugins
{

TablePlugin::TablePlugin(const int max_locations_distance_table,
                         const std::optional<double> default_radius)
    : BasePlugin(default_radius), max_locations_distance_table(max_locations_distance_table)
{
}

Status TablePlugin::HandleRequest(const RoutingAlgorithmsInterface &algorithms,
                                  const api::TableParameters &params,
                                  osrm::engine::api::ResultT &result) const
{
    if (!algorithms.HasManyToManySearch())
    {
        return Error("NotImplemented",
                     "Many to many search is not implemented for the chosen search algorithm.",
                     result);
    }

    BOOST_ASSERT(params.IsValid());

    if (!CheckAllCoordinates(params.coordinates))
    {
        return Error("InvalidOptions", "Coordinates are invalid", result);
    }

    if (!params.bearings.empty() && params.coordinates.size() != params.bearings.size())
    {
        return Error(
            "InvalidOptions", "Number of bearings does not match number of coordinates", result);
    }

    // Empty sources or destinations means the user wants all of them included, respectively
    // The ManyToMany routing algorithm we dispatch to below already handles this perfectly.
    const auto num_sources =
        params.sources.empty() ? params.coordinates.size() : params.sources.size();
    const auto num_destinations =
        params.destinations.empty() ? params.coordinates.size() : params.destinations.size();

    if (max_locations_distance_table > 0 &&
        ((num_sources * num_destinations) >
         static_cast<std::size_t>(max_locations_distance_table * max_locations_distance_table)))
    {
        return Error("TooBig", "Too many table coordinates", result);
    }

    if (!CheckAlgorithms(params, algorithms, result))
        return Status::Error;

    const auto &facade = algorithms.GetFacade();
    auto phantom_nodes = GetPhantomNodes(facade, params);

    if (phantom_nodes.size() != params.coordinates.size())
    {
        return Error(
            "NoSegment", MissingPhantomErrorMessage(phantom_nodes, params.coordinates), result);
    }

    auto snapped_phantoms = SnapPhantomNodes(std::move(phantom_nodes));

    bool request_distance = params.annotations & api::TableParameters::AnnotationsType::Distance;
    bool request_duration = params.annotations & api::TableParameters::AnnotationsType::Duration;
    bool request_energy_consumption = params.annotations & api::TableParameters::AnnotationsType::EnergyConsumption;

    auto result_tables_tuple = algorithms.ManyToManySearch(
        snapped_phantoms, params.sources, params.destinations, request_distance);

    if ((request_duration && std::get<0>(result_tables_tuple).empty()) ||
        (request_distance && std::get<1>(result_tables_tuple).empty()) ||
        (request_energy_consumption && std::get<2>(result_tables_tuple).empty()))
    {
        return Error("NoTable", "No table found", result);
    }

    std::vector<api::TableAPI::TableCellRef> estimated_pairs;

    // Scan table for null results - if any exist, replace with distance estimates
    if (params.fallback_speed != from_alias<double>(INVALID_FALLBACK_SPEED) ||
        params.scale_factor != 1)
    {
        for (std::size_t row = 0; row < num_sources; row++)
        {
            for (std::size_t column = 0; column < num_destinations; column++)
            {
                const auto &table_index = row * num_destinations + column;
                BOOST_ASSERT(table_index < std::get<0>(result_tables_tuple).size());
                if (params.fallback_speed != from_alias<double>(INVALID_FALLBACK_SPEED) &&
                    params.fallback_speed > 0 &&
                    std::get<0>(result_tables_tuple)[table_index] == MAXIMAL_EDGE_DURATION)
                {
                    const auto &source =
                        snapped_phantoms[params.sources.empty() ? row : params.sources[row]];
                    const auto &destination =
                        snapped_phantoms[params.destinations.empty() ? column
                                                                     : params.destinations[column]];

                    auto distance_estimate =
                        params.fallback_coordinate_type ==
                                api::TableParameters::FallbackCoordinateType::Input
                            ? util::coordinate_calculation::greatCircleDistance(
                                  candidatesInputLocation(source),
                                  candidatesInputLocation(destination))
                            : util::coordinate_calculation::greatCircleDistance(
                                  candidatesSnappedLocation(source),
                                  candidatesSnappedLocation(destination));

                    std::get<0>(result_tables_tuple)[table_index] =
                        to_alias<EdgeDuration>(distance_estimate / params.fallback_speed);
                    if (!std::get<1>(result_tables_tuple).empty())
                    {
                        std::get<1>(result_tables_tuple)[table_index] =
                            to_alias<EdgeDistance>(distance_estimate);
                    }

                    estimated_pairs.emplace_back(row, column);
                }
                if (params.scale_factor > 0 && params.scale_factor != 1 &&
                    std::get<0>(result_tables_tuple)[table_index] != MAXIMAL_EDGE_DURATION &&
                    std::get<0>(result_tables_tuple)[table_index] != EdgeDuration{0})
                {
                    EdgeDuration diff =
                        MAXIMAL_EDGE_DURATION / std::get<0>(result_tables_tuple)[table_index];

                    if (params.scale_factor >= from_alias<double>(diff))
                    {
                        std::get<0>(result_tables_tuple)[table_index] =
                            MAXIMAL_EDGE_DURATION - EdgeDuration{1};
                    }
                    else
                    {
                        std::get<0>(result_tables_tuple)[table_index] = to_alias<EdgeDuration>(
                            std::lround(from_alias<double>(std::get<0>(result_tables_tuple)[table_index]) *
                                        params.scale_factor));
                    }
                }
            }
        }
    }

    api::TableAPI table_api{facade, params};
    table_api.MakeResponse(result_tables_tuple, snapped_phantoms, estimated_pairs, result);

    return Status::Ok;
}
} // namespace osrm::engine::plugins
