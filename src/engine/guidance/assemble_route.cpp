#include "engine/guidance/assemble_route.hpp"

#include <numeric>

namespace osrm::engine::guidance
{

Route assembleRoute(const std::vector<RouteLeg> &route_legs)
{
    auto distance =
        std::accumulate(route_legs.begin(),
                        route_legs.end(),
                        0.,
                        [](const double sum, const RouteLeg &leg) { return sum + leg.distance; });
    auto duration =
        std::accumulate(route_legs.begin(),
                        route_legs.end(),
                        0.,
                        [](const double sum, const RouteLeg &leg) { return sum + leg.duration; });
    auto weight =
        std::accumulate(route_legs.begin(),
                        route_legs.end(),
                        0.,
                        [](const double sum, const RouteLeg &leg) { return sum + leg.weight; });

    auto energy_consumption = 
        std::accumulate(route_legs.begin(),
                        route_legs.end(),
                        0.,
                        [](const double sum, const RouteLeg &leg) { return sum + leg.energy_consumption; });

    return Route{distance, duration, weight, energy_consumption};
}

} // namespace osrm::engine::guidance
