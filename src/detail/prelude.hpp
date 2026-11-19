// Included first by every module unit of openssl-m, in the global module
// fragment, before any OpenSSL header.
#pragma once

// On Windows <openssl/e_ostime.h> reaches <winsock2.h> and so <windows.h>,
// whose min/max function-like macros would break later standard-library
// code; the lean set also keeps <wincrypt.h>'s names out (OpenSSL's types.h
// undefines the ones that collide). Consumers that include OpenSSL headers
// themselves should define the same two macros.
#if defined(_WIN32)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#endif

// Spells any type as a single type-id, so a generated declaration can name a
// parameter of function-pointer or array type as `mwrap_openssl_t<T> name`.
template <class T> using mwrap_openssl_t = T;
