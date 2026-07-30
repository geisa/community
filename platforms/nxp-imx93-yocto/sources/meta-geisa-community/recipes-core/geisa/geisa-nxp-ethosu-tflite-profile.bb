# SPDX-License-Identifier: Apache-2.0
#
SUMMARY = "GEISA NXP Ethos-U TensorFlow Lite profile root"
DESCRIPTION = "Stages a locally acquired NXP Ethos-U/TFLite profile under /platform."
LICENSE = "CLOSED"

# The archive and its origin record are intentionally ignored. Parsing stays
# usable without private inputs; do_install fails until their SHA-256 matches.
# See files/README.nxp-ethosu-tflite-profile.md.
GEISA_NXP_ETHOSU_TFLITE_PROFILE_ARCHIVE = "${THISDIR}/files/nxp-ethosu-tflite.tar.gz"
GEISA_NXP_ETHOSU_TFLITE_PROFILE_ORIGIN = "${THISDIR}/files/nxp-ethosu-tflite.local-origin.txt"

python __anonymous() {
    import hashlib
    import os

    archive = d.expand(d.getVar("GEISA_NXP_ETHOSU_TFLITE_PROFILE_ARCHIVE"))
    origin = d.expand(d.getVar("GEISA_NXP_ETHOSU_TFLITE_PROFILE_ORIGIN"))
    state = "missing archive and checksum/origin metadata"
    actual = ""
    expected = ""

    if os.path.isfile(archive):
        digest = hashlib.sha256()
        with open(archive, "rb") as source:
            for chunk in iter(lambda: source.read(1024 * 1024), b""):
                digest.update(chunk)
        actual = digest.hexdigest()
        state = "missing checksum/origin metadata"

    if os.path.isfile(origin):
        with open(origin, encoding="utf-8") as record:
            for line in record:
                if line.startswith("sha256: "):
                    expected = line.split(":", 1)[1].strip()
                    break
        state = "missing archive"

    if actual and expected:
        state = "matched" if actual == expected else "checksum mismatch"

    d.setVar("GEISA_NXP_ETHOSU_TFLITE_PROFILE_ARCHIVE_SHA256", actual)
    d.setVar("GEISA_NXP_ETHOSU_TFLITE_PROFILE_EXPECTED_SHA256", expected)
    d.setVar("GEISA_NXP_ETHOSU_TFLITE_PROFILE_INPUT_STATE", state)
}

S = "${WORKDIR}"

RDEPENDS:${PN} += "geisa-runtime-groups"

do_install() {
    archive="${GEISA_NXP_ETHOSU_TFLITE_PROFILE_ARCHIVE}"
    if [ "${GEISA_NXP_ETHOSU_TFLITE_PROFILE_INPUT_STATE}" != "matched" ]; then
        bbfatal "NXP Ethos-U/TFLite profile input is ${GEISA_NXP_ETHOSU_TFLITE_PROFILE_INPUT_STATE}; run scripts/stage-nxp-ethosu-profile.sh with an authorized local archive"
    fi
    actual="$(sha256sum "${archive}" | awk '{print $1}')"
    if [ "${actual}" != "${GEISA_NXP_ETHOSU_TFLITE_PROFILE_ARCHIVE_SHA256}" ]; then
        bbfatal "NXP Ethos-U/TFLite profile archive changed after parsing; rerun BitBake"
    fi
    tar -xzf "${archive}" -C ${WORKDIR}
    install -d -m 0755 ${D}/platform/profiles/nxp-ethosu-tflite
    cp -a ${WORKDIR}/nxp-ethosu-tflite/. ${D}/platform/profiles/nxp-ethosu-tflite/
    printf '%s\n' "${GEISA_NXP_ETHOSU_TFLITE_PROFILE_ARCHIVE_SHA256}" \
        > ${D}/platform/profiles/nxp-ethosu-tflite/.geisa-source-archive.sha256
    chown -R 0:0 ${D}/platform/profiles/nxp-ethosu-tflite
    chmod -R go-w ${D}/platform/profiles/nxp-ethosu-tflite
}

FILES:${PN} = "/platform/profiles/nxp-ethosu-tflite"
INSANE_SKIP:${PN} += "already-stripped dev-so file-rdeps ldflags"
SKIP_FILEDEPS:${PN} = "1"
do_install[vardeps] += " \
    GEISA_NXP_ETHOSU_TFLITE_PROFILE_ARCHIVE_SHA256 \
    GEISA_NXP_ETHOSU_TFLITE_PROFILE_EXPECTED_SHA256 \
    GEISA_NXP_ETHOSU_TFLITE_PROFILE_INPUT_STATE \
"
