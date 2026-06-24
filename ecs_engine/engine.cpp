#include "engine.hpp"

namespace ecse {

void Engine::tick() {
    for (const auto& wave : scheduler_.get_waves()) {
        for (int idx : wave) {
            pool_.submit([this, idx] { systems_[idx]->update(registry_, cmd_buffers_[idx]); });
        }
        pool_.wait();
        for (int idx : wave)
            cmd_buffers_[idx].flush(registry_, entities_);
    }
}

} // namespace ecse
