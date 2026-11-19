# Using mcpplibs.mwrap.openssl

`import mcpplibs.mwrap.openssl;` declares OpenSSL 3.5.9's API as its public headers do. Types,
struct tags, enumerators and functions are the upstream globals, the very same entities as the
headers', so a translation unit may include the headers too. The library is `compat.openssl`
3.5.9, built from the release by OpenSSL's own Configure (`no-shared no-dso no-tests no-apps
no-engine`; with assembly on Linux and macOS, `no-asm` VC-WIN64A on Windows).

## Modules

| Module | What it holds |
|---|---|
| `mcpplibs.mwrap.openssl` | re-exports the three below |
| `mcpplibs.mwrap.openssl.crypto` | the declarations of libcrypto's headers: 1091 types, 447 tags, 32 enumerators, 5731 functions; and 334 functions declared again (below) |
| `mcpplibs.mwrap.openssl.ssl` | the declarations of libssl's headers (`ssl.h`, `ssl2.h`, `ssl3.h`, `tls1.h`, `dtls1.h`, `srtp.h`, `sslerr.h`, `quic.h`, `prov_ssl.h`): 47 types, 14 tags, 56 enumerators, 602 functions |
| `mcpplibs.mwrap.openssl.macros` | the macro mappings, in `mcpplibs::mwrap::openssl::macros` |

Deprecated functions (`OSSL_DEPRECATEDIN_3_0` …) are exported as the headers declare them and
warn where they are used, as with the headers.

## Inline functions

The headers define some public functions `static ossl_inline`: `ERR_GET_LIB`, `ERR_GET_REASON`,
`ERR_GET_RFLAGS`, `ERR_FATAL_ERROR`, `ERR_COMMON_ERROR`, the byte-order helpers
`OPENSSL_load_u16_le` … `OPENSSL_store_u64_be`, the typed `lh_TYPE_*` functions and others. A
function of internal linkage cannot be exported, so each is declared again in
`mcpplibs::mwrap::openssl` with the same name and signature, forwarding to the header's (334).
The `ossl_check_*` helpers of the type-generic stack macros are not API and are not declared.

```cpp
namespace o = mcpplibs::mwrap::openssl;
if (o::ERR_GET_LIB(ERR_peek_last_error()) == o::macros::ERR_LIB_SSL) { /* … */ }
```

## Macros

A named module cannot export a macro. `tools/surface.py` puts every macro the headers define in
one class ([the complete list](macros.md)):

- **Constants** (11632) are `inline constexpr` of the expansion's type: `SSL_OP_*`,
  `SSL_ERROR_*`, `SSL_VERIFY_*`, `TLS1_2_VERSION`, `EVP_MAX_MD_SIZE`, `EVP_CTRL_*`, `NID_*`,
  `SN_*` / `LN_*` (strings), `OSSL_PKEY_PARAM_*`, `ERR_LIB_*`, the reason codes,
  `OPENSSL_VERSION_NUMBER`, `OPENSSL_VERSION_STR` …
- **Calls** (510): a macro whose expansion is one call of a function, each parameter passed
  whole, is a function of the same name taking that function's parameter types — `const void *`
  where the macro casts a pointer argument — and returning its result: `SSL_CTX_set_mode`,
  `SSL_CTX_set_min_proto_version`, `SSL_set_tlsext_host_name`, `BIO_get_mem_data`,
  `BIO_pending`, `EVP_MD_size` (an alias of `EVP_MD_get_size`) … Unlike the macro, it evaluates
  each argument once.
- **Textual** (4457) stay macros, from `<mwrap/openssl/macros.hpp>`: the type-generic stack and
  lhash macros (`STACK_OF`, `sk_TYPE_*`, `lh_TYPE_*`, `DEFINE_STACK_OF`), the OID arc lists
  (`OBJ_*`, comma-separated), `OPENSSL_malloc` / `OPENSSL_free` / `OPENSSL_strdup` and the others
  that pass their caller's `__FILE__` and `__LINE__`, `ERR_raise` / `ERR_raise_data`, the
  configuration switches (`OPENSSL_NO_*`) for `#if`, header guards and annotations.

`<mwrap/openssl/macros.hpp>` is every public header. Include it before the imports (GCC needs
every `#include` before the first `import`). While a macro is defined it expands even inside a
qualified name, so with the header included, `mcpplibs::mwrap::openssl::macros::SSL_OP_ALL`
expands too: use the macro there, or set it aside with `#pragma push_macro` / `#undef`.

```cpp
#include <mwrap/openssl/macros.hpp>
import std;
import mcpplibs.mwrap.openssl;

void each_certificate(STACK_OF(X509)* chain) {
    for (int i = 0; i < sk_X509_num(chain); ++i) X509_print_fp(stdout, sk_X509_value(chain, i));
}
```

## Windows

On Windows the headers declare the async API (`OSSL_ASYNC_FD`, `SSL_waiting_for_async`,
`SSL_get_all_async_fds` …) only when `<windows.h>` is already included, so the modules include it
first, with `NOMINMAX` and `WIN32_LEAN_AND_MEAN`; `<mwrap/openssl/macros.hpp>` does the same. A
consumer that includes OpenSSL's headers itself should define both macros too.
`OSSL_BAD_ASYNC_FD` (Windows' `INVALID_HANDLE_VALUE`, a pointer cast) is no constant expression
there and stays a macro; so does `CRYPTO_ONCE_STATIC_INIT` on macOS (a braced initializer). `<wincrypt.h>`'s `X509_NAME` and the other macros it shares with OpenSSL are set
aside by OpenSSL's `types.h`, so including `<wincrypt.h>` first works (tested).

## Not supported

compat.openssl is built at install time for the host, by OpenSSL's Configure and make (nmake on
Windows), outside mcpp's compile rules, against the host's C library. There is therefore no cross
target — no wasm32, no musl/openkal graph, no cross architecture — until OpenSSL is built inside
the build graph. On Linux aarch64, mcpp's `gcc@16.1.0` is a musl toolchain
(`aarch64-linux-musl-g++`), so use LLVM there (`llvm@23.1.3`, glibc), as CI does.

## Regenerating

```sh
mcpp build                                    # installs compat.openssl and its headers
python3 tools/surface.py --clang <clang++> --write
python3 tools/check_exports.py --clang <clang++>
```
