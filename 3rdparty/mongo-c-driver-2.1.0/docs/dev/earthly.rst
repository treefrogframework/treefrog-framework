Earthly
#######

Earthly_ is a CI and development tool that containerizes aspects of the CI
pipeline so that they run consistently across hosts and across time.

.. highlight:: console

.. _earthly: https://earthly.dev
.. _earthly secrets: https://docs.earthly.dev/docs/guides/secrets
.. _docker: https://www.docker.com/
.. _podman: https://podman.io/

Running Earthly
***************

.. note::

   Before you can run Earthly, you will need either Podman_ or Docker_ installed
   on your system. If you have trouble getting Earthly to work with Podman,
   refer to `the Earthly Podman Guide`__.

   __ https://docs.earthly.dev/docs/guides/podman

While it is possible to download and install Earthly_ on your system, this task
itself is automated by scripting within the ``mongo-c-driver`` repository. To
run Earthly from the ``mongo-c-driver`` repository, use `tools/earthly.sh`.

.. script:: tools/earthly.sh

   This script will download and cache an Earthly executable on-the-fly and execute
   it with the same command-line arguments that were passed to the script. For any
   ``earthly`` command, you may run `tools/earthly.sh` in its place.

   .. code-block:: console
      :caption: Example Earthly output

      $ ./tools/earthly.sh --version
      earthly-linux-amd64 version v0.8.3 70916968c9b1cbc764c4a4d4d137eb9921e97a1f linux/amd64; EndeavourOS

   Running Earthly via this script ensures that the same Earthly version is used
   across all developer and CI systems.

   .. envvar:: EARTHLY_VERSION

      The `tools/earthly.sh` script inspects the `EARTHLY_VERSION` environment
      variable and downloads+executes that version of Earthly. This allows one
      to test new Earthly versions without modifying the `tools/earthly.sh`
      script.

      This environment variable has a default value, so specifying it is not
      required. Updating the default value in `tools/earthly.sh` will change the
      default version of Earthly that is used by the script.


Testing Earthly
===============

To verify that Earthly is running, execute the ``+env.u20`` Earthly
target. This will exercise most Earthly functionality without requiring any
special parameters or modifying the working directory::

   $ ./tools/earthly.sh +env.u20
   Init 🚀
   ————————————————————————————————————————————————————————————————————————————————

   [... snip ...]

   ========================== 🌍 Earthly Build  ✅ SUCCESS ==========================


Earthly Targets
***************

This section documents some of the available Earthly targets contained in the
top-level ``Earthfile`` in the repository. More are defined, and can be
enumerated using ``earthly ls`` or ``earthly doc`` in the root of the repository.

.. program:: +signed-release
.. earthly-target:: +signed-release

   Creates signed release artifacts using `+release-archive` and `+sign-file`.

   .. seealso:: `releasing.gen-archive`, which uses this target.

   .. earthly-artifact:: +signed-release/dist/

      A directory artifact that contains the `+release-archive/release.tar.gz`,
      `+release-archive/ssdlc_compliance_report.md`, and
      `+sign-file/signature.asc` for the release. The exported filenames are
      based on the `--version` argument.

   .. option:: --version <version>

      Affects the output filename and archive prefix paths in
      `+signed-release/dist/` and sets the default value for `--ref`.

   .. option:: --ref <git-ref>

      Specify the git revision to be archived. Forwarded to
      `+release-archive --ref`. If unspecified, archives the Git tag
      corresponding to `--version`.

   .. rubric:: Secrets

   Secrets for the `+snyk-test` and `+sign-file` targets are required for this
   target.


