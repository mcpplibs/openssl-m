// mcpplibs.mwrap.openssl — OpenSSL 3.5 (libcrypto and libssl) as C++ modules.
// Every exported declaration is the upstream global of <openssl/...>, unchanged;
// the root re-exports the three modules below. The library is compat.openssl,
// built from the same release.
export module mcpplibs.mwrap.openssl;

export import mcpplibs.mwrap.openssl.crypto;
export import mcpplibs.mwrap.openssl.ssl;
export import mcpplibs.mwrap.openssl.macros;
