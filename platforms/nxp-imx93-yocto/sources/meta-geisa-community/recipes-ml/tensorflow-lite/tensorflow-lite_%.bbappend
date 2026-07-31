# SPDX-License-Identifier: Apache-2.0

# pip writes this host-specific provenance file during the target package install.
do_install:append() {
    rm -f ${D}/${PYTHON_SITEPACKAGES_DIR}/tflite_runtime-*.dist-info/direct_url.json
}
