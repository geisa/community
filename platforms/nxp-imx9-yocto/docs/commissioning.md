# Commissioning

The development image intentionally contains no operator accounts, SSH keys,
site network configuration, applications, or private credentials. If intended
for normal usage, provision those items after first boot through your preferred
local deployment and configuration process, but ensure these do not leak into
any repo updates if making contributions.  Keep private credentials outside of
the image and outside this repository.

This image includes sudo policy for a configured operator account, but account
creation and user-specific sudo membership are part of the post-boot
commissioning.

Verify the host with `geisa-system-state --once`, inspect the network and time
configuration, and record the exact image manifest before adding applications.
