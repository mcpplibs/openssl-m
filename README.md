# openssl-m

> OpenSSL 3.5 LTS, libcrypto and libssl, through C++23 modules.

**English** · [简体中文](README.zh.md) · [Upstream](https://github.com/openssl/openssl) · [Releases](https://github.com/mcpplibs/openssl-m/releases) · [mcpp](https://github.com/mcpp-community/mcpp) · [Index](https://github.com/mcpplibs/mcpp-index)

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)
[![mcpp-community](https://img.shields.io/badge/mcpp--community-mwrap-blue)](https://github.com/mcpp-community)
[![CI](https://github.com/mcpplibs/openssl-m/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/mcpplibs/openssl-m/actions/workflows/ci.yml)

The upstream C API is unchanged: `SSL_CTX`, `EVP_DigestInit_ex`, `X509_verify_cert` are the
upstream globals of `<openssl/...>`. The library is `compat.openssl` 3.5.9, built by OpenSSL's
own Configure from the release (with assembly on Linux and macOS; `no-asm` on Windows). The
module surface is derived from the configured headers by the compiler: every declaration,
the functions the headers define `static ossl_inline` declared again, and every macro either
mapped or listed with its reason.

## Modules

- `import mcpplibs.mwrap.openssl;`: everything below
- `import mcpplibs.mwrap.openssl.crypto;`: libcrypto's headers — EVP, BIO, ERR, RAND, X509, PEM, BN, EC, KDF/MAC, OSSL_PARAM, providers
- `import mcpplibs.mwrap.openssl.ssl;`: libssl's headers — `SSL_CTX`, `SSL`, TLS, DTLS, QUIC
- `import mcpplibs.mwrap.openssl.macros;`: `SSL_OP_*`, `TLS1_3_VERSION`, `EVP_MAX_MD_SIZE`, `SSL_CTX_set_min_proto_version`, `BIO_get_mem_data` … in `mcpplibs::mwrap::openssl::macros`

> More Modules API -> [Usage and API](docs/usage.md)

## Getting started

```sh
mcpp new hashes --template mcpplibs.mwrap.openssl@3.5.9
cd hashes
mcpp run
```

The default template checks SHA-256 and HMAC-SHA-256 against published vectors and seals a
message with AES-256-GCM. `--template mcpplibs.mwrap.openssl@3.5.9:tls-loopback` makes a key
and a self-signed certificate at run time and runs a TLS 1.3 client and server over a pair of
memory BIOs — no network, no files.

```toml
[dependencies]
mcpplibs.mwrap.openssl = "3.5.9"
```

```cpp
import std;
import mcpplibs.mwrap.openssl;
namespace m = mcpplibs::mwrap::openssl::macros;

std::string sha256(std::string_view text) {
    unsigned char digest[m::EVP_MAX_MD_SIZE];
    unsigned int size = 0;
    EVP_Digest(text.data(), text.size(), digest, &size, EVP_sha256(), nullptr);
    std::string hex;
    for (unsigned i = 0; i < size; ++i) hex += std::format("{:02x}", digest[i]);
    return hex;
}
```

## Macros and inline functions

A module cannot export a macro. The constants (11632) are `inline constexpr` of their
expansion's type; the macros that expand to one call of a function (510, such as
`SSL_CTX_set_min_proto_version`, `SSL_set_tlsext_host_name`, `BIO_get_mem_data`) are functions
of the same name with that function's parameter types. The functions the headers define
`static ossl_inline` (`ERR_GET_LIB`, `ERR_GET_REASON`, `OPENSSL_load_u32_le`, `lh_*` …, 334)
are declared again in `mcpplibs::mwrap::openssl`. The rest stay macros in
`<mwrap/openssl/macros.hpp>`: the type-generic `sk_TYPE_*` / `lh_*` / `STACK_OF`, the OID arc
lists `OBJ_*`, `OPENSSL_malloc` / `OPENSSL_free` and the others that record `__FILE__`, and the
`OPENSSL_NO_*` switches. Include that header before the imports
([every macro, by class](docs/macros.md)).

## Platforms

Linux (x86_64, aarch64), macOS (arm64) and Windows (x86_64, with `NOMINMAX` and
`WIN32_LEAN_AND_MEAN`). compat.openssl is built at install time for the host by OpenSSL's
Configure, so there is no cross target: no wasm32, no musl graph.

## Build and testing

CI on every platform and compiler (GCC 16, LLVM 23) builds the modules and runs the tests:
digests, MACs and AES-GCM against published vectors, the error queue, a self-signed
certificate verified, TLS 1.2 and 1.3 handshakes over memory BIOs, every constant against its
macro, and header mixing. The export audit ([tools/check_exports.py](tools/check_exports.py))
checks that the generated sources are fresh and, on the Linux LLVM row, derives the record
again from the headers. A separate job scaffolds both templates as an installed package and
runs them.

## License

[Apache-2.0](LICENSE) for the wrapper, by mcpp-community ([NOTICE](NOTICE)). OpenSSL remains
under the Apache License 2.0 ([upstream/openssl/LICENSE.txt](upstream/openssl/LICENSE.txt),
[README](upstream/openssl/README.md)).
