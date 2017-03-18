#pragma once

#include "base_communicator.hpp"

namespace nest {
namespace mc {
namespace communication {

template <typename Time, typename CommunicationPolicy>
class linear_communicator: public base_communicator<Time, CommunicationPolicy> {
public:
    using base = base_communicator<Time, CommunicationPolicy>;
    using spike_type = typename base::spike_type;
    using event_queue = typename base::event_queue;
    using gid_partition_type = typename base::gid_partition_type;
    
    using base::num_groups_local;
    
protected:
    using base::cell_group_index;
    using base::connections_;

public:
    linear_search_communicator(): base() {}

    explicit linear_search_communicator(gid_partition_type cell_gid_partition):
        base(cell_gid_partition)
    {}

    std::vector<event_queue> make_event_queues(const gathered_vector<spike_type>& global_spikes) {
        auto queues = std::vector<event_queue>(num_groups_local());
        for (auto spike : global_spikes.values()) {
            // search for targets
            auto targets = std::equal_range(connections_.begin(), connections_.end(), spike.source);

            // generate an event for each target
            for (auto it=targets.first; it!=targets.second; ++it) {
                auto gidx = cell_group_index(it->destination().gid);
                queues[gidx].push_back(it->make_event(spike));
            }
        }

        return queues;
    }
};

template <typename Time, typename CommunicationPolicy>
using communicator = linear_communicator<Time, CommunicationPolicy>;

}
}
}
