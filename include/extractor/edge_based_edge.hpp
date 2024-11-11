#ifndef EDGE_BASED_EDGE_HPP
#define EDGE_BASED_EDGE_HPP

#include "extractor/travel_mode.hpp"
#include "util/typedefs.hpp"
#include <tuple>

namespace osrm::extractor
{

struct EdgeBasedEdge
{
  public:
    struct EdgeData
    {
        EdgeData()
            : turn_id(0), weight{0}, distance{0}, duration(0), energy_consumption{0}, forward(false), backward(false)
        {
        }

        EdgeData(const NodeID turn_id,
                 const EdgeWeight weight,
                 const EdgeDistance distance,
                 const EdgeDuration duration,
                 const EdgeEnergyConsumption energy_consumption,
                 const bool forward,
                 const bool backward)
            : turn_id(turn_id), weight(weight), distance(distance), duration(duration),
              energy_consumption(energy_consumption), forward(forward), backward(backward)
        {
        }

        NodeID turn_id; // ID of the edge based node (node based edge)
        EdgeWeight weight;
        EdgeDistance distance;
        EdgeEnergyConsumption energy_consumption;
        EdgeDuration::value_type duration : 30;
        std::uint32_t forward : 1;
        std::uint32_t backward : 1;

        auto is_unidirectional() const { return !forward || !backward; }
    };

    EdgeBasedEdge();
    template <class EdgeT> explicit EdgeBasedEdge(const EdgeT &other);
    EdgeBasedEdge(const NodeID source,
                  const NodeID target,
                  const NodeID edge_id,
                  const EdgeWeight weight,
                  const EdgeDuration duration,
                  const EdgeDistance distance,
                  const EdgeEnergyConsumption energy_consumption,
                  const bool forward,
                  const bool backward);
    EdgeBasedEdge(const NodeID source, const NodeID target, const EdgeBasedEdge::EdgeData &data);

    bool operator<(const EdgeBasedEdge &other) const;

    NodeID source;
    NodeID target;
    EdgeData data;
};
static_assert(sizeof(extractor::EdgeBasedEdge) == 28,
              "Size of extractor::EdgeBasedEdge type is "
              "bigger than expected. This will influence "
              "memory consumption.");

// Impl.

inline EdgeBasedEdge::EdgeBasedEdge() : source(0), target(0) {}

inline EdgeBasedEdge::EdgeBasedEdge(const NodeID source,
                                    const NodeID target,
                                    const NodeID edge_id,
                                    const EdgeWeight weight,
                                    const EdgeDuration duration,
                                    const EdgeDistance distance,
                                    const EdgeEnergyConsumption energy_consumption,
                                    const bool forward,
                                    const bool backward)
    : source(source), target(target), data{edge_id, weight, distance, duration, energy_consumption, forward, backward}
{
}

inline EdgeBasedEdge::EdgeBasedEdge(const NodeID source,
                                    const NodeID target,
                                    const EdgeBasedEdge::EdgeData &data)
    : source(source), target(target), data{data}
{
}

inline bool EdgeBasedEdge::operator<(const EdgeBasedEdge &other) const
{
    const auto unidirectional = data.is_unidirectional();
    const auto other_is_unidirectional = other.data.is_unidirectional();
    // if all items are the same, we want to keep bidirectional edges. due to the `<` operator,
    // preferring 0 (false) over 1 (true), we need to compare the inverse of `bidirectional`
    return std::tie(source, target, data.weight, unidirectional) <
           std::tie(other.source, other.target, other.data.weight, other_is_unidirectional);
}
} // namespace osrm::extractor

#endif /* EDGE_BASED_EDGE_HPP */
