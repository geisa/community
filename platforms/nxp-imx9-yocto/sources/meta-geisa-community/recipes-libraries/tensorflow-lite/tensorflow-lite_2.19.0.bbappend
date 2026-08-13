# SPDX-License-Identifier: Apache-2.0

# native_utils.cmake appends /bin to this value when locating protoc. The
# NXP recipe passes STAGING_BINDIR_NATIVE, which produces an invalid /bin/bin
# path for the 2.19 cross-compile.
EXTRA_OECMAKE:append = " -DTFLITE_HOST_TOOLS_DIR=${STAGING_DIR_NATIVE}/usr"
