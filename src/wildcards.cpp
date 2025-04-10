#include "wildcards.hpp"

#include <string>

template<typename PatternIterator, typename TextIterator>
	requires (
		std::forward_iterator<PatternIterator>
		&& std::forward_iterator<TextIterator>
	)
bool wildcard_matches_impl(
	PatternIterator pattern_it,
	const PatternIterator &pattern_end,
	TextIterator text_it,
	const TextIterator &text_end
) {
	// or use string iterator type aliases in function signature???

	while (pattern_it != pattern_end && text_it != text_end) {
		if (*pattern_it == '*') {
			++pattern_it;
			if (pattern_it == pattern_end)
				// trailing wildcard
				return true;

			// try every possible match for the wildcard, starting with zero characters
			while (text_it != text_end) {
				if (wildcard_matches_impl(pattern_it, pattern_end, text_it, text_end))
					return true;
				++text_it;
			}

			return false;
		}

		// handle fixed letters in the pattern
		if (*pattern_it == *text_it) {
			++pattern_it;
			++text_it;
			continue;
		}

		return false;
	}

	// skip remaining '*'s in the pattern
	while (pattern_it != pattern_end && *pattern_it == '*') ++pattern_it;

	return pattern_it == pattern_end && text_it == text_end;
}

bool wildcard_matches(const std::string &pattern, const std::string &str) {
	return wildcard_matches_impl(
		pattern.begin(),
		pattern.end(),
		str.begin(),
		str.end()
	);
}
