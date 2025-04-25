#include "arguments.hpp"

#include <iostream>

bool ProgramArguments::try_parse(const std::vector<std::string> &arguments) {
	if (arguments.size() < 2) {
		std::cerr << "Error: Too few arguments." << std::endl;
		std::cerr << "Usage: dirsync [--help] [source-directory] [target-directory]" << std::endl;
		return false;
	}

	auto &&arg_iter = arguments.begin();
	executable = *(arg_iter++);
	for (; arg_iter != arguments.end(); ++arg_iter) {
		const std::string &argument = *arg_iter;

		const bool is_flag_or_param = argument.starts_with('-');
		if (!is_flag_or_param)
			break;

		if (argument == "-h" || argument == "--help") {
			mode = ProgramMode::help;
		} else if (argument == "--test") {
			mode = ProgramMode::test;
		} else if (argument == "--verbose") {
			verbose = true;
		} else if (argument == "--dry-run") {
			dry_run = true;
		} else if (argument == "--bi" || argument == "--bidirectional") {
			is_one_way_synchronization = false;
		} else if (argument == "-d" || argument == "--delete-extra") {
			delete_extra_target_files = true;
		} else if (argument == "-s" || argument == "--skip-existing" || argument == "--safe") {
			conflict_resolution = ConflictResolutionMode::skip;
		} else if (argument == "-r" || argument == "--rename") {
			conflict_resolution = ConflictResolutionMode::rename;
		} else if (argument == "--copy-configs" || argument == "--copy-configurations") {
			copy_configurations = true;
		}
	}

	// perform flag compatibility check
	if (!is_one_way_synchronization && delete_extra_target_files) {
		delete_extra_target_files = false;
		std::cerr << "Warning: --delete-extra is disabled, because it is incompatible with --bi|--bidirectional.\n";
	}

	if (mode == ProgramMode::help || mode == ProgramMode::test) {
		if (arg_iter != arguments.end())
			std::cerr << "Warning: ignoring specified positional arguments." << std::endl;
		return true;
	}

	if (arg_iter == arguments.end()) {
		std::cerr << "Error: source directory unspecified." << std::endl;
		return false;
	}
	source_directory = *(arg_iter++);
	if (arg_iter == arguments.end()) {
		std::cerr << "Error: target directory unspecified." << std::endl;
		return false;
	}
	target_directory = *(arg_iter++);

	if (arg_iter != arguments.end()) {
		std::cerr << "Warning: Extra parameters ignored." << std::endl;
	}
	return true;
}

const char *flag_to_string(const bool enabled) {
	return enabled ? "enabled" : "disabled";
}

const char *string_or_empty(const std::string &str) {
	if (str.empty())
		return "(empty)";
	return str.c_str();
}

void ProgramArguments::print(std::ostream &stream) const {
	stream << "Flags: " << std::endl;
	stream << "    help: " << flag_to_string(mode == ProgramMode::help)
		<< std::endl;
	stream << "    verbose: " << flag_to_string(verbose) << std::endl;
	stream << "    dry run: " << flag_to_string(dry_run) << std::endl;
	stream << "Source dir: " << string_or_empty(source_directory) << std::endl;
	stream << "Target dir: " << string_or_empty(target_directory) << std::endl;
}
