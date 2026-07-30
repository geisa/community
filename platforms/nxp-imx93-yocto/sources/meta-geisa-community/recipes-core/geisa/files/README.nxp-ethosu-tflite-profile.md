# Local NXP Ethos-U/TFLite Profile Input

`nxp-ethosu-tflite.tar.gz` is deliberately not part of this repository.

The previously staged archive had SHA-256
`b54e65b270c8937c9433d34f5b460df7232b842a049d6e0bbb76871614e3526d`, 481
entries, and a single `nxp-ethosu-tflite/` top-level directory. Its visible
content included `profile.json`, Python runtime modules, native extensions,
and a small number of embedded third-party license files. Its original source,
complete bill of materials, and redistribution authorization were not recorded
with the archive. The `CLOSED` recipe alone is not evidence that it may be
redistributed.

Do not add the archive to Git. A maintainer who has separately obtained the
required runtime artifacts under the applicable terms must stage an archive
locally before building the development image:

```sh
./scripts/stage-nxp-ethosu-profile.sh /path/to/nxp-ethosu-tflite.tar.gz \
  --origin 'source URL or internal acquisition record'
```

The helper accepts either a root-level profile tree or a tree under
`nxp-ethosu-tflite/`, validates every listed member before copying, and records
the selected layout, supplied origin, and SHA-256 in an ignored local file. BitBake
includes the parsed archive SHA-256 and input state in the profile task
signature, rechecks the archive before installation, and records only the SHA-
256 in the installed profile. A future public distribution requires an explicit
provenance record, complete license review, and confirmed redistribution
permission for every included component.
