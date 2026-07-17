#!/usr/bin/env bash

# shellcheck disable=SC2034

openssl_version_to_url() {
  declare version
  version="${1:?"usage: openssl_version_to_url <version>"}"

  command -v perl >/dev/null || return

  declare uversion # 1.2.3 -> 1_2_3
  uversion="$(echo "${version:?}" | perl -lpe 's|\.|_|g')" || return

  declare download_url
  if [[ "${version:?}" == 1.0.2 ]]; then
    url="https://github.com/openssl/openssl/releases/download/OpenSSL_${uversion:?}u/openssl-${version:?}u.tar.gz"
  elif [[ "${version:?}" == 1.1.1 ]]; then
    url="https://github.com/openssl/openssl/releases/download/OpenSSL_${uversion:?}w/openssl-${version:?}w.tar.gz"
  else
    url="https://github.com/openssl/openssl/releases/download/openssl-${version:?}/openssl-${version:?}.tar.gz"
  fi

  echo "${url:?}"
}

# Download the requested OpenSSL version into `openssl-<version>`.
openssl_download() {
  declare version
  version="${1:?"usage: openssl_download <version>"}"

  command -v curl perl tar sha256sum >/dev/null || return

  declare url
  url="$(openssl_version_to_url "${version:?}")" || return

  declare openssl_checksum_1_0_2="ecd0c6ffb493dd06707d38b14bb4d8c2288bb7033735606569d8f90f89669d16"
  declare openssl_checksum_1_1_1="cf3098950cb4d853ad95c0841f1f9c6d3dc102dccfcacd521d93925208b76ac8"
  declare openssl_checksum_3_0_9="eb1ab04781474360f77c318ab89d8c5a03abc38e63d65a603cabbf1b00a1dc90" # FIPS 140-2
  declare openssl_checksum_3_0_17="dfdd77e4ea1b57ff3a6dbde6b0bdc3f31db5ac99e7fdd4eaf9e1fbb6ec2db8ce"
  declare openssl_checksum_3_1_2="a0ce69b8b97ea6a35b96875235aa453b966ba3cba8af2de23657d8b6767d6539" # FIPS 140-3
  declare openssl_checksum_3_1_8="d319da6aecde3aa6f426b44bbf997406d95275c5c59ab6f6ef53caaa079f456f"
  declare openssl_checksum_3_2_5="b36347d024a0f5bd09fefcd6af7a58bb30946080eb8ce8f7be78562190d09879"
  declare openssl_checksum_3_3_4="8d1a5fc323d3fd351dc05458457fd48f78652d2a498e1d70ffea07b4d0eb3fa8"
  declare openssl_checksum_3_4_2="17b02459fc28be415470cccaae7434f3496cac1306b86b52c83886580e82834c"
  declare openssl_checksum_3_5_1="529043b15cffa5f36077a4d0af83f3de399807181d607441d734196d889b641f"
  declare openssl_checksum_4_0_0="c32cf49a959c4f345f9606982dd36e7d28f7c58b19c2e25d75624d2b3d2f79ac"

  declare checksum_name
  checksum_name="openssl_checksum_$(echo "${version:?}" | perl -lpe 's|\.|_|g')" || return

  [[ -n "$(eval "echo \${${checksum_name:-}:-}")" ]] || {
    echo "missing checksum for OpenSSL version \"${version:?}\""
    return 1
  } >&2

  declare tarfile
  tarfile="openssl-${version:?}.tar.gz"

  echo "Downloading OpenSSL ${version:?}..."
  curl -sSL -o "${tarfile:?}" "${url:?}" || return
  echo "Downloading OpenSSL ${version:?}... done."

  echo "${!checksum_name:?} ${tarfile:?}" | sha256sum -c >/dev/null || return

  echo "Decompressing openssl-${version:?}.tar.gz..."
  tar --one-top-level="openssl-${version:?}" --strip-components=1 -xzf "${tarfile:?}" || return
  echo "Decompressing openssl-${version:?}.tar.gz... done."
}
