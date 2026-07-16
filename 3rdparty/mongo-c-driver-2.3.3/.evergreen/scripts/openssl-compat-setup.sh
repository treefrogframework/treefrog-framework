#!/usr/bin/env bash

set -o errexit
set -o pipefail

# shellcheck source=.evergreen/scripts/env-var-utils.sh
. "$(dirname "${BASH_SOURCE[0]}")/env-var-utils.sh"
. "$(dirname "${BASH_SOURCE[0]}")/use-tools.sh" paths

check_var_req OPENSSL_VERSION
check_var_opt OPENSSL_ENABLE_FIPS

declare script_dir
script_dir="$(to_absolute "$(dirname "${BASH_SOURCE[0]}")")"

# Use ccache if able.
. "${script_dir:?}/find-ccache.sh"

mongoc_dir="$(to_absolute "${script_dir:?}/../..")"
openssl_source_dir="${mongoc_dir:?}/openssl-${OPENSSL_VERSION:?}"
openssl_install_dir="${mongoc_dir:?}/openssl-install-dir"

. "${mongoc_dir:?}/.evergreen/scripts/openssl-downloader.sh"

openssl_download "${OPENSSL_VERSION:?}"

rm -rf "${openssl_install_dir:?}"
mkdir "${openssl_install_dir:?}" # For openssl.cnf.

declare -a config_flags=(
  "--prefix=${openssl_install_dir:?}"
  "--openssldir=${openssl_install_dir:?}/ssl"
  "--libdir=lib"
  "shared"                              # Enable shared libraries.
  "-fPIC"                               # For static libraries.
  "-Wl,-rpath,${openssl_install_dir:?}" # For shared libraries.
)

if [[ "${OPENSSL_ENABLE_FIPS:-}" == "ON" ]]; then
  config_flags+=("enable-fips")
fi

echo "Building and installing OpenSSL ${OPENSSL_VERSION:?}..."
(
  cd "${openssl_source_dir:?}"

  export MAKEFLAGS="--no-print-directory"
  CC="ccache gcc" ./config "${config_flags[@]:?}"

  # Silence: `WARNING: can't open config file: <prefix>/openssl.cnf` during build.
  cp apps/openssl.cnf "${openssl_install_dir:?}"

  if [[ "${OPENSSL_ENABLE_FIPS:-}" == "ON" ]]; then
    # Use FIPS by default: https://docs.openssl.org/master/man7/fips_module/
    perl -i'' -p \
      -e "s|# (.include fipsmodule.cnf)|.include ${openssl_install_dir:?}/ssl/fipsmodule.cnf|;" \
      -e "s|# (fips = fips_sect)|\$1\n\n[algorithm_sect]\ndefault_properties = fips=yes\n|;" \
      -e "s|(providers = provider_sect)|\$1\nalg_section = algorithm_sect|;" \
      -e "s|# (activate = 1)|\$1|;" \
      "${openssl_install_dir:?}/openssl.cnf"
  fi

  case "${OPENSSL_VERSION:?}" in
  # Parallel builds can be flaky for some versions.
  1.0.2) make ;;

  # Seems fine.
  *) make -j "$(nproc)" ;;
  esac

  make --no-print-directory install_sw

  if [[ "${OPENSSL_ENABLE_FIPS:-}" == "ON" ]]; then
    make install_ssldirs # For ssl/fipsmodule.cnf.
    make install_fips    # For lib/lib/ossl-modules/fips.so.

    # Post-installation attention.
    env \
      PATH="${openssl_install_dir:?}/bin:${PATH:-}" \
      LD_LIBRARY_PATH="${openssl_install_dir:?}/lib:${LD_LIBRARY_PATH:-}" \
      openssl fipsinstall \
      -out "${openssl_install_dir:?}/ssl/fipsmodule.cnf" \
      -module "${openssl_install_dir:?}/lib/ossl-modules/fips.so" \
      -quiet

    # Verification.
    echo "Verifying OpenSSL FIPS 3.0 module is enabled..."
    declare providers
    providers="$(./util/wrap.pl -fips apps/openssl list -provider-path providers -provider fips -providers)"
    [[ -n "$(echo "${providers:-}" | grep "OpenSSL FIPS Provider")" ]] || {
      echo "missing \"OpenSSL FIPS Provider\" in: ${providers:-}"
    } >&2
    echo "Verifying OpenSSL FIPS 3.0 module is enabled... done."
  fi
) >/dev/null
echo "Building and installing OpenSSL ${OPENSSL_VERSION:?}... done."
