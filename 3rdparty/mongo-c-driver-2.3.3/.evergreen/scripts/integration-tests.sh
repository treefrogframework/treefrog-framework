#!/usr/bin/env bash
#
# Start up mongo-orchestration (a server to spawn mongodb clusters) and set up a cluster.
#
# Specify the following environment variables:
#
# MONGODB_VERSION: latest, 4.2, 4.0
# TOPOLOGY: server, replica_set, sharded_cluster
# AUTH: auth, noauth
# SSL: openssl, darwinssl, winssl, nossl
# ORCHESTRATION_FILE: <file name in DET configs/${TOPOLOGY}s/>
# REQUIRE_API_VERSION: set to a non-empty string to set the requireApiVersion parameter
#   This is currently only supported for standalone servers
# LOAD_BALANCER: off, on
#
# This script may be run locally.
#

set -o errexit # Exit the script with error if any of the commands fail

# shellcheck source=.evergreen/scripts/env-var-utils.sh
. "$(dirname "${BASH_SOURCE[0]:?}")/env-var-utils.sh"
. "$(dirname "${BASH_SOURCE[0]:?}")/use-tools.sh" paths

: "${AUTH:="noauth"}"
: "${LOAD_BALANCER:="off"}"
: "${MONGODB_VERSION:="latest"}"
: "${ORCHESTRATION_FILE:-}"
: "${REQUIRE_API_VERSION:-}"
: "${SSL:="nossl"}"
: "${TOPOLOGY:="server"}"

declare script_dir
script_dir="$(to_absolute "$(dirname "${BASH_SOURCE[0]:?}")")"

# By fetch-det.
export DRIVERS_TOOLS
DRIVERS_TOOLS="$(cd ../drivers-evergreen-tools && pwd)" # ./mongoc -> ./drivers-evergreen-tools
if [[ "${OSTYPE:?}" == cygwin ]]; then
  DRIVERS_TOOLS="$(cygpath -m "${DRIVERS_TOOLS:?}")"
fi

export MONGO_ORCHESTRATION_HOME="${DRIVERS_TOOLS:?}/.evergreen/orchestration"
export MONGODB_BINARIES="${DRIVERS_TOOLS:?}/mongodb/bin"
export PATH="${MONGODB_BINARIES:?}:$PATH"

"${DRIVERS_TOOLS:?}/.evergreen/run-mongodb.sh" start
