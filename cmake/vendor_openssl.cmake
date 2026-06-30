# OpenSSL isn't a CMake project; build it via ExternalProject.

include(ExternalProject)
include(ProcessorCount)

set(REGOCPP_VENDORED_OPENSSL_VERSION "3.5.7")
set(REGOCPP_VENDORED_OPENSSL_SHA256 "a8c0d28a529ca480f9f36cf5792e2cd21984552a3c8e4aa11a24aa31aeac98e8")

set(_ossl_prefix "${CMAKE_BINARY_DIR}/vendored-openssl")
set(_ossl_install "${_ossl_prefix}/install")
set(_ossl_libdir "${_ossl_install}/lib")
set(_ossl_incdir "${_ossl_install}/include")

if(APPLE)
  if(CMAKE_OSX_ARCHITECTURES)
    set(_ossl_arch "${CMAKE_OSX_ARCHITECTURES}")
  else()
    set(_ossl_arch "${CMAKE_SYSTEM_PROCESSOR}")
  endif()
  if(_ossl_arch MATCHES "arm64|aarch64")
    set(_ossl_target "darwin64-arm64-cc")
  else()
    set(_ossl_target "darwin64-x86_64-cc")
  endif()
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
  set(_ossl_target "linux-aarch64")
else()
  set(_ossl_target "linux-x86_64")
endif()

set(_ossl_configure_flags
  "${_ossl_target}"
  no-shared no-tests no-apps no-docs
  "--prefix=${_ossl_install}"
  "--openssldir=${_ossl_install}/ssl"
  "--libdir=lib"
)

# On Apple, pin crypto to the deployment target for a lower min OS than host.
if(APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET)
  list(APPEND _ossl_configure_flags "-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
endif()

ProcessorCount(_ossl_jobs)
if(_ossl_jobs EQUAL 0)
  set(_ossl_jobs 1)
endif()

set(_ossl_libssl "${_ossl_libdir}/libssl.a")
set(_ossl_libcrypto "${_ossl_libdir}/libcrypto.a")

ExternalProject_Add(regocpp_vendored_openssl
  URL "https://github.com/openssl/openssl/releases/download/openssl-${REGOCPP_VENDORED_OPENSSL_VERSION}/openssl-${REGOCPP_VENDORED_OPENSSL_VERSION}.tar.gz"
  URL_HASH "SHA256=${REGOCPP_VENDORED_OPENSSL_SHA256}"
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  PREFIX "${_ossl_prefix}"
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND <SOURCE_DIR>/Configure ${_ossl_configure_flags}
  BUILD_COMMAND make -j${_ossl_jobs}
  INSTALL_COMMAND make install_sw
  BUILD_BYPRODUCTS "${_ossl_libssl}" "${_ossl_libcrypto}"
)

# Imported targets require their include directory to exist at configure time.
file(MAKE_DIRECTORY "${_ossl_incdir}")

add_library(OpenSSL::Crypto STATIC IMPORTED GLOBAL)
set_target_properties(OpenSSL::Crypto PROPERTIES
  IMPORTED_LOCATION "${_ossl_libcrypto}"
  INTERFACE_INCLUDE_DIRECTORIES "${_ossl_incdir}"
)

add_library(OpenSSL::SSL STATIC IMPORTED GLOBAL)
set_target_properties(OpenSSL::SSL PROPERTIES
  IMPORTED_LOCATION "${_ossl_libssl}"
  INTERFACE_INCLUDE_DIRECTORIES "${_ossl_incdir}"
  INTERFACE_LINK_LIBRARIES "OpenSSL::Crypto"
)

# Static libcrypto pulls in dl and threads on most platforms.
set(_ossl_sys_deps "${CMAKE_DL_LIBS}")
if(Threads_FOUND)
  list(APPEND _ossl_sys_deps Threads::Threads)
endif()
if(_ossl_sys_deps)
  set_property(TARGET OpenSSL::Crypto APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${_ossl_sys_deps}")
endif()
