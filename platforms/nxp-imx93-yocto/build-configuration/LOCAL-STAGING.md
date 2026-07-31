# Local Build Values

This repository intentionally does not prescribe a build-host download or
sstate-cache location. Set `DL_DIR` and `SSTATE_DIR` in the environment when a
shared cache is useful; they are local values and must not be committed.

The source directories under `sources/` are pinned Git submodules. A release
build must use those checked-out revisions rather than a sibling development
tree.
