# Example 2: renaming conflict strategy

Goal: two-way synchronization with renaming conflicts.

Make sure you update the `b/common.txt` (e.g., with touch command) to be considered
newer than their respective counterparts. 

The initial file structure:

```
a/
| - common.txt (older version)
| - only-in-a.txt
b/
| - common.txt (newer version)
| - only-in-b.txt

```

After syncing with the two-way synchronization with conflict-renaming strategy:

```
$ touch b/common.txt # to ensure newer timestamp
$ dirsync --verbose --bidirectional --rename b a
Copying "b/common.txt"
Copying "a/only-in-a.txt"
Copying "b/only-in-b.txt"
```

the file tree will look like this:

```
a/
| - common-2025-04-30-10-04-10.txt (contains "b/common.txt" contents)
| - common.txt (original in "a", unmodified)
| - only-in-a.txt
| - only-in-b.txt (copied here)
b/
| - common.txt (original in "b", unmodified)
| - only-in-a.txt (copied here)
| - only-in-b.txt
```
