#include "memory_unit_util.hpp"

#include "duckdb/common/exception.hpp"

namespace duckdb {

uint64_t ConvertBytes(uint64_t bytes, MemoryUnit unit) {
	switch (unit) {
	case MemoryUnit::BYTES:
		return bytes;
	case MemoryUnit::KB:
		return bytes / 1000;
	case MemoryUnit::KiB:
		return bytes / 1024;
	case MemoryUnit::MB:
		return bytes / (1000ULL * 1000);
	case MemoryUnit::MiB:
		return bytes / (1024ULL * 1024);
	case MemoryUnit::GB:
		return bytes / (1000ULL * 1000 * 1000);
	case MemoryUnit::GiB:
		return bytes / (1024ULL * 1024 * 1024);
	case MemoryUnit::TB:
		return bytes / (1000ULL * 1000 * 1000 * 1000);
	case MemoryUnit::TiB:
		return bytes / (1024ULL * 1024 * 1024 * 1024);
	default:
		return bytes;
	}
}

MemoryUnit ParseUnit(const string &unit_str) {
	string lower_unit = StringUtil::Lower(unit_str);
	if (lower_unit == "bytes" || lower_unit == "b") {
		return MemoryUnit::BYTES;
	}
	if (lower_unit == "kb") {
		return MemoryUnit::KB;
	}
	if (lower_unit == "kib") {
		return MemoryUnit::KiB;
	}
	if (lower_unit == "mb") {
		return MemoryUnit::MB;
	}
	if (lower_unit == "mib") {
		return MemoryUnit::MiB;
	}
	if (lower_unit == "gb") {
		return MemoryUnit::GB;
	}
	if (lower_unit == "gib") {
		return MemoryUnit::GiB;
	}
	if (lower_unit == "tb") {
		return MemoryUnit::TB;
	}
	if (lower_unit == "tib") {
		return MemoryUnit::TiB;
	}
	throw InvalidInputException("Invalid unit '%s'. Supported units: bytes, KB, KiB, MB, MiB, GB, GiB, TB, TiB",
	                            unit_str);
}

} // namespace duckdb
