# The Scarthgap baseline pseudo revision relies on host-facing patches that are
# incompatible with the current build host. Use the validated pseudo revision,
# provide its matching older-glibc context, and make the native sqlite3
# dependency explicit for fakeroot setscene tasks.
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI:remove = " \
    file://0001-configure-Prune-PIE-flags.patch \
    file://glibc238.patch \
"
SRCREV = "9ab513512d8b5180a430ae4fa738cb531154cdef"
PV = "1.9.3+git"

PSEUDO_SETSCENE_DEPS = ""
PSEUDO_SETSCENE_DEPS:class-native = "sqlite3-native:do_populate_sysroot"
do_populate_sysroot_setscene[depends] += "${PSEUDO_SETSCENE_DEPS}"
