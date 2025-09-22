Debian Packaging
################

.. highlight:: console
.. default-role:: bash

Release Publishing
******************

.. ! NOTE: Updates to these instructions should be synchronized to the corresponding
   ! C++ release process documentation located in the "etc/releasing.md" file in the C++
   ! driver repository

.. note::

    The Debian package release is made only after the upstream release has been
    tagged.

    After a C driver release is completed (i.e. the :ref:`releasing.jira` step
    of the release process), a new Jira ticket will be automatically created to
    track the work of the corresponding release of the Debian package for the C
    driver (`example ticket <https://jira.mongodb.org/browse/CDRIVER-5554>`__).

To publish a new release Debian package, perform the following:

1. Change to the packaging branch, ``git checkout debian/unstable``, and make sure
   the working directorty is clean, ``git status``, and up-to-date, ``git pull``.
2. Because it is possible to have divergences between release branches, some
   special procedures are needed. Execute the following sequence of commands
   (substituting version numbers as appropriate):

.. code-block:: console

   $ git merge --no-commit --no-ff 2.x.y        # may result in conflicts
   $ git checkout HEAD -- debian                # ensures debian/ dir is preserved
   $ git add .                                  # prepare to resolve conflicts
   $ git checkout --no-overlay 2.x.y -- . ':!debian'   # resolve conflicts
   $ git add .
   $ git commit

3. Verify that there are no extraneous differences from the release tag,
   ``git diff 2.x.y..HEAD --stat -- . ':!debian'``; the command should produce
   no output, and if any output is shown then that indicates differences in
   files outside the ``debian/`` directory.
4. If there were any files outside the ``debian/`` directory listed in the last
   step then something has gone wrong. Discard the changes on the branch and
   start again.
5. Create a new changelog entry (use the command ``dch -i`` to ensure proper
   formatting), then adjust the version number on the top line of the changelog
   as appropriate.
6. Make any other necessary changes to the Debian packaging components (e.g.,
   update to standards version, dependencies, descriptions, etc.) and make
   relevant entries in ``debian/changelog`` as needed.
7. Use ``git add`` to stage the changed files for commit (only files in the
   ``debian/`` directory should be committed), then commit them (the ``debcommit``
   utility is helpful here).
8. Build the package with ``gbp buildpackage`` and inspect the resulting package
   files (at a minimum use ``debc`` on the ``.changes`` file in order to confirm
   files are installed to the proper locations by the proper packages and also
   use ``lintian`` on the ``.changes`` file in order to confirm that there are no
   unexpected errors or warnings; the ``lintian`` used for this check should
   always be the latest version as it is found in the unstable distribution).
9. If any changes are needed, make them, commit them, and rebuild the package.

   .. note:: It may be desirable to squash multiple commits down to a single commit before building the final packages.

10. Mark the package ready for release with the ``dch -r`` command, commit the
    resulting changes (after inspecting them),
    ``git commit debian/changelog -m 'mark ready for release'``.
11. Build the final packages. Once the final packages are built, they can be
    signed and uploaded and the version can be tagged using the ``--git-tag``
    option of ``gbp buildpackage``. The best approach is to build the packages,
    prepare everything and then upload. Once the archive has accepted the
    upload, then execute
    ``gbp buildpackage --git-tag --git-tag-only --git-sign-tags`` and push the
    commits on the ``debian/unstable`` branch as well as the new signed tag.
