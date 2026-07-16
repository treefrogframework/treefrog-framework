# MongoDB C Driver SSDLC Compliance Report (@version@)

<!--
This document is intended for distribution with release archives of the MongoDB
C Driver. It wil be copied into the release archive as ssdlc_compliance_report.md
-->

## Release Creator

For information on the release creator, refer to the
[C/C++ Release Info Spreadsheet](https://docs.google.com/spreadsheets/d/1yHfGmDnbA5-Qt8FX4tKWC5xk9AhzYZx1SKF4AD36ecY)
(internal link).

## Process Document

<!-- DRIVERS-2892: replace with link to public-facing document once available. -->
Not yet available.

## Third-Party Dependencies and Vulnerabilities

The tracking of security information for bundled third-party dependencies is
performed using Snyk. Refer to the `etc/third_party_vulnerabilities.md` file
within the repository or the release archive for vulnerability information known
at the time of a release. Full bundled dependency information is available in
the `cyclonedx.sbom.json` file within the release archive.

## Static Analysis Findings

Refer to the
[SSDLC Static Analysis Reports](https://drive.google.com/drive/folders/17bjBnQ3mhEXvs6IK1rrTphJr0CUO2qZh)
folder (internal) for release-specific reports. Available as-needed from the
MongoDB C Driver team.

## Security Testing Report

Refer to the
[Driver Security Testing Summary](https://docs.google.com/document/d/1y2K_RY4GZVXpQvv4JH_35mSzFRTawNJ3mibpvSBU8H0)
document (internal). Available as-needed from the MongoDB C Driver team.

<!--  The below URL is inserted as part of the Earthfile+release-archive target. -->
Refer to the [Evergreen waterfall build](@waterfall_url@) (internal) for test results.

## Release Signature Information

The generated release archive is signed. The detached signature is available
alongside the release archive in the GitHub Release page and can be verified
using the `c-driver` public key available at https://pgp.mongodb.com/.

## Security Assessment Report

Not applicable for the MongoDB C Driver.
