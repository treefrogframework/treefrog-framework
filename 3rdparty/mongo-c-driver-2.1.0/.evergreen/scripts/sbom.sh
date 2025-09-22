#!/usr/bin/env bash

set -o errexit
set -o pipefail

: "${branch_name:?}"
: "${DOCKER_CONFIG:?}"
: "${KONDUKTO_TOKEN:?}"

command -v podman >/dev/null || {
  echo "missing required program podman" 1>&2
  exit 1
}

silkbomb="901841024863.dkr.ecr.us-east-1.amazonaws.com/release-infrastructure/silkbomb:2.0"

# Ensure latest version of SilkBomb is being used.
podman pull "${silkbomb:?}"

silkbomb_augment_flags=(
  --repo mongodb/mongo-c-driver
  --branch "${branch_name:?}"
  --sbom-in /pwd/etc/cyclonedx.sbom.json
  --sbom-out /pwd/augmented-sbom.json

  # Any notable updates to the Augmented SBOM version should be done manually after careful inspection.
  # Otherwise, it should be equal to the SBOM Lite version, which should normally be `1`.
  --no-update-sbom-version
)

# First validate the SBOM Lite.
podman run -it --rm -v "$(pwd):/pwd" "${silkbomb:?}" \
  validate --purls /pwd/etc/purls.txt --sbom-in /pwd/etc/cyclonedx.sbom.json --exclude jira

# Then download the Augmented SBOM. Allow the timestamp to be updated.
podman run -it --rm -v "$(pwd):/pwd" --env 'KONDUKTO_TOKEN' "${silkbomb:?}" \
  augment "${silkbomb_augment_flags[@]:?}"

[[ -f ./augmented-sbom.json ]] || {
  echo "failed to download Augmented SBOM" 1>&2
  exit 1
}
