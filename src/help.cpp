#include "help.hpp"

#include <iostream>

const char *HELP =
	"Usage: dirsync [OPTIONS] <source-directory> <target-directory>\n"
	"\n"
	"The dirsync utility recursively synchronizes or copies files from the source\n"
	"directory to the target directory.\n"
	"OPTIONS:\n"
	"-h, --help:	Display help information and exit. No positional arguments are needed.\n"
	"--verbose:	Output detailed information during synchronization (files copied, skipped, etc.).\n"
	"--dry-run:	Simulate the synchronization without actually copying or deleting files. May be useful with --verbose.\n"
	"--bi, --bidirectional:	Perform two-way synchronization (both source and target may be updated).\n"
	"-d, --delete-extra:	Deletes extra files and folders in the target directory that do not exist in the source directory. This flag is disabled with a warning when running two-way synchronization.\n"
	"-s, --skip-existing, --safe:	Skip copying files that are already in their respective destination.\n"
	"-r, --rename:	Use renaming conflict strategy: copy the source content to a new file with appended \"last write\" timestamp in the filename, using -YYYY-MM-DD-hh-mm-ss suffix format. File extension is kept.\n"
	"--copy-configs, --copy-configurations:	Copy directory configuration files themselves, if encountered.\n"
	"--test:	Runs implementation tests. Used by developers and testers.\n";

void print_help() {
	std::cout << HELP << std::endl;
}