.. program:: +release-archive
.. earthly-target:: +release-archive

   Generate a source release archive of the repository for the specified branch.
   Requires the secrets for `+snyk-test`.
   Requires ``etc/augmented-sbom.json`` is present (obtained from Evergreen).

   .. earthly-artifact:: +release-archive/release.tar.gz

      The resulting source distribution archive for the specified branch. The
      generated archive includes the source tree, but also includes other
      release artifacts that are generated on-the-fly when invoked.

   .. earthly-artifact:: +release-archive/ssdlc_compliance_report.md

      The SSDLC compliance report for the release. This file is based on the
      content of ``etc/ssdlc.md``, which has certain substrings replaced based
      on attributes of the release.

   .. option:: --ref <git-ref>

      Specifies the Git revision that is used when we use ``git archive`` to
      generate the repository archive snapshot. Use of ``git archive`` ensures
      that the correct contents are included in the archive (i.e. it won't
      include local changes and ignored files). This also allows a release
      snapshot to be taken for a non-active branch.

   .. option:: --prefix <path>

      Specify a filepath prefix to appear in the generated filepaths. This has
      no effect on the files archived, which is selected by
      `+release-archive --ref`.

.. program:: +sbom-validate
.. earthly-target:: +sbom-validate

   Validate the `etc/cyclonedx.sbom.json`.

