# Example 1: general usage

Goal: one-way synchronization with local configuration.

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
with

```
$ dirsync --verbose source target
Skipped copying older version of "source/conflicts/skip-older.txt"
Copying "source/conflicts/different.txt"
Copying "source/root.txt"
```

the file tree will look like this:

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
