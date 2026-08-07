# SPDX-License-Identifier: Apache-2.0

# protobuf.cmake derives the native-tools module path from this TensorFlow
# Lite source root when the native_tools subdirectory is configured directly.
DEPENDS:append:class-target = " flatbuffers-native"
EXTRA_OECMAKE:append:class-target = " \
    -DFLATC_INSTALL_PREFIX=${B}/flatbuffers-flatc-install \
    -DTFLITE_SOURCE_DIR=${S}/tensorflow/lite \
    -DTFLITE_HOST_TOOLS_DIR=${STAGING_BINDIR_NATIVE} \
    -DTFLITE_NATIVE_TOOLS_BUILD_PROTOC=OFF \
"
EXTRA_OECMAKE:append:class-native = " \
    -DTFLITE_NATIVE_TOOLS_BUILD_PROTOC=ON \
"
