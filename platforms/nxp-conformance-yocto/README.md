# NXP GEISA Conformance Yocto Platform

## Relationship to GEISA

This project builds the NXP-based Yocto platform environment developed for
GEISA conformance and mock implementation testing.

It includes GEISA mock services and supporting platform configuration,
including:

- the GEISA API mockup;
- the GEISA ADM client;
- Wakaama integration;
- Mosquitto policy for GEISA API topics;
- LXC and application-base image support;
- NXP i.MX93, i.MX93 EVK, and i.MX95 EVK targets.

This is a Community platform project. It is not the GEISA specification, an
official conformance result, or a production-ready platform.

For general GEISA development on the FRDM-i.MX93, use the cleaner development
platform at:

`platforms/nxp-imx93-yocto/`

That project does not preload this conformance and mock stack.

## Project Information

- **Status:** Maintenance Limited
- **Maintainers:** Kévin L'hôpital (`@kevlhop`), Nghia Dam (`@ndam-sfl`)
- **License:** Apache-2.0, with component-specific licenses where identified
- **Original repository:** `geisa/lee-mockup`
- **Imported source revision:** `c343a196874f506b184f779d43848c9c99aab65e`
- **GEISA versions tested or supported:** Historical GEISA development and
  conformance environment
- **Support:** Best-effort

The original Git history is preserved.

## Choosing an NXP Platform

Use this project when the goal is to reproduce, inspect, or extend the
NXP-based GEISA conformance and mock environment.

Use `platforms/nxp-imx93-yocto/` when the goal is to:

- develop GEISA applications;
- develop or test a platform implementation;
- start from a cleaner FRDM-i.MX93 image;
- avoid preinstalled API and ADM mock implementations;
- avoid inheriting conformance-specific Mosquitto policy and test behavior.

The two projects are intentionally independent. Their Yocto layers, source
trees, recipes, build outputs, and dependency setup must not be mixed.

## Building or Using the Project

This project keeps its external Yocto dependencies self-contained. They are
not registered in the Community repository's root `.gitmodules`.

From the Community repository root:

    cd platforms/nxp-conformance-yocto
    ./scripts/setup-sources.sh

The setup script reads this project's local `.gitmodules` and checks out the
exact dependency revisions recorded by the imported project.

To inspect the dependency list without cloning:

    ./scripts/setup-sources.sh --dry-run

The original CQFD and manual build instructions are retained in
[DEVELOPER_GUIDE.adoc](DEVELOPER_GUIDE.adoc).

Typical CQFD builds include:

    cqfd -b imx93-dev
    cqfd -b imx93-evk-dev
    cqfd -b imx95-evk-dev
    cqfd -b imx93-prod
    cqfd -b imx93-evk-prod
    cqfd -b imx95-evk-prod

The initial Yocto build requires substantial time, storage, network bandwidth,
and host resources.

## Project Contents

The project-owned layers include:

- `sources/meta-geisa-bsp`
- `sources/meta-geisa-distro`
- `sources/meta-geisa-app`

The image currently includes or builds against:

- `adm-client`, sourced from the historical GEISA ADM mockup;
- `gapi`, sourced from the historical GEISA API mockup;
- Wakaama and TinyDTLS;
- Mosquitto with conformance-oriented dynamic-security configuration;
- LXC and application-base image support.

These components are part of this project's conformance and mock environment.
They are not part of the clean `nxp-imx93-yocto` platform baseline.

## Known Limitations

- This project combines board support, image construction, conformance support,
  and mock implementations in one Yocto environment.
- It should not be used as the default starting point for general GEISA
  application or platform development.
- The Wakaama recipe uses `AUTOREV`, so a new build may not resolve to the same
  Wakaama source revision unless that behavior is changed separately.
- The API mockup recipe also uses `AUTOREV`.
- The ADM client is pinned to a historical standalone-repository revision.
- The image includes conformance-oriented Mosquitto users, permissions, and
  topic policy.
- The image named `geisa-prod-image` contains a root password documented as
  being for test purposes and must not be treated as production hardened.
- The project predates the cleaner Community NXP development platform and does
  not share its release, validation, packaging, or security posture.
- A full build has not been repeated as part of the Community migration.
- Modernizing the mock stack, dependency pins, schemas, recipes, or image
  architecture should be handled as separate functional work.
