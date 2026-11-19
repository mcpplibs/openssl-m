// The headers and the import in one translation unit (the side header first;
// GCC needs every include before the import): the module's names are the very
// same entities as the headers'; the functions declared again for the headers'
// `static ossl_inline` ones, and the call macros' mappings, give the results of
// the originals. While a macro is defined it expands even inside a qualified
// name, so a mapping of the same name is reached with the macro set aside.
// (tests/constants_test.cpp compares every constant.)
#if defined(_WIN32)
// <wincrypt.h> defines X509_NAME and others as macros; OpenSSL's types.h sets them aside.
#  define NOMINMAX
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <wincrypt.h>
#endif
#include <mwrap/openssl/macros.hpp>
#include <cstring>
#include <string_view>
#include <type_traits>
#include <gtest/gtest.h>

import mcpplibs.mwrap.openssl;

namespace o = mcpplibs::mwrap::openssl;

TEST(HeaderMix, TheSameEntities) {
    static_assert(std::is_same_v<decltype(&::SSL_new), SSL* (*)(SSL_CTX*)>);
    EVP_MD_CTX* context = ::EVP_MD_CTX_new();
    ASSERT_NE(context, nullptr);
    EXPECT_EQ(EVP_DigestInit_ex(context, ::EVP_sha256(), nullptr), 1);
    EVP_MD_CTX_free(context);
    X509_NAME* name = X509_NAME_new();  // the OpenSSL type, also with <wincrypt.h> on Windows
    EXPECT_EQ(X509_NAME_entry_count(name), 0);
    X509_NAME_free(name);
}

TEST(HeaderMix, InlineFunctionsDeclaredAgain) {
    for (unsigned long code : {ERR_PACK(ERR_LIB_SSL, 0, SSL_R_BAD_LENGTH), ERR_PACK(ERR_LIB_EVP, 0, EVP_R_BAD_DECRYPT), 0x8000002UL,
                               static_cast<unsigned long>(ERR_SYSTEM_FLAG | 2)}) {
        EXPECT_EQ(o::ERR_GET_LIB(code), ::ERR_GET_LIB(code)) << code;
        EXPECT_EQ(o::ERR_GET_REASON(code), ::ERR_GET_REASON(code)) << code;
        EXPECT_EQ(o::ERR_GET_RFLAGS(code), ::ERR_GET_RFLAGS(code)) << code;
        EXPECT_EQ(o::ERR_FATAL_ERROR(code), ::ERR_FATAL_ERROR(code)) << code;
        EXPECT_EQ(o::ERR_COMMON_ERROR(code), ::ERR_COMMON_ERROR(code)) << code;
    }
    unsigned char a[8], b[8];
    o::OPENSSL_store_u64_le(a, 0x0102030405060708ULL);
    ::OPENSSL_store_u64_le(b, 0x0102030405060708ULL);
    EXPECT_EQ(std::memcmp(a, b, 8), 0);
}

namespace {
long by_macro_get_mem_data(BIO* bio, char** data) { return BIO_get_mem_data(bio, data); }
int by_macro_pending(BIO* bio) { return BIO_pending(bio); }
}  // namespace

#pragma push_macro("BIO_get_mem_data")
#pragma push_macro("BIO_pending")
#undef BIO_get_mem_data
#undef BIO_pending
TEST(HeaderMix, CallMacrosGiveTheMacrosResults) {
    BIO* bio = BIO_new(BIO_s_mem());
    ASSERT_EQ(BIO_write(bio, "abc", 3), 3);
    char *by_macro = nullptr, *by_mapping = nullptr;
    EXPECT_EQ(o::macros::BIO_get_mem_data(bio, &by_mapping), by_macro_get_mem_data(bio, &by_macro));
    EXPECT_EQ(by_macro, by_mapping);
    EXPECT_EQ(o::macros::BIO_pending(bio), by_macro_pending(bio));
    BIO_free(bio);
}
#pragma pop_macro("BIO_pending")
#pragma pop_macro("BIO_get_mem_data")

// The side header's purpose: the configuration switches and the textual macros.
#ifndef OPENSSL_NO_ENGINE
#error "compat.openssl is configured no-engine; the switch is a macro here"
#endif
TEST(HeaderMix, TextualMacros) {
    STACK_OF(X509)* chain = sk_X509_new_null();
    ASSERT_NE(chain, nullptr);
    EXPECT_EQ(sk_X509_num(chain), 0);
    sk_X509_free(chain);
    void* memory = OPENSSL_malloc(16);
    ASSERT_NE(memory, nullptr);
    OPENSSL_free(memory);
}
