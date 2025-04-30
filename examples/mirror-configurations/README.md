# Example 3: mirror with local configuration

Goal: one-way synchronization with copying local configurations,
skipping existing files.

The initial file structure:

```
source/
| - common.txt (with content A)
| - new-subdirectory/
|   | - .dirsync.json (see bellow)
|   | - ignored.log (excluded)
|   | - text.txt
mirror/
| - common.txt (with content B)

source/new-subdirectory/.dirsync.json = {
  "configVersion": {...},
  "exclusionPatterns": ["*.log"]
}
```

We synchronize with `--copy-configurations` flag, allowing the program
to copy `.dirsync.*` files themselves, along with normal synchronization.
We also use conflict-skipping strategy with `-s|--skip-existing|--safe` flag,
meaning all common files will be left unchanged and desynchronized.

```
$ dirsync --verbose --copy-configurations --safe source mirror
Copying "source/new-subdirectory/.dirsync.json"
Copying "source/new-subdirectory/text.txt"
```

After the sync, the file tree will look like this:

```
source/
| - common.txt (with content A, unchanged)
| - new-subdirectory/
|   | - .dirsync.json (see bellow)
|   | - ignored.log (excluded)
|   | - text.txt
mirror/
| - common.txt (with content B, unchanged)
| - new-subdirectory/
|   | - .dirsync.json (copied here)
|   | - text.txt

!!! source/new-subdirectory/ignored.log is excluded from sync by the local config
```

The contents of `common.txt` file is unchanged, regardless which version is newer.
