do_configure:append () {
    echo '/dev/mmcblk0p3  /data  ext4  defaults  0  2' >> ${WORKDIR}/fstab
}
