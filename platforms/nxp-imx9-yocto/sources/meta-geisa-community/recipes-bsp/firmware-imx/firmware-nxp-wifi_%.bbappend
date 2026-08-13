# The pinned NXP FRDM append adds this package even though the base recipe
# already declares it. Normalize the final package list so package QA remains
# enabled while preserving the base recipe's package split.
python __anonymous() {
    packages = (d.getVar("PACKAGES") or "").split()
    unique_packages = list(dict.fromkeys(packages))
    d.setVar("PACKAGES", " ".join(unique_packages))
}
