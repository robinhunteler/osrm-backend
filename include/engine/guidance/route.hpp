#ifndef ROUTE_HPP
#define ROUTE_HPP

namespace osrm::engine::guidance
{

struct Route
{
    double distance;
    double duration;
    double weight;
    double energy_consumption;
};
} // namespace osrm::engine::guidance

#endif
