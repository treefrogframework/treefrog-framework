#!/usr/bin/env bash

set -o errexit
set -o pipefail

set +o xtrace # Don't echo commands

# shellcheck source=.evergreen/scripts/env-var-utils.sh
. "$(dirname "${BASH_SOURCE[0]}")/env-var-utils.sh"
. "$(dirname "${BASH_SOURCE[0]}")/use-tools.sh" platform paths

declare script_dir
script_dir="$(to_absolute "$(dirname "${BASH_SOURCE[0]}")")"

declare mongoc_dir
mongoc_dir="$(to_absolute "${script_dir}/../..")"

declare install_dir="${mongoc_dir}/install-dir"
declare openssl_install_dir="${mongoc_dir}/openssl-install-dir"

# Create directory for secrets within Evergreen task directory. Task directory is cleaned up between tasks.
declare secrets_dir
secrets_dir="$(to_absolute "${mongoc_dir}/../secrets")"
mkdir -p "${secrets_dir}"
chmod 700 "${secrets_dir}"

# Create certificate to test X509 auth with Atlas on cloud-prod:
atlas_x509_path="${secrets_dir:?}/atlas_x509.pem"
echo "${atlas_x509_cert_base64:?}" | base64 --decode > "${secrets_dir:?}/atlas_x509.pem"
# Fix path on Windows:
if $IS_WINDOWS; then
    atlas_x509_path="$(cygpath -m "${atlas_x509_path}")"
fi

# Create certificate to test X509 auth with Atlas on cloud-dev
atlas_x509_dev_path="${secrets_dir:?}/atlas_x509_dev.pem"
echo "${atlas_x509_dev_cert_base64:?}" | base64 --decode > "${atlas_x509_dev_path:?}"
# Fix path on Windows:
if $IS_WINDOWS; then
    atlas_x509_dev_path="$(cygpath -m "${atlas_x509_dev_path}")"
fi

# Create Kerberos config and keytab files.
echo "Setting up Kerberos ... begin"
if command -v kinit >/dev/null; then
    # Copy host config and append realm:
    if [ -e /etc/krb5.conf ]; then
      cat /etc/krb5.conf > "${secrets_dir:?}/krb5.conf"
    fi
    cat "${mongoc_dir}/.evergreen/etc/kerberos.realm" >> "${secrets_dir:?}/krb5.conf"
    # Set up keytab:
    echo "${keytab:?}" | base64 --decode > "${secrets_dir:?}/drivers.keytab"
    # Initialize kerberos:
    KRB5_CONFIG="${secrets_dir:?}/krb5.conf" kinit -k -t "${secrets_dir:?}/drivers.keytab" -p drivers@LDAPTEST.10GEN.CC
    echo "Setting up Kerberos ... done"
else
    echo "No 'kinit' detected"
    echo "Setting up Kerberos ... skipping"
fi

declare c_timeout="connectTimeoutMS=30000&serverSelectionTryOnce=false"

declare sasl="OFF"
if grep -r -q "#define MONGOC_ENABLE_SASL 1" "${install_dir:?}"; then
  sasl="ON"
fi

declare ssl="OFF"
if grep -r -q "#define MONGOC_ENABLE_SSL 1" "${install_dir:?}"; then
  ssl="ON"
fi

# shellcheck source=.evergreen/scripts/add-build-dirs-to-paths.sh
. "${script_dir}/add-build-dirs-to-paths.sh"

# shellcheck source=.evergreen/scripts/bypass-dlclose.sh
. "${script_dir}/bypass-dlclose.sh"

declare ping
declare test_gssapi
declare ip_addr
case "${OSTYPE}" in
cygwin)
  ping="${mongoc_dir}/cmake-build/src/libmongoc/Debug/mongoc-ping.exe"
  test_gssapi="${mongoc_dir}/cmake-build/src/libmongoc/Debug/test-mongoc-gssapi.exe"
  ip_addr="$(getent hosts "${auth_host:?}" | head -n 1 | awk '{print $1}')"
  ;;

darwin*)
  ping="${mongoc_dir}/cmake-build/src/libmongoc/mongoc-ping"
  test_gssapi="${mongoc_dir}/cmake-build/src/libmongoc/test-mongoc-gssapi"
  ip_addr="$(dig "${auth_host:?}" +short | tail -1)"
  ;;

*)
  ping="${mongoc_dir}/cmake-build/src/libmongoc/mongoc-ping"
  test_gssapi="${mongoc_dir}/cmake-build/src/libmongoc/test-mongoc-gssapi"
  ip_addr="$(getent hosts "${auth_host:?}" | head -n 1 | awk '{print $1}')"
  ;;
esac
: "${ping:?}"
: "${test_gssapi:?}"
: "${ip_addr:?}"

