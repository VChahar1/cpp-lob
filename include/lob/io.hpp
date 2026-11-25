#pragma once

#include <lob/events.hpp>

#include <iosfwd>
#include <optional>
#include <string>
#include <vector>
namespace lob {

std::optional<OrderEvent> parse_event_line(const std::string& line);

std::vector<OrderEvent> read_events(std::istream& in, std::size_t* skipped_out = nullptr);

void write_trade(std::ostream& out, const Trade& t);

}