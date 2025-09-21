.. title:: Releasing the MongoDB C Driver
.. rubric:: Releasing the MongoDB C Driver
.. The use of "rubric" here is to give the page a title header that does
   not effect the section numbering, which we use to enumerate the steps of the
   process. This page is not included directly in a visible toctree, and is instead
   linked manually with a :doc: role. If this page is included in a visible toctree, then
   the top-level sections would be inlined into the toctree in an unintuitive manner.

This page documents the process required for releasing a new version of the
MongoDB C driver library. The release includes the following steps:

.. sectnum::
   :prefix: Step #
.. contents:: Release Process

.. _latest-build: https://spruce.mongodb.com/commits/mongo-c-driver
.. _evg-release: https://spruce.mongodb.com/commits/mongo-c-driver-latest-release
.. _evg-release-settings: https://spruce.mongodb.com/project/mongo-c-driver-latest-release/settings/general
.. _snyk: https://app.snyk.io
.. _dbx-c-cxx-releases-github: https://github.com/orgs/mongodb/teams/dbx-c-cxx-releases/
.. _dbx-c-cxx-releases-mana: https://mana.corp.mongodb.com/resources/68029673d39aa9f7de6399f9

.. rubric:: Checklist Form

The following Markdown text can be copied as a step-by-step checklist for this
process.