# Archlinux (which we use for testing various self-installed OpenSSL versions)
# stores their trust list under /etc/ca-certificates/extracted/.
# We need to copy it to our custom installed OpenSSL trust store.
declare pem_file="/etc/ca-certificates/extracted/tls-ca-bundle.pem"
if [[ -f "${pem_file}" ]]; then
  [[ ! -d "${install_dir}" ]] || cp -v "${pem_file}" "${install_dir}/cert.pem"
  [[ ! -d "${install_dir}/ssl" ]] || cp -v "${pem_file}" "${install_dir}/ssl/cert.pem"
  [[ ! -d "${openssl_install_dir}" ]] || cp -v "${pem_file}" "${openssl_install_dir}/cert.pem"
  [[ ! -d "${openssl_install_dir}/ssl" ]] || cp -v "${pem_file}" "${openssl_install_dir}/ssl/cert.pem"
fi

# Custom OpenSSL library may be installed. Only prepend to LD_LIBRARY_PATH when
# necessary to avoid conflicting with system binary requirements.
declare openssl_lib_prefix="${LD_LIBRARY_PATH:-}"
if [[ -d "${openssl_install_dir}" ]]; then
  openssl_lib_prefix="${openssl_install_dir}/lib:${openssl_lib_prefix:-}"
fi

# There may be additional certs required by auth tests. Direct OpenSSL to use
# the system cert directory if available.
[[ ! -d /etc/ssl/certs ]] || export SSL_CERT_DIR=/etc/ssl/certs

ulimit -c unlimited || true

if command -v ldd >/dev/null; then
  LD_LIBRARY_PATH="${openssl_lib_prefix}" ldd "${ping}" | grep "libssl" || true
  LD_LIBRARY_PATH="${openssl_lib_prefix}" ldd "${test_gssapi}" | grep "libssl" || true
elif command -v otool >/dev/null; then
  # Try using otool on MacOS if ldd is not available.
  LD_LIBRARY_PATH="${openssl_lib_prefix}" otool -L "${ping}" | grep "libssl" || true
  LD_LIBRARY_PATH="${openssl_lib_prefix}" otool -L "${test_gssapi}" | grep "libssl" || true
fi

maybe_skip() {
  if true; then
    # TODO: Remove if-block when resolving CDRIVER-5995.
    echo "Skipping test until DEVPROD-9029 is resolved."
    return 
  fi

  if $IS_ZSERIES; then
    # TODO: Remove if-block when resolving CDRIVER-5990.
    echo "Skipping test until DEVPROD-16954 is resolved."
    return
  fi
  # Run the test command:
  "$@"
}

if [[ "${ssl}" != "OFF" ]]; then
  # FIXME: CDRIVER-2008 for the cygwin check
  if [[ "${OSTYPE}" != "cygwin" ]]; then
    echo "Authenticating using X.509"
    LD_LIBRARY_PATH="${openssl_lib_prefix}" maybe_skip "${ping}" "mongodb://CN=client,OU=kerneluser,O=10Gen,L=New York City,ST=New York,C=US@${auth_host}/?ssl=true&authMechanism=MONGODB-X509&sslClientCertificateKeyFile=src/libmongoc/tests/x509gen/ldaptest-client-key-and-cert.pem&sslCertificateAuthorityFile=src/libmongoc/tests/x509gen/ldaptest-ca-cert.crt&sslAllowInvalidHostnames=true&${c_timeout}"
  fi
  echo "Connecting to Atlas Free Tier"
  LD_LIBRARY_PATH="${openssl_lib_prefix}" "${ping}" "${atlas_free:?}&${c_timeout}"
  echo "Connecting to Atlas Free Tier with SRV"
  LD_LIBRARY_PATH="${openssl_lib_prefix}" "${ping}" "${atlas_free_srv:?}&${c_timeout}"
  echo "Connecting to Atlas Replica Set"
  LD_LIBRARY_PATH="${openssl_lib_prefix}" "${ping}" "${atlas_replset:?}&${c_timeout}"
  echo "Connecting to Atlas Replica Set with SRV"
  LD_LIBRARY_PATH="${openssl_lib_prefix}" "${ping}" "${atlas_replset_srv:?}${c_timeout}"
  echo "Connecting to Atlas Sharded Cluster"
  LD_LIBRARY_PATH="${openssl_lib_prefix}" "${ping}" "${atlas_shard:?}&${c_timeout}"
  echo "Connecting to Atlas Sharded Cluster with SRV"
  LD_LIBRARY_PATH="${openssl_lib_prefix}" "${ping}" "${atlas_shard_srv:?}${c_timeout}"
  if [[ -z "${require_tls12:-}" ]]; then
    echo "Connecting to Atlas with only TLS 1.1 enabled"
    LD_LIBRARY_PATH="${openssl_lib_prefix}" "${ping}" "${atlas_tls11:?}&${c_timeout}"
    echo "Connecting to Atlas with only TLS 1.1 enabled with SRV"
    LD_LIBRARY_PATH="${openssl_lib_prefix}" "${ping}" "${atlas_tls11_srv:?}${c_timeout}"
  fi
  echo "Connecting to Atlas with only TLS 1.2 enabled"
  LD_LIBRARY_PATH="${openssl_lib_prefix}" "${ping}" "${atlas_tls12:?}&${c_timeout}"
  echo "Connecting to Atlas with only TLS 1.2 enabled with SRV"
  LD_LIBRARY_PATH="${openssl_lib_prefix}" "${ping}" "${atlas_tls12_srv:?}${c_timeout}"
  HAS_CIPHERSUITES_FOR_SERVERLESS="YES"
  if [[ "${OSTYPE}" == "cygwin" ]]; then
    # Windows Server 2008 hosts do not appear to share TLS 1.2 cipher suites with Atlas Serverless.
    WINDOWS_OSNAME="$(systeminfo | grep 'OS Name:' | awk -F ':' '{print $2}')"
    if [[ "${WINDOWS_OSNAME}" == *"Windows Server 2008"* ]]; then
        echo "Detected Windows Server 2008 ... skipping Atlas Serverless test due to no shared cipher suites."
        HAS_CIPHERSUITES_FOR_SERVERLESS="NO"
    fi
  fi
  if [[ "${HAS_CIPHERSUITES_FOR_SERVERLESS}" == "YES" ]]; then
    echo "Connecting to Atlas Serverless with SRV"
    LD_LIBRARY_PATH="${openssl_lib_prefix}" "${ping}" "${atlas_serverless_srv:?}/?${c_timeout}"
    echo "Connecting to Atlas Serverless"
    LD_LIBRARY_PATH="${openssl_lib_prefix}" "${ping}" "${atlas_serverless:?}&${c_timeout}"
  fi

  echo "Connecting to Atlas (cloud-prod) with X509"
  LD_LIBRARY_PATH="${openssl_lib_prefix}" "${ping}" "${atlas_x509:?}&tlsCertificateKeyFile=${atlas_x509_path}&${c_timeout}"

  echo "Connecting to Atlas (cloud-dev) with X509"
  LD_LIBRARY_PATH="${openssl_lib_prefix}" "${ping}" "${atlas_x509_dev:?}&tlsCertificateKeyFile=${atlas_x509_dev_path}&${c_timeout}"

