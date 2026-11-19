// TLS through the module alone: a key and a self-signed certificate made at run
// time, verified; a client and a server over a pair of memory BIOs complete a
// TLS 1.3 handshake, and a TLS 1.2 one when the client caps the version, with
// SNI and host-name verification; data goes both ways. No network, no files.
#include <gtest/gtest.h>

import std;
import mcpplibs.mwrap.openssl;

namespace m = mcpplibs::mwrap::openssl::macros;

namespace {
struct Identity {
    EVP_PKEY* key = nullptr;
    X509* certificate = nullptr;
    ~Identity() { X509_free(certificate); EVP_PKEY_free(key); }
};

void make_identity(Identity& id) {
    id.key = EVP_PKEY_Q_keygen(nullptr, nullptr, "EC", "P-256");
    ASSERT_NE(id.key, nullptr);
    id.certificate = X509_new();
    ASSERT_EQ(X509_set_version(id.certificate, m::X509_VERSION_3), 1);
    ASSERT_EQ(ASN1_INTEGER_set(X509_get_serialNumber(id.certificate), 1), 1);
    ASSERT_NE(X509_gmtime_adj(X509_getm_notBefore(id.certificate), 0), nullptr);
    ASSERT_NE(X509_gmtime_adj(X509_getm_notAfter(id.certificate), 3600), nullptr);
    ASSERT_EQ(X509_set_pubkey(id.certificate, id.key), 1);
    X509_NAME* name = X509_get_subject_name(id.certificate);
    ASSERT_EQ(X509_NAME_add_entry_by_txt(name, "CN", m::MBSTRING_ASC, reinterpret_cast<const unsigned char*>("localhost"), -1, -1, 0), 1);
    ASSERT_EQ(X509_set_issuer_name(id.certificate, name), 1);
    ASSERT_GT(X509_sign(id.certificate, id.key, EVP_sha256()), 0);
}

// Drive both ends until each has finished, as an application with non-blocking I/O does.
bool handshake(SSL* client, SSL* server) {
    bool client_done = false, server_done = false;
    for (int round = 0; round < 100 && !(client_done && server_done); ++round) {
        for (auto [ssl, done] : {std::pair{client, &client_done}, std::pair{server, &server_done}}) {
            if (*done) continue;
            const int result = SSL_do_handshake(ssl);
            if (result == 1) { *done = true; continue; }
            const int error = SSL_get_error(ssl, result);
            if (error != m::SSL_ERROR_WANT_READ && error != m::SSL_ERROR_WANT_WRITE) return false;
        }
    }
    return client_done && server_done;
}

void session(const Identity& id, long max_version, int expected_version) {
    SSL_CTX* server_context = SSL_CTX_new(TLS_server_method());
    SSL_CTX* client_context = SSL_CTX_new(TLS_client_method());
    ASSERT_EQ(SSL_CTX_use_certificate(server_context, id.certificate), 1);
    ASSERT_EQ(SSL_CTX_use_PrivateKey(server_context, id.key), 1);
    ASSERT_EQ(X509_STORE_add_cert(SSL_CTX_get_cert_store(client_context), id.certificate), 1);
    SSL_CTX_set_verify(client_context, m::SSL_VERIFY_PEER, nullptr);
    ASSERT_EQ(m::SSL_CTX_set_min_proto_version(client_context, m::TLS1_2_VERSION), 1);
    ASSERT_EQ(m::SSL_CTX_set_max_proto_version(client_context, max_version), 1);
    EXPECT_EQ(m::SSL_CTX_get_max_proto_version(client_context), max_version);

    SSL* server = SSL_new(server_context);
    SSL* client = SSL_new(client_context);
    BIO *server_io = nullptr, *client_io = nullptr;
    ASSERT_EQ(BIO_new_bio_pair(&server_io, 0, &client_io, 0), 1);
    SSL_set_bio(server, server_io, server_io);
    SSL_set_bio(client, client_io, client_io);
    SSL_set_accept_state(server);
    SSL_set_connect_state(client);
    ASSERT_EQ(m::SSL_set_tlsext_host_name(client, "localhost"), 1);
    ASSERT_EQ(SSL_set1_host(client, "localhost"), 1);

    ASSERT_TRUE(handshake(client, server));
    EXPECT_EQ(SSL_version(client), expected_version);
    EXPECT_EQ(SSL_get_verify_result(client), m::X509_V_OK);
    EXPECT_EQ(std::string_view(SSL_get_servername(server, m::TLSEXT_NAMETYPE_host_name)), "localhost");

    char buffer[64] = {};
    ASSERT_EQ(SSL_write(client, "ping", 4), 4);
    ASSERT_EQ(SSL_read(server, buffer, sizeof buffer), 4);
    EXPECT_EQ(std::string_view(buffer, 4), "ping");
    ASSERT_EQ(SSL_write(server, "pong", 4), 4);
    ASSERT_EQ(SSL_read(client, buffer, sizeof buffer), 4);
    EXPECT_EQ(std::string_view(buffer, 4), "pong");

    SSL_free(client);
    SSL_free(server);
    SSL_CTX_free(client_context);
    SSL_CTX_free(server_context);
}
}  // namespace

TEST(Tls, SelfSignedCertificateVerifies) {
    Identity id;
    make_identity(id);
    X509_STORE* store = X509_STORE_new();
    ASSERT_EQ(X509_STORE_add_cert(store, id.certificate), 1);
    X509_STORE_CTX* context = X509_STORE_CTX_new();
    ASSERT_EQ(X509_STORE_CTX_init(context, store, id.certificate, nullptr), 1);
    EXPECT_EQ(X509_verify_cert(context), 1);
    X509_STORE_CTX_free(context);
    X509_STORE_free(store);
    EXPECT_EQ(X509_check_host(id.certificate, "localhost", 0, 0, nullptr), 1);
    EXPECT_NE(X509_check_host(id.certificate, "example.org", 0, 0, nullptr), 1);
}

TEST(Tls, Tls13OverMemoryBios) {
    Identity id;
    make_identity(id);
    session(id, m::TLS1_3_VERSION, m::TLS1_3_VERSION);
}

TEST(Tls, Tls12WhenTheClientCapsIt) {
    Identity id;
    make_identity(id);
    session(id, m::TLS1_2_VERSION, m::TLS1_2_VERSION);
}
