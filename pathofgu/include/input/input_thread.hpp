#pragma once

#include "events/event_queue.hpp"
#include "events/events.hpp"

#include <optional>
#include <string>
#include <thread>

// Parses a raw input line into a PlayerCommand.
// Returns nullopt for unrecognised input.
std::optional<PlayerCommand> parse_command(const std::string& line);

// Launches a jthread that reads stdin and pushes PlayerCommands to the queue.
// Stops automatically when the stop_token is requested or stdin closes.
std::jthread launch_input_thread(EventQueue<PlayerCommand>& queue);
