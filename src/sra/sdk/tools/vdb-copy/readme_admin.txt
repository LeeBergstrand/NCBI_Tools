vdb-copy extended help for admins:

------------------------------------------------------------------------
legacy vs. non-legacy:
vdb-copy examines the source-table to detect if it is a legacy-table.
(a legacy table is a table that has "untyped"-functions in it's schema)

If it is not a legacy-table, no config-entry is necessary to perform
the copy. But if it is a legacy-table, vdb-copy needs to find a config-file
that contains the information about a matching new schema for the copy.
If this information is not found or the schema-file it points to is not
found, vdb-copy cannot perform the copy and fails.

That means: vdb-copy depends on config-file(s) and schema-file(s)
for legacy-tables!

There are 3 steps to find config-file(s):

(1) environment-variables:
The environment-variables  "KLIB_CONFIG", "VDB_CONFIG" and "VDBCONFIG"
are tested in this order. If one is found and it points to config-files
the succeeding ones are not tested. The content of the environment-variable
has to be a path to a directory containing one ore more config-files.
If at least one config-file is found the following steps are not performed.

(2) the standard-location:
The path "/etc/ncbi" is searched for config-files. If at least one config-
file is found, the following step is not performed.

(3) lib-path:
vdb-copy can detect where dynamic libraries it needs are loaded from. If this
directory-path has a sub-directory "ncbi", this directory is scanned for
config-files.

The current directory or the directory where the binary of the tool is found are
not scanned for config-files!

Config-files are files with the extension 'kfg'.

All files with this extension in the directories above (if found) are scanned.

where the tool has found config-files can be checked in this way:
$vdb-copy src dst -+KFG

For vdb-copy to work on legacy files we need:

(1) an entry with the name of the schema that was found in the source-table
(2) a path to a new schema to use for this table (path to text-file)
(3) an existing schema-text-file from (2)

