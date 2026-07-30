# Release Process

This initial subtree supports development-image work only. Before a public
artifact is released, record a source revision and submodule revisions, run a
normal build, and collect one unambiguous WIC plus its manifests, SPDX archive,
and checksums.

Run `./scripts/validate-release.sh RELEASE_DIRECTORY`. It verifies staged
checksums and performs read-only ext4 checks on the WIC. The release must stop
if e2fsck cannot validate the selected ext4 feature set or reports errors.

Hardware validation must use the exact artifact, preserve its evidence, and
confirm no credentials, accounts, applications, fixtures, or local paths are
embedded. Do not publish an image containing the local Ethos-U/TFLite profile
until its redistribution status is confirmed.
