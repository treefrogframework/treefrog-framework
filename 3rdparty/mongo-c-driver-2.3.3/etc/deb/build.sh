#!/usr/bin/env bash

set -euo pipefail

## Script parameters
# Directory with the mongo-c-driver repository
: "${MCD_DIR:=$(dirname "$(dirname "$(dirname "${BASH_SOURCE[0]}")")")}"
# The Git remote from which we will pull the Debian package control files
: "${DEBIAN_REMOTE:="https://github.com/mongodb/mongo-c-driver"}"
# The branch within the remote that will be used for package control files
: "${DEBIAN_BRANCH:="debian/unstable"}"
# Directory where temporary files will be written
: "${SCRATCH_DIR:=$PWD/.scratch}"

# Ensure our scratch space exists
mkdir -p -- "$SCRATCH_DIR"

# Pull Debian control files branch into a temporary branch so that we can check them out
printf "Pulling Debian packages files from %s@%s\n" "$DEBIAN_BRANCH" "$DEBIAN_REMOTE"
# Force update the branch to contain the content from the remote
tmp_branch=debian-control-files-branch-tmp
git -C "$MCD_DIR" fetch --quiet --force --depth=1 "$DEBIAN_REMOTE" "$DEBIAN_BRANCH:$tmp_branch"
# Grab the `debian/` subdirectory from the branch
git -C "$MCD_DIR" checkout $tmp_branch -- debian/
# Delete the temporary branch
git -C "$MCD_DIR" branch --quiet -D $tmp_branch

# Temporarily apply and remove patches to test that they apply cleanly.
printf "Checking Debian patches...\n"
export QUILT_PATCHES="$MCD_DIR/debian/patches"
export QUILT_PC="$SCRATCH_DIR/quilt.pc"
env -C "$MCD_DIR" quilt push -a --refresh
env -C "$MCD_DIR" quilt pop -a

# Get the package name according to the changelog file
dch_pkg_name=$(dpkg-parsechangelog --show-field Source --file "$MCD_DIR/debian/changelog")
# The full version
dch_pkg_version=$(dpkg-parsechangelog --show-field Version --file "$MCD_DIR/debian/changelog")
# Get the version number without the version suffix
dch_base_version=$(sed -r 's/-[^-]+$//' <<< "$dch_pkg_version")
printf "Upstream package %s version %s\n" "$dch_pkg_name" "$dch_base_version"
# Snapshot version includes date and version information
snapshot_version="$dch_base_version-0+$(date +%Y%m%d)+git$(git -C "$MCD_DIR" rev-parse --short HEAD)"

# Create a new snapshot version for the current date and Git hash
echo "Creating snapshot changelog entry"
dch --force-bad-version \
    --release-heuristic log \
    --changelog "$MCD_DIR/debian/changelog" \
    --newversion "$snapshot_version" \
    --distribution UNRELEASED \
    "Built from Git snapshot."

gbp_options=(
    # Use the working copy rather than a specific commit. Also disables modification checks.
    --git-export=WC
    --git-no-pbuilder
    --git-verbose
)

env -C "$MCD_DIR" gbp buildpackage "${gbp_options[@]}"
