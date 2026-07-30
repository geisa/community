# Commissioning

The development image intentionally contains no operator account, SSH key,
site network identity, application, or private repository credential. Provision
those items after first boot through the local deployment process.

The image includes sudo policy for a configured operator account, but account
creation and user-specific sudo membership are mutable commissioning state.
Keep private credentials outside the image and outside this repository.

Verify the host with `geisa-system-state --once`, inspect the network and time
configuration, and record the exact image manifest before adding applications.
