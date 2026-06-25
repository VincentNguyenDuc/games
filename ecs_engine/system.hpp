#pragma once

#include "command_buffer.hpp"
#include "registry.hpp"

#include <vector>

namespace ecse {

struct ISystem {
    std::vector<ComponentType> reads;
    std::vector<ComponentType> writes;
    virtual ~ISystem() = default;
    virtual void update(EntityComponentRegistry&, CommandBuffer&) = 0;
};

struct SystemNode {
    ISystem* system;
    int id;
    std::vector<int> depends_on;
    int in_degree;
};

using NodeWaves = std::vector<std::vector<int>>;

class SystemsScheduler {
private:
    std::vector<SystemNode> nodes;
    NodeWaves waves;
    void compute_waves();

public:
    SystemsScheduler() = default;
    void add_system(ISystem*);
    void build();
    const NodeWaves& get_waves() const;
};

} // namespace ecse