fi

echo "Authenticating using PLAIN"
LD_LIBRARY_PATH="${openssl_lib_prefix}" maybe_skip "${ping}" "mongodb://${auth_plain:?}@${auth_host}/?authMechanism=PLAIN&${c_timeout}"

echo "Authenticating using default auth mechanism"
# Though the auth source is named "mongodb-cr", authentication uses the default mechanism (currently SCRAM-SHA-1).
LD_LIBRARY_PATH="${openssl_lib_prefix}" maybe_skip "${ping}" "mongodb://${auth_mongodbcr:?}@${auth_host}/mongodb-cr?${c_timeout}"

if [[ "${sasl}" != "OFF" ]]; then
  echo "Authenticating using GSSAPI"
  LD_LIBRARY_PATH="${openssl_lib_prefix}" maybe_skip "${ping}" "mongodb://${auth_gssapi:?}@${auth_host}/?authMechanism=GSSAPI&${c_timeout}"

  echo "Authenticating with CANONICALIZE_HOST_NAME"
  LD_LIBRARY_PATH="${openssl_lib_prefix}" maybe_skip "${ping}" "mongodb://${auth_gssapi:?}@${ip_addr}/?authMechanism=GSSAPI&authMechanismProperties=CANONICALIZE_HOST_NAME:true&${c_timeout}"

  declare ld_preload="${LD_PRELOAD:-}"
  if [[ "${ASAN:-}" == "on" ]]; then
    ld_preload="$(bypass_dlclose):${ld_preload}"
  fi

  echo "Test threaded GSSAPI auth"
  LD_LIBRARY_PATH="${openssl_lib_prefix}" MONGOC_TEST_GSSAPI_HOST="${auth_host}" MONGOC_TEST_GSSAPI_USER="${auth_gssapi}" LD_PRELOAD="${ld_preload:-}" maybe_skip "${test_gssapi}"
  echo "Threaded GSSAPI auth OK"

  if [[ "${OSTYPE}" == "cygwin" ]]; then
    echo "Authenticating using GSSAPI (service realm: LDAPTEST.10GEN.CC)"
    LD_LIBRARY_PATH="${openssl_lib_prefix}" maybe_skip "${ping}" "mongodb://${auth_crossrealm:?}@${auth_host}/?authMechanism=GSSAPI&authMechanismProperties=SERVICE_REALM:LDAPTEST.10GEN.CC&${c_timeout}"
    echo "Authenticating using GSSAPI (UTF-8 credentials)"
    LD_LIBRARY_PATH="${openssl_lib_prefix}" maybe_skip "${ping}" "mongodb://${auth_gssapi_utf8:?}@${auth_host}/?authMechanism=GSSAPI&${c_timeout}"
  fi
fi