.. code-block:: markdown

   - [ ] Check Static Analysis
   - [ ] Check that Tests Are Passing
   - [ ] Create a new **mongo-c-driver** clone
   - [ ] Check and Update the SBOM Lite
   - [ ] Start Snyk Monitoring
   - [ ] Address and Report Vulnerabilities
   - [ ] Validate API Documentation
   - [ ] Get a GitHub API Token
   - [ ] Do the Release:
       - [ ] Start a Release Stopwatch (start time: HH:MM)
       - [ ] Clone the Driver Tools
       - [ ] If patch release: Check consistency with [the Jira release](https://jira.mongodb.org/projects/CDRIVER/versions/XXXXXX)
       - [ ] Run the Release Script
       - [ ] Fixup the `NEWS` Pages
       - [ ] Signed & Upload the Release
       - [ ] Publish Release Artifacts
       - [ ] Publish Documentation
       - [ ] Announce the Release on the Community Forums
       - [ ] Copy the Release Updates to the ``master`` Branch
       - [ ] Close the Jira Release
       - [ ] Update the GitHub Webhook
       - [ ] Comment on the Generated DOCSP Ticket
       - [ ] Update the EVG Project
       - [ ] Stop the Release Stopwatch (end time: HH:MM)
       - [ ] Record the Release
   - [ ] vcpkg
   - [ ] Conan


Check Static Analysis
#####################

Check Coverity, fix high-impact issues, and generate a static analysis report.
`Follow the process outlined in this document`__ for the *C Driver* project in
Coverity.

__ https://docs.google.com/document/d/1rkFL8ymbkc0k8Apky9w5pTPbvKRm68wj17mPJt2_0yo

.. note::

   This step may require additional code changes that can delay the release!


Check that Tests Are Passing
############################

Ensure that the latest commits on the branch are successful in CI.

- For minor releases, `refer to the tests in the latest build <latest-build_>`_
- For patch releases, `refer to the latest runs for the branch project <evg-release_>`_.

.. warning::

   Be sure that you are looking at the correct branch in the Project Health
   page! The branch/release version will be displayed in the *Project* dropdown
   near the top of the page.

If the project health page displays task failures, ensure that they are not
unexpected by the changes introduced in the new release.


.. _releasing.sbom:

Check and Update the SBOM Lite and `etc/purls.txt`
##################################################

Check that the `etc/purls.txt` file is up-to-date with the set of
:term:`vendored dependencies <vendored dependency>`. If any items need to be
updated, refer to `sbom-lite-updating`.

Create a New Clone of ``mongo-c-driver``
########################################

To prevent publishing unwanted changes and to preserve local changes, create a
fresh clone of the C driver. We will clone into a new arbitrary directory which
we will refer to as ``$RELEASE_CLONE`` ::

   $ git clone "git@github.com:mongodb/mongo-c-driver.git" $RELEASE_CLONE

.. note:: Unless otherwise noted, all commands below should be executed from within
   the ``$RELEASE_CLONE`` directory.

Switch to a branch that corresponds to the release version:

- **If performing a minor release (x.y.0)**, create a new branch for the
  major+minor release version. For example: If the major version is ``5`` and
  the minor version is ``42``, create a branch ``r5.42``::

      $ git checkout master      # Ensure we are on the `master` branch to begin
      $ git checkout -b "r5.42"  # Create and switch to a new branch

  Push the newly created branch into the remote::

      $ git push origin "r5.42"

- **If performing a patch release (x.y.z)**, there should already exist a
  release branch corresponding to the major+minor version of the patch. For
  example, if we are releasing patch version ``7.8.9``, then there should
  already exist a branch ``r7.8``. Switch to that branch now::

      $ git checkout --track origin/r7.8

.. _releasing.snyk:

Start Snyk Monitoring
#####################

We wish to track vulnerability information within bundled dependencies for
releases until such releases are no longer supported. We use Snyk_ to perform
this monitoring.

.. seealso:: `snyk scanning` for information on how Snyk is used

.. program:: +snyk-monitor-snapshot

To enable Snyk monitoring for a release, execute the `+snyk-monitor-snapshot`
Earthly target for the release branch to be monitored. Be sure to specify the
correct branch name with `--branch`, and use `--name` to identify the snapshot
as belonging to the new release version. Let ``$RELEASE_BRANCH`` being the name
of the branch from which we are releasing (e.g. ``r1.27``), and let ``$NEW_VERSION`` be the new
release version that we are posting (e.g. ``1.27.6``):

.. code-block:: console

   $ tools/earthly.sh +snyk-monitor-snapshot --branch "$RELEASE_BRANCH" --name="release-$NEW_VERSION"

.. note::

   If any subsequent step requires modifying the repository on that branch,
   re-run the `+snyk-monitor-snapshot` command to renew the Snyk monitor.

.. _releasing.vuln-report:

Address and Report Vulnerabilities in Dependencies
##################################################

Update the `etc/third_party_vulnerabilities.md` file according to the details
currently available in the Snyk web UI for the C driver target. See
`vuln-reporting` for more information on this process.

If there are new unaddressed vulnerabilities for the pending release, *and* an
upstream fix is available, *and* performing an upgrade is a simple enough
option, create a new changeset that will upgrade that dependency so that a fix
is available for the release.

.. note::

   This action must be performed on the branch from which the release will be
   created.

.. important::

   If any dependency was upgraded to remove vulnerabilities, return to
   `releasing.sbom`.


Validate that New APIs Are Documented
#####################################

The Evergreen CI task *abi-compliance-check* generates an "ABI Report"
``compat_report.html`` with an overview of all new/removed/changed symbols since
the prior release of the C driver.

Visit the most recent Evergreen build for the project, open the
*abi-compliance-check* task, go to the *Files* tab, and open the *ABI Report:
compat_report.html* artifact. In the *Added Symbols* section will be a list of
all newly introduced APIs since the most release release version. Verify that
documentation has been added for every symbol listed here. If no new symbols are
added, then the documentation is up-to-date.


.. _release.github-token:

Get a GitHub API Token
######################

Later, we will use an automated script to publish the release artifacts to
GitHub and create the GitHub Release object. In order to do this, it is required
to have a GitHub API token that can be used to create and modify the releases
for the repository.

To get an access token, perform the following:

1. Open the `Settings > Personal access tokens`__ page on GitHub.
2. Press the *Generate new token* dropdown.

   1. Select a "general use"/\ "classic" token. (Creating a fine-grained access
      token requires administrative approval before it can be used.)

3. Set a *note* for the token that explains its purpose. This can be arbitrary,
   but is useful when reviewing the token later.
4. Set the expiration to the minimum (we only need the token for the duration of
   this release).
5. In the scopes, enable the ``public_repo`` and ``repo_deployment`` scopes.
6. Generate the new token. Be sure to copy the access token a save it for later,
   as it won't be recoverable once the page is unloaded!
7. Grant the token access to the ``mongodb`` organization using the "Configure
   SSO" dropdown.

__ https://github.com/settings/tokens

.. XXX: The following applies to fine-grained access tokens. Not sure if these work yet?

   1. Open the `Settings > Personal access tokens`__ page on GitHub.
   2. Press the *Generate new token* dropdown.

      1. Select a "Find-grained, repo-scoped" token. The general use token is also
         acceptable but is very coarse and not as restricted.

   3. Set a token name. This can be arbitrary, but would be best to refer to the
      purpose so that it can be recognized later.
   4. Set the expiration to the minimum (we only need the token for the duration of
      this release).
   5. Set the *Resource owner* to **mongodb** (**mongodb** refers to the GitHub
      organization that owns the repository that will contain the release. A
      personal account resource owner will only have access to the personal
      repositories.)
   6. Under *Repository access* choose "Only select repositories".
   7. In the repository selection dropdown, select ``mongodb/mongo-c-driver``.
   8. Under *Permissions > Repository permissions*, set the access level on
      *Contents* to *Read and write*. This will allow creating releases and
      publishing release artifacts. No other permissions need to be modified.
      (Selecting this permission may also enable the *Metadata* permission; this is
      normal.)

Join the Release Team
#####################

The release process may require creating new branches, new tags, and directly
pushing to development branches. These operations are normally restricted by
branch protection rules.

When assigned the responsibility of performing a release, submit a request to a
repository administrator to be temporarily added to the
`releases team <dbx-c-cxx-releases-github_>`_ on GitHub for the duration of the
release process. The team member must be added via
`MANA <dbx-c-cxx-releases-mana_>`_ (the GitHub team should normally be empty,
meaning there should not be any member with the "Maintainer" role to add new
users via GitHub).

Do the Release
##############

.. highlight:: console
.. default-role:: bash

The release process at this point is semi-automated by scripts stored in a
separate repository.

.. hint::

   It may be useful (but is not required) to perform the following steps within
   a new Python `virtual environment`__ dedicated to the process.

__ https://docs.python.org/3/library/venv.html


.. _do.stopwatch:

Start a Release Stopwatch
*************************

Start a stopwatch before proceeding.


Clone the Driver Tools
**********************

Clone the driver tools repository into a new directory, the path to which will be
called `$CDRIVER_TOOLS`::

   $ git clone "git@github.com:10gen/mongo-c-driver-tools.git" $CDRIVER_TOOLS

Install the Python requirements for the driver tools::

   $ pip install -r $CDRIVER_TOOLS/requirements.txt


**For Patch Releases**: Check Consistency with the Jira Release
***************************************************************

**If we are releasing a patch version**, we must check that the Jira release
matches the content of the branch to be released. Open
`the releases page on Jira <Jira releases_>`_ and open the release page for the new patch
release. Verify that the changes for all tickets in the Jira release have been
cherry-picked onto the release branch (not including the "Release x.y.z" ticket
that is part of every Jira release).

.. _Jira releases:
.. _jira-releases: https://jira.mongodb.org/projects/CDRIVER?selectedItem=com.atlassian.jira.jira-projects-plugin%3Arelease-page&status=unreleased


Run the Release Script
**********************

Start running the release script:

1. Let `$PREVIOUS_VERSION` be the prior ``x.y.z`` version of the C driver
   that was released.
2. Let `$NEW_VERSION` be the ``x.y.z`` version that we are releasing.
3. Run the Python script::

      $ python $CDRIVER_TOOLS/release.py release $PREVIOUS_VERSION $NEW_VERSION


Fixup the ``NEWS`` Pages
************************

Manually edit the `$RELEASE_CLONE/NEWS` and `$RELEASE_CLONE/src/libbson/NEWS`
files with details of the release. **Do NOT** commit any changes to these files
yet: That step will be handled automatically by the release script in the next
steps.


.. _do.upload:

Sign & Upload the Release
*************************

Run the ``release.py`` script to sign the release objects::

   $ python $CDRIVER_TOOLS/release.py sign

Let `$GITHUB_TOKEN` be the personal access token that was obtained from the
:ref:`release.github-token` step above. Use the token with the ``upload`` subcommand
to post the release to GitHub:

.. note:: This will create the public release object on GitHub!

.. note:: If this is a pre-release, add the `--pre` option to the `release.py upload` command below.

::

   $ python $CDRIVER_TOOLS/release.py upload $GITHUB_TOKEN

Update the :file:`VERSION_CURRENT` file on the release branch::

   $ python $CDRIVER_TOOLS/release.py post_release_bump


Publish Additional Artifacts
****************************

.. note::

   This is currently a manual additional process, but may be automated to be
   part of the release scripts in the future.


.. warning::
   This step should be run using the ``master`` branch, regardless of
   which branch is used for the release.

We publish a release archive that contains a snapshot of the repository and some
additional metadata, along with an OpenPGP signature of that archive. This
archive is created using scripts in the C driver repository itself, not in
`$CDRIVER_TOOLS`.


.. _releasing.gen-archive:

Generate the Release Artifacts
==============================

The release artifacts are generated using :doc:`Earthly <earthly>`.
Specifically, it is generated using the :any:`+signed-release` target. Before
running :any:`+signed-release`, one will need to set up some environment that is
required for it to succeed:

1. :ref:`Authenticate with the DevProd-provided Amazon ECR instance <earthly.amazon-ecr>`
2. Set the Earthly secrets required for the :any:`+sign-file` target.
3. Download an augmented SBOM from a recent execution of the ``sbom`` task in
   an Evergreen patch or commit build and save it to ``etc/augmented-sbom.json``.

Once these prerequesites are met, creating the release archive can be done using
the :any:`+signed-release` target.::

   $ ./tools/earthly.sh --artifact +signed-release/dist dist --version=$NEW_VERSION

.. note:: `$NEW_VERSION` must correspond to the Git tag created by the release.

The above command will create a `dist/` directory in the working directory that
contains the release artifacts from the :any:`+signed-release/dist/` directory
artifact. The generated filenames are based on the
:any:`+signed-release --version` argument. The archive contents come from the
Git tag corresponding to the specified version. The detached PGP signature is
the file with the `.asc` extension and corresponds to the archive file with the
same name without the `.asc` suffix.

.. code-block::
   :caption: Example :any:`+signed-release` output with `$NEW_VERSION="1.27.2"`

   $ ls dist/
   mongo-c-driver-1.27.2.tar.gz
   mongo-c-driver-1.27.2.tar.gz.asc

.. note::

   The public key that corresponds to the signature is available at
   https://pgp.mongodb.com/c-driver.pub


Attach the Release Artifacts
============================

In the :ref:`do.upload` step, a GitHub release was created. Navigate to that
GitHub release and edit the release to attach additional artifacts. Attach the
files from :any:`+signed-release/dist/` to the newly created release.


Publish Documentation
*********************

**If this is a stable release** (not a pre-release), publish the documentation
with the following command::

   $ python $CDRIVER_TOOLS/release.py docs $NEW_VERSION


Announce the Release on the Community Forums
********************************************

Open the `MongoDB Developer Community / Product & Driver Announcments`__ page on
the Community Forums and prepare a new post for the release.

__ https://www.mongodb.com/community/forums/c/announcements/driver-releases/110

To generate the release template text, use the following::

   $ git checkout $RELEASE_BRANCH
   $ python $CDRIVER_TOOLS/release.py announce -t community $NEW_VERSION

Update/fix-up the generated text for the new release and publish the new post.

.. seealso::

   `An example of a release announcment post`__

   __ https://www.mongodb.com/community/forums/t/mongodb-c-driver-1-24-0-released/232021


Copy the Release Updates to the ``master`` Branch
*************************************************

Create a new branch from the C driver ``master`` branch, which will be used to
publish a PR to merge the updates to the release files back into ``master``::

   $ git checkout master
   $ git checkout -b post-release-merge

(Here we have named the branch ``post-release-merge``, but the branch name is
arbitrary.)

Do the following:

1. Manually update the ``NEWS`` and ``src/libbson/NEWS`` files with the content
   from the release branch that we just published. Commit these changes to the
   new branch.
2. For a non-patch release, manually update the :file:`VERSION_CURRENT` file.
   Example: if ``1.28.0`` was just released, update to ``1.29.0-dev``.
3. For a non-patch release, update the :file:`etc/prior_version.txt` file to
   contain the version that you have just released. This text should match the
   generated Git tag. (The tag should always be an ancestor of the branch that
   contains that :file:`etc/prior_version.txt`.)

Push this branch to your fork of the repository::

   $ git push git@github.com:$YOUR_GH_USERNAME/mongo-c-driver.git post-release-merge

Now `create a new GitHub Pull Request`__ to merge the ``post-release-merge``
changes back into the ``master`` branch.

__ https://github.com/mongodb/mongo-c-driver/pulls


Leave the Release Team
**********************

Remove yourself from the `releases team <dbx-c-cxx-releases-github_>`_ on GitHub
via `MANA <dbx-c-cxx-releases-mana_>`_.


.. _releasing.jira:

Close the Jira Release Ticket and Finish the Jira Release
*********************************************************

Return to the `Jira releases`_ page and open the release for the release
version. Close the *Release x.y.z* ticket that corresponds to the release and
"Release" that version in Jira, ensuring that the release date is correct. (Do
not use the "Build and Release" option)


Update GitHub Webhook
*********************

For a non-patch release, update the `Github Webhook <https://wiki.corp.mongodb.com/display/INTX/Githook>`_
to include the new branch.

Navigate to the `Webhook Settings <https://github.com/mongodb/mongo-c-driver/settings/hooks>`_.

Click ``Edit`` on the hook for ``https://githook.mongodb.com/``.

Add the new release branch to the ``Payload URL``. Remove unmaintained
release branches.


Comment on the Generated DOCSP Ticket
*************************************

.. note:: This step is not applicable for patch releases.

After a **minor** or **major** release is released in Jira (done in the previous
step), a DOCSP "Update Compat Tables" ticket will be created automatically
(`example DOCSP ticket`__). Add a comment to the newly created ticket for the
release describing if there are any changes needed for the
`driver/server compatibility matrix`__ or the
`C language compatibility matix`__.

__ https://jira.mongodb.org/browse/DOCSP-39145
__ https://www.mongodb.com/docs/languages/c/c-driver/current/#mongodb-compatibility
__ https://www.mongodb.com/docs/languages/c/c-driver/current/#language-compatibility


Update the Release Evergreen Project
************************************

**For minor releases**, open the
`release project settings <evg-release-settings_>`_ and update the *Display
Name* and *Branch Name* to match the new major+minor release version.


Stop the Stopwatch & Record the Release
***************************************

Stop the stopwatch started at :ref:`do.stopwatch`. Record the the new release
details in the `C/C++ Release Info`__ sheet.

__ https://docs.google.com/spreadsheets/d/1yHfGmDnbA5-Qt8FX4tKWC5xk9AhzYZx1SKF4AD36ecY/edit#gid=0


Linux Distribution Packages
###########################

.. ! NOTE: Updates to these instructions should be synchronized to the corresponding
   ! C++ release process documentation located in the "etc/releasing.md" file in the C++
   ! driver repository

Debian
******

.. seealso::

   The Debian packaging and releasing process are detailed on the :doc:`debian`
   page.

Fedora
******

After the changes for `CDRIVER-3957`__, the RPM spec file has been vendored into
the project; it needs to be updated periodically. The DBX C/C++ team does not
maintain the RPM spec file. These steps can be done once the RPM spec file is
updated (which will likely occur some time after the C driver is released).

__ https://jira.mongodb.org/browse/CDRIVER-3957

1. From the project's root directory, download the latest spec file::

      $ curl -L -o .evergreen/mongo-c-driver.spec https://src.fedoraproject.org/rpms/mongo-c-driver/raw/rawhide/f/mongo-c-driver.spec

2. Confirm that our spec patch applies to the new downstream spec::

      $ patch --dry-run -d .evergreen/etc -p0 -i spec.patch

3. If the patch command fails, rebase the patch on the new spec file.
4. For a new major release (e.g., 1.17.0, 1.18.0, etc.), then ensure that the
   patch updates the `up_version` to be the NEXT major version (e.g., when
   releasing 1.17.0, the spec patch should update `up_version`` to 1.18.0); this
   is necessary to ensure that the spec file matches the tarball created by the
   dist target; if this is wrong, then the `rpm-package-build` task will fail in
   the next step.
5. Additionally, ensure that any changes made on the release branch vis-a-vis
   the spec file are also replicated on the master or main branch.
6. Test the RPM build in Evergreen with a command such as the following::

      $ evergreen patch -p mongo-c-driver -v packaging -t rpm-package-build -f

7. There is no package upload step, since the downstream maintainer handles that
   and we only have the Evergreen task to ensure that we do not break the
   package build.
8. The same steps need to be executed on active release branches (e.g., r1.19),
   which can usually be accomplished via `git cherry-pick` and then resolving
   any minor conflict.


vcpkg
#####

To update the package in vcpkg, create an issue to update
`the mongo-c-driver manifest`__. To submit an issue, `follow the steps here`__
(`example issue`__).

Await a community PR to resolve the issue, or submit a new PR.

__ https://github.com/microsoft/vcpkg/blob/master/versions/m-/mongo-c-driver.json
__ https://github.com/microsoft/vcpkg/issues/new/choose
__ https://github.com/microsoft/vcpkg/issues/34855


Conan
#####

Create a new issue in the conan-center-index project to update `the recipe files
for the C driver package`__. To submit an issue, `follow the process
here`__ (`example issue`__)

Await a community PR to resolve the issue, or submit a new PR.

__ https://github.com/conan-io/conan-center-index/blob/master/recipes/mongo-c-driver/config.yml
__ https://github.com/conan-io/conan-center-index/issues/new/choose/
__ https://github.com/conan-io/conan-center-index/issues/20879


Docker
######

The C driver does not have its own container image, but it may be useful to
update the C driver used in the C++ container image build.

If the C driver is being released without a corresponding C++ driver release, consider
updating the mongo-cxx-driver container image files to use the newly released C driver
version. `Details for this process are documented here`__

__ https://github.com/mongodb/mongo-cxx-driver/blob/5f2077f98140ea656983ea5881de31d73bb3f735/etc/releasing.md#docker-image-build-and-publish
