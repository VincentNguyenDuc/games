#include "detail/system.hpp"

static bool conflicts(const ISystem& a, const ISystem& b) {
    for (const auto& w : a.writes) {
        for (const auto& r : b.reads)
            if (w == r)
                return true;
        for (const auto& bw : b.writes)
            if (w == bw)
                return true;
    }
    for (const auto& w : b.writes)
        for (const auto& r : a.reads)
            if (w == r)
                return true;
    return false;
}

void SystemsScheduler::add_system(ISystem* system) {
    int id = static_cast<int>(nodes.size());
    nodes.push_back({system, id, {}, 0});
}

void SystemsScheduler::build() {
    int n = static_cast<int>(nodes.size());
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (conflicts(*nodes[i].system, *nodes[j].system)) {
                nodes[j].depends_on.push_back(i);
                nodes[j].in_degree++;
            }
        }
    }
    compute_waves();
}

void SystemsScheduler::compute_waves() {
    int n = static_cast<int>(nodes.size());

    std::vector<int> in_degree(n);
    for (int i = 0; i < n; i++)
        in_degree[i] = nodes[i].in_degree;

    std::vector<std::vector<int>> successors(n);
    for (int i = 0; i < n; i++)
        for (int dep : nodes[i].depends_on)
            successors[dep].push_back(i);

    waves.clear();
    std::vector<int> ready;
    for (int i = 0; i < n; i++)
        if (in_degree[i] == 0)
            ready.push_back(i);

    while (!ready.empty()) {
        waves.push_back(ready);
        std::vector<int> next;
        for (int id : ready)
            for (int succ : successors[id])
                if (--in_degree[succ] == 0)
                    next.push_back(succ);
        ready = next;
    }
}

const NodeWaves& SystemsScheduler::get_waves() const { return waves; }
