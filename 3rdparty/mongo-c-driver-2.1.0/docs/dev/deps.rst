External Dependencies
#####################

The C driver libraries make use of several external dependencies that are
tracked separately from the library itself. These can be classified as:

.. glossary::

   Bundled Dependency
   Vendored Dependency

      A dependency is "bundled" or "vendored" if its source lives directly
      within the repository that is consuming it. We control the exact version
      of the dependency that is used during the build process, aiding in
      reproducibility and consumability for users.

   System Dependency

      A "system" dependency is any dependency that we rely on being provided by
      the user when they are building or executing the software. We have no
      control over the versions of system dependencies that the user might
      provide to us.

.. _snyk: https://app.snyk.io

The Software Bill of Materials (SBOM)
*************************************

The repository and driver releases contain a machine-readable
:abbr:`SBOM (software bill of materials)` that describes the contents of the
:term:`vendored dependencies <vendored dependency>` used in the distributed
driver libraries.

The SBOM comes in two flavors: The SBOM-\ *lite* and the *augmented* SBOM
(aSBOM). The SBOM-lite is the stored in `etc/cyclonedx.sbom.json` within the
repository, and is mostly generated from `etc/purls.txt`.


.. _sbom-lite:

The SBOM-Lite
=============

The SBOM-lite is "lite" in that it contains only the minimum information
required to later build the `augmented SBOM`_. It contains the name, version,
copyright statements, URLs, and license identifiers of the dependencies.

.. file:: etc/cyclonedx.sbom.json

   The `SBOM-lite`_ for the C driver project. This file is generated
   semi-automatically from `etc/purls.txt` and the `+sbom-generate` Earthly
   target. This file is used by SilkBomb to produce an `augmented SBOM`_.

   .. warning:: This file is **partially generated**! Prefer to edit `etc/purls.txt`!
      Refer to: `sbom-lite-updating`

.. file:: etc/purls.txt

   This file contains a set of purls__ (package URLs) that refer to the
   third-party components that are :term:`vendored <vendored dependency>` into
   the repository. A purl is a URL string that identifies details about software
   packages, including their upstream location and version.

   This file is maintained by hand and should be updated whenever any vendored
   dependencies are updated within the repository. Refer to: `sbom-lite-updating`

   __ https://github.com/package-url/purl-spec


.. _sbom-lite-updating:

Updating the SBOM-Lite
----------------------

Whenever a change is made to the set of vendored dependencies in the repository,
the content of `etc/purls.txt` should be updated and the SBOM-lite
`etc/cyclonedx.sbom.json` file re-generated. The contents of the SBOM lite JSON
*should not* need to be updated manually. Refer to the following process:

1. Add/remove/update the package URLs in `etc/purls.txt` corresponding to the
   vendored dependencies that are being changed.
2. Execute the `+sbom-generate` Earthly target successfully.
3. Stage and commit the changes to *both* `etc/purls.txt` and
   `etc/cyclonedx.sbom.json` simultaneously.

.. _augmented-SBOM:
.. _augmented SBOM:

The Augmented SBOM
==================

