#include "ecs.hpp"

#include <catch2/catch_test_macros.hpp>
#include <thread>

using namespace ecse;
#include <vector>

TEST_CASE("push and pop_blocking round-trip", "[event_queue]") {
    EventQueue<int> q;
    q.push(42);
    REQUIRE(q.pop_blocking() == 42);
}

TEST_CASE("events are popped in FIFO order", "[event_queue]") {
    EventQueue<int> q;
    q.push(1);
    q.push(2);
    q.push(3);

    REQUIRE(q.pop_blocking() == 1);
    REQUIRE(q.pop_blocking() == 2);
    REQUIRE(q.pop_blocking() == 3);
}

TEST_CASE("try_pop returns nullopt on empty queue", "[event_queue]") {
    EventQueue<int> q;
    REQUIRE(q.try_pop() == std::nullopt);
}

TEST_CASE("try_pop returns value when available", "[event_queue]") {
    EventQueue<int> q;
    q.push(99);
    REQUIRE(q.try_pop() == 99);
    REQUIRE(q.try_pop() == std::nullopt);
}

TEST_CASE("empty() reflects queue state", "[event_queue]") {
    EventQueue<int> q;
    REQUIRE(q.empty());
    q.push(1);
    REQUIRE(!q.empty());
    q.pop_blocking();
    REQUIRE(q.empty());
}

TEST_CASE("concurrent pushes from multiple threads are all received", "[event_queue]") {
    EventQueue<int> q;
    constexpr int n_threads = 8;
    constexpr int n_per_thread = 100;

    std::vector<std::thread> threads;
    for (int t = 0; t < n_threads; ++t)
        threads.emplace_back([&q, t] {
            for (int i = 0; i < n_per_thread; ++i)
                q.push(t * n_per_thread + i);
        });
    for (auto& t : threads)
        t.join();

    int count = 0;
    while (!q.empty()) {
        q.pop_blocking();
        ++count;
    }
    REQUIRE(count == n_threads * n_per_thread);
}
