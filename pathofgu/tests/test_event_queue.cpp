#include <catch2/catch_test_macros.hpp>

#include "events/event_queue.hpp"

TEST_CASE("EventQueue can push and pop events", "[event_queue]") {
    EventQueue<int> queue;

    SECTION("Push and pop a single event") {
        queue.push(42);
        REQUIRE(queue.pop_blocking() == 42);
    }

    SECTION("Push multiple events and pop them in order") {
        queue.push(1);
        queue.push(2);
        queue.push(3);

        REQUIRE(queue.pop_blocking() == 1);
        REQUIRE(queue.pop_blocking() == 2);
        REQUIRE(queue.pop_blocking() == 3);
    }

    SECTION("Pop from an empty queue returns nullopt") { REQUIRE(queue.try_pop() == std::nullopt); }
}