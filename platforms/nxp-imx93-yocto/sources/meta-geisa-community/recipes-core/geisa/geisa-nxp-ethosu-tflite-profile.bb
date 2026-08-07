# SPDX-License-Identifier: Apache-2.0
#
SUMMARY = "Reproducible GEISA NXP Ethos-U TensorFlow Lite profile root"
DESCRIPTION = "Constructs the managed Ethos-U/TFLite profile from declared Yocto dependencies and a pinned PyAV wheel."

# The generated profile combines NXP BSP runtime outputs with a third-party
# binary wheel. Its component licenses are recorded by the same-build SPDX and
# package manifests; binary redistribution requires their separate review.
LICENSE = "CLOSED"

PYAV_WHEEL = "av-18.0.0-cp311-abi3-manylinux_2_28_aarch64.whl"
PYAV_URL = "https://files.pythonhosted.org/packages/84/74/6732f17b96dc23fd23b876b2805435855abdc8a3b397142be4e581165de8/${PYAV_WHEEL}"
PYAV_SHA256 = "4d683b7747a0ba9222b8a5f81e41db5f796e7f64473454ec4fe2548e083c2fa0"

SRC_URI = " \
    file://assemble-nxp-ethosu-tflite-profile.py \
    ${PYAV_URL};name=pyav;downloadfilename=${PYAV_WHEEL};unpack=0 \
"
SRC_URI[pyav.sha256sum] = "${PYAV_SHA256}"

S = "${UNPACKDIR}"
include ${TOPDIR}/conf/geisa-source-state.conf
PACKAGE_ARCH = "${MACHINE_ARCH}"

DEPENDS = " \
    ethos-u-driver-stack \
    python3-numpy \
    tensorflow-lite \
    tensorflow-lite-ethosu-delegate \
"
RDEPENDS:${PN} += "geisa-runtime-groups"

do_install() {
    output="${D}/platform/profiles/nxp-ethosu-tflite"
    rm -rf "$output"
    python3 "${UNPACKDIR}/assemble-nxp-ethosu-tflite-profile.py" assemble \
        --sysroot "${RECIPE_SYSROOT}" \
        --wheel "${DL_DIR}/${PYAV_WHEEL}" \
        --root "$output" \
        --source-revision "${GEISA_SOURCE_REVISION}"
    python3 "${UNPACKDIR}/assemble-nxp-ethosu-tflite-profile.py" verify --root "$output"
    chown -R 0:0 "$output"
    chmod -R go-w "$output"
}

FILES:${PN} = "/platform/profiles/nxp-ethosu-tflite"
# These libraries are deliberately private to the read-only profile; managed
# applications load them through the profile's explicit library path.
PRIVATE_LIBS:${PN} = "libethosu.so.1.0.0 libethosu_delegate.so libtensorflow-lite.so.${GEISA_TFLITE_RUNTIME_VERSION}"
INSANE_SKIP:${PN} += "already-stripped dev-so file-rdeps ldflags"
SKIP_FILEDEPS:${PN} = "1"
do_install[vardeps] += "PYAV_SHA256 PYAV_URL GEISA_SOURCE_REVISION"
