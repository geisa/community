# SPDX-License-Identifier: Apache-2.0
#
# The upstream TensorFlow Lite host-tools CMake builds flatc through an
# ExternalProject and otherwise tries to install that nested project into /usr
# during do_compile. Keep the nested install staged inside WORKDIR so the
# target package remains buildable without host filesystem writes.

EXTRA_OECMAKE += "-DFLATC_INSTALL_PREFIX=${B}/flatbuffers-flatc-install"
