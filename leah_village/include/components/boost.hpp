#pragma once

// Temporary production multiplier on a ResourceProducer (e.g. gem boost).
// ProduceSystem applies `rate * multiplier` while this component is present.
// BoostSystem counts down time_remaining_s and removes the component on expiry.
struct Boost {
    float multiplier;       // e.g. 2.0f for a 2× boost
    float time_remaining_s; // how long the boost lasts
};
