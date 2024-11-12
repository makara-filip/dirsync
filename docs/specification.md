# C++ project specification: Directory sync utility

Proposed project is a command-line tool that synchronizes directories
and files between two specified paths. It can be used to manage
back-ups without the need to copy everything recursively.

The tool should accept various options to specify the behavior:
- one-way vs two-way synchronization
- conflict resolution if any file has a newer version: overwrite, rename or skip
- deleting extra files (that are not in the reference tree),
- logging verbosity options
- dry running to check all changes before proceeding
- excluding patterns with exact filenames and wildcards
- usage of a stored config file for persistent configuration (see bellow)

Synchronized directories can (recursively) contain a local configuration file.
This includes keeping track of what files are excluded, too large 
to be copied to personal computers but kept in back-up media,
directory role (back-up, PC), storing the program version, etc.
The config file will be stored in a JSON format. For serialization and parsing,
an external C++ library will be used (specific library yet to be determined).

For file operations (copy/move/delete, getting timestamps, permissions),
the C++17 std::filesystem namespace will be used.
