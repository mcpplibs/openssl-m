// The API through the module alone: no OpenSSL header in this translation
// unit. Digests, MACs and an AEAD against published vectors, the error queue,
// and the functions the headers define `static ossl_inline` (declared again in
// mcpplibs::mwrap::openssl).
#include <gtest/gtest.h>

import std;
import mcpplibs.mwrap.openssl;

namespace m = mcpplibs::mwrap::openssl::macros;
namespace o = mcpplibs::mwrap::openssl;

namespace {
std::string hex(const unsigned char* data, std::size_t size) {
    std::string out;
    for (std::size_t i = 0; i < size; ++i) out += std::format("{:02x}", data[i]);
    return out;
}
const unsigned char* bytes(std::string_view text) { return reinterpret_cast<const unsigned char*>(text.data()); }
}  // namespace

TEST(Api, Version) {
    EXPECT_EQ(OpenSSL_version_num(), m::OPENSSL_VERSION_NUMBER);
    EXPECT_EQ(m::OPENSSL_VERSION_MAJOR, 3);
    EXPECT_EQ(m::OPENSSL_VERSION_MINOR, 5);
    EXPECT_EQ(m::OPENSSL_VERSION_PATCH, 9);
    EXPECT_EQ(std::string_view(m::OPENSSL_VERSION_STR), "3.5.9");
    EXPECT_TRUE(std::string_view(OpenSSL_version(m::OPENSSL_VERSION)).starts_with("OpenSSL 3.5.9"));
}

