// Included first by every module unit of openssl-m, in the global module
// fragment, before any OpenSSL header.
#pragma once

// On Windows the headers declare part of the API only when <windows.h> is
// already in (async.h: OSSL_ASYNC_FD is a HANDLE, and ssl.h's SSL_*async*
// functions take it), so it comes first, lean and without min/max macros that
// would break later standard-library code. <wincrypt.h> stays out; OpenSSL's
// types.h sets aside the names it shares with it when it is in. Consumers
// that include OpenSSL's headers themselves should do the same
// (<mwrap/openssl/macros.hpp> does).
#if defined(_WIN32)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#endif

// Spells any type as a single type-id, so a generated declaration can name a
// parameter of function-pointer or array type as `mwrap_openssl_t<T> name`.
template <class T> using mwrap_openssl_t = T;
