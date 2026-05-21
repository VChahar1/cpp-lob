#include <lob/io.hpp>
#include <lob/events.hpp>

#include <catch.hpp>
#include <sstream>

using namespace lob;

TEST_CASE("Parse NEW event", "[io]") {
    auto ev = parse_event_line("NEW 42 B 10000 5");
    REQUIRE(ev.has_value());
    REQUIRE(std::holds_alternative<NewOrder>(*ev));
    const auto& n = std::get<NewOrder>(*ev);
    REQUIRE(n.id == 42);
    REQUIRE(n.side == Side::Bid);
    REQUIRE(n.price == 10000);
    REQUIRE(n.quantity == 5);
}

TEST_CASE("Parse NEW with sell side", "[io]") {
    auto ev = parse_event_line("NEW 1 S 10100 3");
    REQUIRE(ev.has_value());
    REQUIRE(std::get<NewOrder>(*ev).side == Side::Ask);
}

TEST_CASE("Parse CANCEL event", "[io]") {
    auto ev = parse_event_line("CANCEL 7");
    REQUIRE(ev.has_value());
    REQUIRE(std::holds_alternative<Cancel>(*ev));
    REQUIRE(std::get<Cancel>(*ev).id == 7);
}

TEST_CASE("Parse MOD event", "[io]") {
    auto ev = parse_event_line("MOD 3 10050 8");
    REQUIRE(ev.has_value());
    REQUIRE(std::holds_alternative<Modify>(*ev));
    const auto& m = std::get<Modify>(*ev);
    REQUIRE(m.id == 3);
    REQUIRE(m.new_price == 10050);
    REQUIRE(m.new_quantity == 8);
}

TEST_CASE("Blank lines and comments are skipped", "[io]") {
    REQUIRE_FALSE(parse_event_line("").has_value());
    REQUIRE_FALSE(parse_event_line("   ").has_value());
    REQUIRE_FALSE(parse_event_line("# comment").has_value());
    REQUIRE_FALSE(parse_event_line("   # indented comment").has_value());
}

TEST_CASE("Malformed lines return nullopt", "[io]") {
    REQUIRE_FALSE(parse_event_line("FOOBAR").has_value());
    REQUIRE_FALSE(parse_event_line("NEW 1").has_value());         // too few fields
    REQUIRE_FALSE(parse_event_line("NEW 1 X 10000 5").has_value()); // bad side
    REQUIRE_FALSE(parse_event_line("NEW abc B 10000 5").has_value()); // non-numeric id
}

TEST_CASE("read_events handles a multi-line stream", "[io]") {
    std::istringstream iss(
        "NEW 1 B 10000 5\n"
        "# this is a comment\n"
        "\n"
        "NEW 2 S 10100 3\n"
        "CANCEL 1\n"
    );
    std::size_t skipped = 0;
    auto events = read_events(iss, &skipped);
    REQUIRE(events.size() == 3);
    REQUIRE(skipped == 0);
    REQUIRE(std::holds_alternative<NewOrder>(events[0]));
    REQUIRE(std::holds_alternative<NewOrder>(events[1]));
    REQUIRE(std::holds_alternative<Cancel>(events[2]));
}

TEST_CASE("read_events counts skipped malformed lines", "[io]") {
    std::istringstream iss(
        "NEW 1 B 10000 5\n"
        "garbage line\n"
        "NEW 2 S 10100 3\n"
    );
    std::size_t skipped = 0;
    auto events = read_events(iss, &skipped);
    REQUIRE(events.size() == 2);
    REQUIRE(skipped == 1);
}

TEST_CASE("write_trade emits canonical format", "[io]") {
    std::ostringstream oss;
    write_trade(oss, Trade{1, 2, 10100, 5, 0});
    REQUIRE(oss.str() == "TRADE 1 2 10100 5\n");
}