.. program:: +sign-file
.. earthly-target:: +sign-file

   Signs a file using Garasign. Use of this target requires authenticating
   against the DevProd-provided Amazon ECR instance! (Refer to:
   `earthly.amazon-ecr`)

   .. earthly-artifact:: +sign-file/signature.asc

      The detached PGP signature for the input file.

   .. rubric:: Parameters
   .. option:: --file <filepath>

      **Required**. Specify a path to a file (on the host) to be signed. This
      file must be a descendant of the directory that contains the ``Earthfile``
      and must not be excluded by an ``.earthlyignore`` file (it is copied
      into the container using the COPY__ command.)

      __ https://docs.earthly.dev/docs/earthfile#copy

   .. rubric:: Secrets
   .. envvar::
      GRS_CONFIG_USER1_PASSWORD
      GRS_CONFIG_USER1_USERNAME

      **Required**. [#creds]_

      .. seealso:: `earthly.secrets`

   .. _earthly.amazon-ecr:

   Authenticating with Amazon ECR
   ==============================

   In order to run `+sign-file` or any target that depends upon it, the
   container engine client\ [#oci]_ will need to be authenticated with the
   DevProd-provided Amazon ECR instance using AWS CLI v2::

      # Forward the short-term AWS credentials to the container engine client.
      $ aws ecr get-login-password --profile <profile> | podman login --username AWS --password-stdin 901841024863.dkr.ecr.us-east-1.amazonaws.com

   Configure the AWS profile using ``aws configure sso`` or modifying the
   ``$HOME/.aws/config`` file such that:

   - The SSO start URL is ``https://d-9067613a84.awsapps.com/start#/``.
   - The SSO and client region are ``us-east-1``.
   - The SSO registration scope is ``sso:account:access`` (default).
   - The SSO account ID is ``901841024863`` (aka ``devprod-platforms-ecr``).
   - The SSO role name is ``ECRScopedAccess`` (default).

   To refresh short-term credentials when they have expired, run
   ``aws sso login --profile <profile>`` followed by the same
   ``aws ecr get-login-password ... | podman login ...`` command described
   above.

   .. seealso:: `"DevProd Platforms Container Registry"
      <https://docs.devprod.prod.corp.mongodb.com/devprod-platforms-ecr>`_ and
      `"Configuring IAM Identity Center authentication with the AWS CLI"
      <https://docs.aws.amazon.com/cli/latest/userguide/cli-configure-sso.html>`_.

.. earthly-target:: +sbom-generate

   Updates the `etc/cyclonedx.sbom.json` file **in-place** based on the contents
   of `etc/purls.txt` and the existing `etc/cyclonedx.sbom.json`.

   After running this target, the contents of the `etc/cyclonedx.sbom.json` file
   may change.

   .. seealso:: `sbom-lite` and `sbom-lite-updating`

.. earthly-target:: +sbom-generate-new-serial-number

   Equivalent to `+sbom-generate` but uses the ``--generate-new-serial-number``
   flag to generate a new unique serial number and reset the SBOM version to 1.

   After running this target, the contents of the `etc/cyclonedx.sbom.json` file
   may change.

   .. seealso:: `sbom-lite` and `sbom-lite-updating`

.. program:: +snyk-monitor-snapshot
.. earthly-target:: +snyk-monitor-snapshot

   Executes `snyk monitor`__ on a crafted snapshot of the remote repository.
   This target specifically avoids an issue outlined in `snyk scanning` (See
   "Caveats"). Clones the repository at the given `--branch` for the snapshot
   being taken.

   __ https://docs.snyk.io/snyk-cli/commands/monitor

   .. seealso:: Release process step: `releasing.snyk`

   .. rubric:: Parameters
   .. option:: --branch <branch>

      **Required**. The name of the branch or tag to be snapshot.

   .. option:: --name <name>

      **Required**. The name for the monitored snapshot ("target reference") to
      be stored in the Snyk server.

      .. note:: If a target with this name already exists in the Snyk server,
         then executing `+snyk-monitor-snapshot` will replace that target.

   .. option:: --remote <url | "local">

      The repository to be snapshot and posted to Snyk for monitoring. Defaults
      to the upstream repository URL. Use ``"local"`` to snapshot the repository
      in the working directory (not recommended except for testing).

   .. rubric:: Secrets
   .. envvar:: SNYK_ORGANIZATION

      The API ID of the Snyk_ organization that owns the Snyk target. For the C
      driver, this secret must be set to the value for the organization ID of
      the MongoDB **dev-prod** Snyk organization.

      **Do not** use the organization ID of **mongodb-default**.

      The `SNYK_ORGANIZATION` may be obtained from the `Snyk organization page
      <https://app.snyk.io/org/dev-prod/manage/settings>`_.

      .. _snyk: https://app.snyk.io

   .. envvar:: SNYK_TOKEN

      Set this to the value of an API token for accessing Snyk in the given
      `SNYK_ORGANIZATION`.

      The `SNYK_TOKEN` may be obtained from the `Snyk account page <https://app.snyk.io/account>`_.

.. program:: +snyk-test
.. earthly-target:: +snyk-test

   Execute `snyk test`__ on the local copy. This target specifically avoids an
   issue outlined in `Snyk Scanning > Caveats <snyk caveats>`.

   __ https://docs.snyk.io/snyk-cli/commands/test

   .. earthly-artifact:: +snyk-test/snyk.json

      The Snyk JSON data result of the scan.

   .. rubric:: Secrets
   .. envvar:: SNYK_TOKEN
      :noindex:

      See: `SNYK_TOKEN`


.. program:: +verify-headers
.. earthly-target:: +verify-headers

   This runs `CMake's header verification`__ on the library sources, to ensure
   that the public API headers can be ``#include``\ 'd directly in a C++
   compiler.

   __ https://cmake.org/cmake/help/latest/prop_tgt/VERIFY_INTERFACE_HEADER_SETS.html

   This target does not produce any output artifacts. This only checks that our
   public API headers are valid. This checks against a variety of environments
   to test that we are including the necessary standard library headers in our
   public API headers.


.. _earthly.secrets:

Setting Earthly Secrets
***********************

Some of the above targets require defining `earthly secrets`_\
[#creds]_.

To pass secrets to Earthly, it is easiest to use a ``.secret`` file in the root
of the repository. Earthly will implicitly read this file for secrets required
during execution. Your ``.secret`` file will look something like this:

.. code-block:: ini
   :caption: Example ``.secret`` file content

   GRS_CONFIG_USER1_USERNAME=john.doe
   GRS_CONFIG_USER1_PASSWORD=hunter2

.. warning::

   Earthly supports passing secrets on the command line, **but this is not
   recommended** as the secrets will then be stored in shell history.

   Shell history can be supressed by prefixing a command with an extra space,
   but this is more cumbersome than using environment variables or a ``.secret``
   file.

.. seealso:: `The Earthly documentation on passing secrets <earthly secrets_>`_

.. [#oci]

   You container engine client will probably be Docker or Podman. Wherever the
   :bash:`podman` command is used, :bash:`docker` should also work equivalently.


.. [#creds]

   Credentials are expected to be available in `AWS Secrets Manager
   <https://wiki.corp.mongodb.com/display/DRIVERS/Using+AWS+Secrets+Manager+to+Store+Testing+Secrets>`_ under
   ``drivers/c-driver``.
