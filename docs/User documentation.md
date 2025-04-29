# User documentation: dirsync

Dirsync is a command-line utility for synchronizing files across directories,
either one-way from the source directory to the target one, or two-way,
synchronizing symmetrically across two source directories.
The program supports detailed control over conflict resolution, verbosity, dry-running.
Additional directory-specific configuration is supported, saved in the directories themselves
in a config file.

## Minimum software and hardware requirements

The following operating system versions are supported (and tested):

- Windows 11 (Windows 10 functionality is not tested),
- Linux: most popular and modern distributions, tested on Ubuntu 22.04.5 LTS,
- macOS 15 (Sequoia) or later

For build and installation, [CMake build system](https://cmake.org)
and a C++ compiler are used. Check your installed versions:

- CMake 3.29.3 or later
- C++ compiler: clang version 17 and or later OR gcc version 14 or later, with formatting and filesystem support

The hardware requirements:

- CPU: any modern processor (even low-end or embedded CPUs)
- architecture: works on both 32-bit and 64-bit systems, 64-bit is preferred
- RAM: ~50 MB free memory, depends on the number of files being processed
- storage: sufficient space for synchronized files
- file system: must be directory-based, all FAT, NTFS, ext,
  Apple-based filesystems are supported

## Installation

There is a shell script `install.sh` for source code compilation.
Run it inside the `project` directory. Note that the executable binary
is not added to the PATH variable.
If you want the project to be more accessible, move it to your executables
directory, e.g. `/usr/local/bin`. This action may require superuser privileges.

## Directory configuration

A local file can configure each directory's sync behavior,
mainly for excluding files and directories to be excluded.

The configuration is stored in the local `.dirsync.json` file in the JSON
format. The scheme is explained bellow:

```json
{
  // used for program & file compatibility check
  "configVersion": {
    "major": 0,
    "minor": 0,
    "patch": 0
  },
  // these files and directories are not synced
  "excludedPatterns": [
    "*.log",
    "logs",
    "ignored-example.txt",
    "log-*.*",
    "*.png"
  ],
  // max file size in bytes to be copied to the configured directory
  "maxFileSize": 2000000
  // ~2 MB
}
```

The configuration applies in the currently configured directory
AND recursively in directories located in that directory.

For example, if the source directory contains such JSON file,
files `source/monday.log` and `source/any1/any2/any3/ignored-example.txt`
are not synchronized. Likewise, if a large file of 5 MB is bidirectionally
synchronized to this directory, it is skipped, because this directory
does not accept large files.

## Usage

```
dirsync --help

dirsync [OPTIONS] <source-directory> <target-directory>

dirsync --bidirectional [OPTIONS] <source-first> <source-second>
```

There are two required positional arguments, unless `--help` or `--test` is specified.
They represent two directories to be synchronized. If two-way synchronization
is enabled by `--bi|--bidirectional` flag, both are treated as source directories.
When running one-way synchronization, first one is source one, second is target.

The options are specified in the table bellow. If not specified, no special behaviour occurs.

| Flag                                      | Description                                                                                                                                                                                     |
|-------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `-h`, `--help`                            | Display help information and exit. No positional arguments are needed.                                                                                                                          |
| `--verbose`                               | Output detailed information during synchronization (files copied, skipped, etc.).                                                                                                               |
| `--dry-run`                               | Simulate the synchronization without actually copying or deleting files. May be useful with `--verbose`.                                                                                        |
| `--bi`, `--bidirectional`                 | Perform two-way synchronization (both source and target may be updated).                                                                                                                        |
| `-d`, `--delete-extra`                    | Deletes extra files and folders in the target directory that do not exist in the source directory. This flag is disabled with a warning when running two-way synchronization.                   |
| `-s`, `--skip-existing`, `--safe`         | Skip copying files that are already in their respective destination.                                                                                                                            |
| `-r`, `--rename`                          | Use renaming conflict strategy: copy the source content to a new file with appended "last write" timestamp in the filename, using `-YYYY-MM-DD-hh-mm-ss` suffix format. File extension is kept. |
| `--copy-configs`, `--copy-configurations` | Copy directory configuration files themselves, if encountered.                                                                                                                                  |
| `--test`                                  | Runs implementation tests. Used by developers and testers.                                                                                                                                      |

## Conflict resolution strategies

The default filename conflict strategy is overriding files with newer version
and skip copying if the source directory contains older version than the target.
The program determines the file age (or version) by the file last write time.

When using `-s|--skip-existing|--safe` flag, the conflicts are avoided
by not copying any files.

When using `-r|--rename`, the original files are kept unmodified.
If their respective last write times are not equivalent, the source file contents
are copied to a new file with filename of suffixing the source last write time
in the `YYYY-MM-DD-hh-mm-ss` format. For example, when both `source/example.txt`
and `target/example.txt` files are present but not with the same last write time,
they are left unchanged and a new file `target/example-2018-05-12-11-11-11.txt`
is created with the contents equal to `source/example.txt`. Notice the dash `-`
before the date.

## Examples

Some example usage is mentioned bellow.

```bash
# show help
dirsync --help

# basic one-way sync with override-with-newer strategy (default)
dirsync ./source ./backup

# one-way sync with verbose output and dry-run,
# logging what extra backup files would be deleted (if not dry-run)
dirsync --verbose --dry-run --delete-extra ./source ./backup

# bidirectional synchronization (both directories updated):
dirsync --bidirectional ./dirA ./dirB

# one-way sync, deleting extra files in target and resolving conflicts by renaming:
dirsync --delete-extra --rename ./source ./destination
```

There are also pre-made examples with directory trees in the project's `examples` directory.
One of them, called `general-usage` example, is demonstrated in the following section.
Make sure you update the `source/conflicts/different.txt` and `target/conflicts/skip-older.txt`
(e.g., with touch command) to be considered newer than their respective counterparts.
The initial file structure:

```
source/
| - .dirsync.json (see bellow)
| - conflicts/
|   | - different.txt (new version)
|   | - skip-older.txt (old version)
| - first/
|   | - recursive.ignored.txt
| - ignored-directory/
|   | - ignored-by-parent.txt
target/
| - conflicts/
|   | - different.txt (old version)
|   | - skip-older.txt (new version)
| - too-large-for-source.txt (more than 50 bytes in size)

source/.dirsync.json = {
  "configVersion": {...},
  "exclusionPatterns": [
    "*.ignored.txt",
    "ignored-directory"
  ],
  "maxFileSize": 50 // bytes
}
```

After syncing with the default options (one-way, override-with-newer conflict resolution)
with `dirsync --verbose source target`, the file tree will look like this:

```
target/
| - conflicts/
|   | - different.txt (new version)  !!! overriden by newer version from the source
|   | - skip-older.txt (new version) !!! not overriden, the source has older version

!!! ignored-directory is excluded, with everything inside it
!!! recursive.ignored.txt is excluded, no "first" directory is created here
```

When synchronized in the opposite way, the `source` does not accept large files,
so `too-large-for-source.txt` is not copied.

## Notes

Symbolic links and filesystem and filename case sensitivity are not fully handled.
