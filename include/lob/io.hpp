#pragma once

#include <lob/events.hpp>

#include <iosfwd>
#include <optional>
#include <string>
#include <vector>
namespace lob {

// Parse one line of the simple text format. Returns std::nullopt for
// blank lines, comment lines, or unparseable input. The line should NOT
// include a trailing newline.
//
// Format:
//   NEW    <id> <B|S> <price> <quantity>
//   CANCEL <id>
//   MOD    <id> <new_price> <new_quantity>
//   # comment ...
std::optional<OrderEvent> parse_event_line(const std::string& line);

// Read events from an input stream. Stops at EOF. Lines that fail to
// parse are silently skipped (counted via `skipped_out` if non-null).
std::vector<OrderEvent> read_events(std::istream& in, std::size_t* skipped_out = nullptr);

// Write a Trade in the canonical "TRADE <buyer> <seller> <price> <qty>" form,
// followed by a newline.
void write_trade(std::ostream& out, const Trade& t);

}  // namespace lob