At time of writing, the *augmented SBOM* file is not stored within the
repository [#f1]_, but is instead obtained on-the-fly as part of the release
process, as this is primarily a release artifact for end users.

The augmented SBOM contains extra data about the dependencies from the
`SBOM-lite <sbom-lite>`, including vulnerabilities known at the time of the
augmented SBOM's generation. [#asbom-vulns]_

The augmented SBOM is produced by an external process that is not contained
within the repository itself. The augmented SBOM must be downloaded from a
recent execution of the ``sbom`` task in an Evergreen patch or commit build.
This file is included in the release archive for the `+release-archive` target.

.. _snyk scanning:

Snyk Scanning
*************

Snyk_ is a tool that detects dependencies and tracks vulnerabilities in
packages. Snyk is used in a limited fashion to detect vulnerabilities in the
bundled dependencies in the C driver repository.

.. _snyk caveats:

Caveats
=======

At the time of writing (June 20, 2024), Snyk has trouble scanning the C driver
repository for dependencies. If given the raw repository, it will detect the
mongo-c-driver package as the sole "dependency" of itself, and it fails to
detect the other dependencies within the project. The `+snyk-test` Earthly
target is written to avoid this issue and allow Snyk to accurately detect other
dependencies within the project.

For now, vulnerability collection is partially a manual process. This is
especially viable as the native code contains a very small number of
dependencies and it is trivial to validate the output of Snyk by hand.

.. seealso:: The `releasing.snyk` step of the release process


.. _vuln-reporting:

3rd-Party Dependency Vulnerability Reporting
********************************************

Vulnerabilities in :term:`bundled dependencies <bundled dependency>` are tracked
by Snyk, but we maintain a hand-written document that details the
vulnerabilities in current and past dependencies of in-support release versions.

.. file:: etc/third_party_vulnerabilities.md

   The third-party dependency vulnerabily report. This file is stored in the
   repository and updated manually as vulnerabilities are added/removed.

   .. seealso:: At release-time, this file is added to the release archive. See:
      `releasing.vuln-report`


Updating the Vulnerability Report
=================================

When updating `etc/third_party_vulnerabilities.md`, perform the following steps:

1. Open the Snyk_ web UI and sign in via SSO.
2. Open `this Snyk search query`__ (Find the **mongodb/mongo-c-driver** CLI
   target within the **dev-prod** organization. Do not use the *GitHub target*:
   That one is not currently useful to us.)

   __ https://app.snyk.io/org/dev-prod/projects?searchQuery=mongo-c-driver&filters[Integrations]=cli
3. Expand the **mongodb/mongo-c-driver** target, and then expand all **currently
   supported release versions**. (If you are preparing for a new release, that
   version should also be available and used after the `releasing.snyk` process
   has been completed.)
4. Take note of *all unique vulnerabilities amongst all supported versions'*
   that are listed in Snyk. These will be the *relevant* vulnerabilities.
5. For each relevant vulnerability that is not already listed in
   `etc/third_party_vulnerabilities.md`, add a new entry under its corresponding
   package heading that includes the details outlined in the `attribute table`
   below. [#fixit]_

6. For each *already recorded* vulnerability :math:`V` listed in
   `etc/third_party_vulnerabilities.md`:

   1. If :math:`V` is not *relevant* (i.e. it is no longer part of any
      supported release version), delete its entry from
      `etc/third_party_vulnerabilities.md`.
   2. Otherwise, update the entry for of :math:`V` according to the current
      details of the codebase and Snyk report. [#fixit]_

      It is possible that no details need to be modified e.g. if the
      vulnerability is old and already fixed in a past release.

7. Save and commit the changes to `etc/third_party_vulnerabilities.md`.


.. _attribute table:

3rd-Party Dependency Vulnerability Attributes
=============================================

The following attributes of external vulnerabilities must be recorded within
`etc/third_party_vulnerabilities.md`.

.. list-table::

   - - Attribute
     - Explanation
   - - **Date Detected**
     - The ISO 8601 date at which the vulnerability was first detected.
   - - **CVE Number**
     - The CVE record number. Recommended to include a hyperlink to the CVE.

       Example: `CVE-2023-45853 <https://www.cve.org/CVERecord?id=CVE-2023-45853>`_
   - - **Snyk Entry**
     - A link to the Snyk entry in the Snyk Security database.

       Example:
       `SNYK-UNMANAGED-MADLERZLIB-5969359 <https://security.snyk.io/vuln/SNYK-UNMANAGED-MADLERZLIB-5969359>`_.
   - - **Severity**
     - The severity of the vulnerability according to Snyk (Critical/High/Medium/Low)
   - - **Description**
     - Paste the description field from Snyk.
   - - **Upstream Fix Status**
     - One of "false positive", "won't fix", "fix pending", or "fix available".
       If a fix is avilable, this entry should include the version number and
       date at which the upstream project released a fix.
   - - **mongo-c-driver Fix Status**
     - One of "false positive", "won't fix", "fix pending", or "fix available".
       If a fix is avilable, this entry should include the version number and
       release date of the C driver that includes the fixed version. Use "fix
       pending" if the bundled dependency has been upgraded but there has not
       been a release that includes this upgrade.
   - - **Notes**
     - If a fix is available from the upstream package but has been purposefully
       omitted from a C driver release, this field should explain the reasoning
       for that omission.

       Other notes about the vulnerability that may be useful to users and
       future developers can also be included here.


.. rubric:: Example

The following is an example for a vulnerability listing in
`etc/third_party_vulnerabilities.md`

.. code-block:: markdown

   # Zlib

   ## CVE-2023-45853 - Integer Overflow or Wraparound

   - **Date Detected**: 2024-06-24
   - **CVE Number**: [CVE-2023-45853](https://www.cve.org/CVERecord?id=CVE-2023-45853)
   - **Snyk Entry**: [SNYK-UNMANAGED-MADLERZLIB-5969359](https://security.snyk.io/vuln/SNYK-UNMANAGED-MADLERZLIB-5969359)
   - **Severity**: High
   - **Description**: Affected versions of this package are vulnerable to
     Integer Overflow or Wraparound via the `MiniZip` function in `zlib`, by
     providing a long filename, comment, or extra field.
   - **Upstream Fix Status**: Fix available (1.3.1, 2024-01-22)
   - **mongo-c-driver Fix Status**: Fix available (1.27.3, 2024-06-26)
   - **Notes**: This issue was related to Zip file handling, which was not used
     by mongo-c-driver. This errant code was never reachable via the C driver
     APIs.


.. rubric:: Footnotes

.. [#f1]

   This may change in the future depending on how the process may evolve.

.. [#asbom-vulns]

   At time of writing, the vulnerabilities listing in the augmented SBOM is
   incomplete and vulnerability collection is partially manual. See:
   `snyk caveats` and `releasing.vuln-report`.

.. [#fixit]

   If a fix is available and is reasonably easy to introduce, consider upgrading
   the associated dependency to include a fix before the next release is
   finalized.

   If a fix is available but *not* applied, then the rationale for such a
   decision will need to be included in the vulnerability listing (See the
   **Notes** section in the `attribute table`).
