#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include "string_utils.hpp"

using namespace duckdb;

TEST_CASE("TrimString - basic trimming", "[string_utils]") {
	REQUIRE(TrimString("  hello  ") == "hello");
	REQUIRE(TrimString("hello") == "hello");
	REQUIRE(TrimString("  hello") == "hello");
	REQUIRE(TrimString("hello  ") == "hello");
}

TEST_CASE("TrimString - whitespace characters", "[string_utils]") {
	REQUIRE(TrimString("  \t\n\r  hello  \t\n\r  ") == "hello");
	REQUIRE(TrimString("\t\n\r") == "");
	REQUIRE(TrimString("  \t\n\r  ") == "");
}

TEST_CASE("TrimString - empty and whitespace-only strings", "[string_utils]") {
	REQUIRE(TrimString("") == "");
	REQUIRE(TrimString("   ") == "");
	REQUIRE(TrimString("\t") == "");
	REQUIRE(TrimString("\n") == "");
	REQUIRE(TrimString("\r") == "");
	REQUIRE(TrimString(" \t\n\r ") == "");
}

TEST_CASE("TrimString - no trimming needed", "[string_utils]") {
	REQUIRE(TrimString("hello world") == "hello world");
	REQUIRE(TrimString("a") == "a");
	REQUIRE(TrimString("test123") == "test123");
}

TEST_CASE("RemoveQuotes - basic quote removal", "[string_utils]") {
	REQUIRE(RemoveQuotes("\"hello\"") == "hello");
	REQUIRE(RemoveQuotes("\"test\"") == "test");
	REQUIRE(RemoveQuotes("\"\"") == "");
}

TEST_CASE("RemoveQuotes - with whitespace", "[string_utils]") {
	REQUIRE(RemoveQuotes("  \"hello\"  ") == "hello");
	REQUIRE(RemoveQuotes("  \"test\"  ") == "test");
	REQUIRE(RemoveQuotes("\t\"hello\"\t") == "hello");
}

TEST_CASE("RemoveQuotes - no quotes", "[string_utils]") {
	REQUIRE(RemoveQuotes("hello") == "hello");
	REQUIRE(RemoveQuotes("  hello  ") == "hello");
	REQUIRE(RemoveQuotes("test") == "test");
}

TEST_CASE("RemoveQuotes - single quote", "[string_utils]") {
	REQUIRE(RemoveQuotes("\"") == "\"");
	REQUIRE(RemoveQuotes("  \"  ") == "\"");
}

TEST_CASE("RemoveQuotes - mismatched quotes", "[string_utils]") {
	REQUIRE(RemoveQuotes("\"hello") == "\"hello");
	REQUIRE(RemoveQuotes("hello\"") == "hello\"");
	REQUIRE(RemoveQuotes("'hello'") == "'hello'");
}

TEST_CASE("RemoveQuotes - empty string", "[string_utils]") {
	REQUIRE(RemoveQuotes("") == "");
	REQUIRE(RemoveQuotes("   ") == "");
	REQUIRE(RemoveQuotes("\"\"") == "");
}

TEST_CASE("RemoveQuotes - nested quotes", "[string_utils]") {
	REQUIRE(RemoveQuotes("\"hello\\\"world\"") == "hello\\\"world");
	REQUIRE(RemoveQuotes("\"test\"value\"") == "test\"value");
}

int main(int argc, char *argv[]) {
	return Catch::Session().run(argc, argv);
}