// FIPS 180-2, Appendix B.1: SHA-256("abc").
TEST(Api, Sha256) {
    unsigned char digest[m::EVP_MAX_MD_SIZE];
    unsigned int size = 0;
    ASSERT_EQ(EVP_Digest(bytes("abc"), 3, digest, &size, EVP_sha256(), nullptr), 1);
    EXPECT_EQ(hex(digest, size), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

    EVP_MD_CTX* context = EVP_MD_CTX_new();
    ASSERT_NE(context, nullptr);
    ASSERT_EQ(EVP_DigestInit_ex(context, EVP_sha256(), nullptr), 1);
    ASSERT_EQ(EVP_DigestUpdate(context, "a", 1), 1);
    ASSERT_EQ(EVP_DigestUpdate(context, "bc", 2), 1);
    ASSERT_EQ(EVP_DigestFinal_ex(context, digest, &size), 1);
    EVP_MD_CTX_free(context);
    EXPECT_EQ(hex(digest, size), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    // A call macro: EVP_MD_size is EVP_MD_get_size.
    EXPECT_EQ(m::EVP_MD_size(EVP_sha256()), 32);
}

// RFC 4231, test case 2: HMAC-SHA-256 with the key "Jefe".
TEST(Api, Hmac) {
    unsigned char out[m::EVP_MAX_MD_SIZE];
    std::size_t size = 0;
    const std::string_view key = "Jefe", data = "what do ya want for nothing?";
    ASSERT_NE(EVP_Q_mac(nullptr, "HMAC", nullptr, "SHA256", nullptr, key.data(), key.size(), bytes(data), data.size(), out, sizeof out, &size),
              nullptr);
    EXPECT_EQ(hex(out, size), "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
}

// The GCM specification (McGrew & Viega), test case 14: AES-256, zero key, zero IV, one zero block.
TEST(Api, Aes256Gcm) {
    const unsigned char key[32] = {}, iv[12] = {}, plain[16] = {};
    unsigned char cipher[16], tag[16], back[16];
    int size = 0, final_size = 0;
    EVP_CIPHER_CTX* context = EVP_CIPHER_CTX_new();
    ASSERT_EQ(EVP_EncryptInit_ex(context, EVP_aes_256_gcm(), nullptr, key, iv), 1);
    ASSERT_EQ(EVP_EncryptUpdate(context, cipher, &size, plain, sizeof plain), 1);
    ASSERT_EQ(EVP_EncryptFinal_ex(context, cipher + size, &final_size), 1);
    ASSERT_EQ(EVP_CIPHER_CTX_ctrl(context, m::EVP_CTRL_GCM_GET_TAG, sizeof tag, tag), 1);
    EXPECT_EQ(hex(cipher, sizeof cipher), "cea7403d4d606b6e074ec5d3baf39d18");
    EXPECT_EQ(hex(tag, sizeof tag), "d0d1c8a799996bf0265b98b5d48ab919");

    ASSERT_EQ(EVP_DecryptInit_ex(context, EVP_aes_256_gcm(), nullptr, key, iv), 1);
    ASSERT_EQ(EVP_DecryptUpdate(context, back, &size, cipher, sizeof cipher), 1);
    ASSERT_EQ(EVP_CIPHER_CTX_ctrl(context, m::EVP_CTRL_GCM_SET_TAG, sizeof tag, tag), 1);
    EXPECT_EQ(EVP_DecryptFinal_ex(context, back + size, &final_size), 1);
    EXPECT_EQ(std::memcmp(back, plain, sizeof plain), 0);
    // A tampered tag is refused.
    tag[0] ^= 1;
    ASSERT_EQ(EVP_DecryptInit_ex(context, nullptr, nullptr, key, iv), 1);
    ASSERT_EQ(EVP_DecryptUpdate(context, back, &size, cipher, sizeof cipher), 1);
    ASSERT_EQ(EVP_CIPHER_CTX_ctrl(context, m::EVP_CTRL_GCM_SET_TAG, sizeof tag, tag), 1);
    EXPECT_NE(EVP_DecryptFinal_ex(context, back + size, &final_size), 1);
    EVP_CIPHER_CTX_free(context);
}

TEST(Api, RandomBytes) {
    unsigned char a[32] = {}, b[32] = {};
    ASSERT_EQ(RAND_bytes(a, sizeof a), 1);
    ASSERT_EQ(RAND_bytes(b, sizeof b), 1);
    EXPECT_NE(std::memcmp(a, b, sizeof a), 0);
}

// The error queue, read with the functions upstream declares `static ossl_inline` (ERR_GET_LIB …), declared
// again in mcpplibs::mwrap::openssl.
TEST(Api, ErrorQueue) {
    ERR_clear_error();
    EXPECT_EQ(BIO_new_file("/this/file/does/not/exist", "rb"), nullptr);
    const unsigned long code = ERR_peek_last_error();
    ASSERT_NE(code, 0u);
    EXPECT_EQ(o::ERR_GET_LIB(code), m::ERR_LIB_BIO);
    EXPECT_EQ(o::ERR_GET_REASON(code), m::BIO_R_NO_SUCH_FILE);
    EXPECT_FALSE(o::ERR_FATAL_ERROR(code));
    char text[256];
    ERR_error_string_n(code, text, sizeof text);
    EXPECT_NE(std::string_view(text).find("no such file"), std::string_view::npos) << text;
    ERR_clear_error();
    EXPECT_EQ(ERR_peek_error(), 0u);
}

TEST(Api, ByteOrder) {
    unsigned char buffer[8] = {};
    o::OPENSSL_store_u32_be(buffer, 0x01020304u);
    EXPECT_EQ(hex(buffer, 4), "01020304");
    std::uint32_t value = 0;
    o::OPENSSL_load_u32_le(&value, buffer);
    EXPECT_EQ(value, 0x04030201u);
}

// A memory BIO through the call macros: BIO_get_mem_data is BIO_ctrl(BIO_CTRL_INFO).
TEST(Api, MemoryBio) {
    BIO* bio = BIO_new(BIO_s_mem());
    ASSERT_EQ(BIO_write(bio, "hello", 5), 5);
    char* data = nullptr;
    const long size = m::BIO_get_mem_data(bio, &data);
    EXPECT_EQ(std::string_view(data, size), "hello");
    EXPECT_EQ(m::BIO_pending(bio), 5);
    EXPECT_EQ(BIO_free(bio), 1);
}
