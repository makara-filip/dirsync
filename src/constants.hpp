#ifndef DIRSYNC_CONSTANTS_HPP
#define DIRSYNC_CONSTANTS_HPP

#include <iostream>
#include "json.hpp"

constexpr int EXIT_CODE_INCORRECT_USAGE = 1;
constexpr int EXIT_CODE_NONEXISTENT_SOURCE_DIRECTORY = 2;
constexpr int EXIT_CODE_FILESYSTEM_ERROR = 3;
constexpr int EXIT_CODE_CONFIG_FILE_PARSE_ERROR = 4;
constexpr int EXIT_CODE_CONFIG_VERSION_INCOMPATIBLE = 5;
constexpr int EXIT_CODE_INCOMPATIBLE_ENTRIES = 6;

class Version {
	size_t major = 0;
	size_t minor = 0;
	size_t patch = 0;

	// expose private members for automatic JSON serialization/parsing
	using Json = nlohmann::json;
	friend void to_json(Json &json, const Version &parsed);
	friend void from_json(const Json &json, Version &parsed);

	public:
	constexpr Version() {}
	constexpr Version(const size_t major, const size_t minor, const size_t patch)
		: major(major), minor(minor), patch(patch) {}

	std::strong_ordering operator<=>(const Version &) const = default;
	// default comparisons: https://en.cppreference.com/w/cpp/language/default_comparisons

	bool is_compatible_with(const Version &other) const {
		if (major != other.major) return false; // breaking changes

		// if major == 0 (unstable, pre-release), semantic versioning is stricter
		if (major == 0) return *this == other;

		return *this <= other;
	}

	std::ostream &operator<<(std::ostream &stream) const {
		stream << major << '.' << minor << '.' << patch;
		return stream;
	}
};

constexpr Version PROGRAM_VERSION(0, 0, 0);

#endif // DIRSYNC_CONSTANTS_HPP
