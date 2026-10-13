/*
*  This file is part of aasdk library project.
*
*  aasdk is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.

*  aasdk is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with aasdk. If not, see <http://www.gnu.org/licenses/>.
*/

// Cryptor::decrypt against a real TLS server in memory, as the phone would be (average-web-dev/aasdk#6):
// several records in one frame, TLS 1.3 and ciphers whose record overhead is not 29 bytes.

#include <gtest/gtest.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <aasdk/Messenger/Cryptor.hpp>
#include <aasdk/Transport/SSLWrapper.hpp>
#include <aasdk/Error/Error.hpp>


namespace aasdk
{
namespace messenger
{
namespace ut
{

namespace
{

// RSA because the static RSA suite (AES128-SHA) needs it; generated once, it takes a moment
EVP_PKEY* serverKey()
{
    static EVP_PKEY* key = EVP_RSA_gen(2048);
    return key;
}

X509* serverCertificate()
{
    static X509* certificate = []() {
        X509* cert = X509_new();
        X509_set_version(cert, 2);
        ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);
        X509_gmtime_adj(X509_getm_notBefore(cert), 0);
        X509_gmtime_adj(X509_getm_notAfter(cert), 3600);
        X509_set_pubkey(cert, serverKey());
        X509_NAME* name = X509_get_subject_name(cert);
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char*>("phone"), -1, -1, 0);
        X509_set_issuer_name(cert, name);
        X509_sign(cert, serverKey(), EVP_sha256());
        return cert;
    }();
    return certificate;
}

// The phone's side: TLS server on memory BIOs
class TlsServer
{
public:
    TlsServer(int version, const char* cipherList)
    {
        context_ = SSL_CTX_new(TLS_server_method());
        SSL_CTX_set_min_proto_version(context_, version);
        SSL_CTX_set_max_proto_version(context_, version);
        if(cipherList != nullptr)
        {
            SSL_CTX_set_cipher_list(context_, cipherList);
        }
        SSL_CTX_use_certificate(context_, serverCertificate());
        SSL_CTX_use_PrivateKey(context_, serverKey());
        ssl_ = SSL_new(context_);
        in_ = BIO_new(BIO_s_mem());
        out_ = BIO_new(BIO_s_mem());
        SSL_set_bio(ssl_, in_, out_);
        SSL_set_accept_state(ssl_);
    }

    ~TlsServer()
    {
        SSL_free(ssl_);
        SSL_CTX_free(context_);
    }

    void feed(const common::Data& data)
    {
        if(!data.empty())
        {
            BIO_write(in_, data.data(), static_cast<int>(data.size()));
        }
    }

    common::Data drain()
    {
        common::Data data(BIO_ctrl_pending(out_));
        if(!data.empty())
        {
            BIO_read(out_, data.data(), static_cast<int>(data.size()));
        }
        return data;
    }

    int handshake()
    {
        return SSL_do_handshake(ssl_);
    }

    // one SSL_write; OpenSSL splits it into records of at most 16 kB
    common::Data encrypt(const std::string& plain)
    {
        EXPECT_EQ(SSL_write(ssl_, plain.data(), static_cast<int>(plain.size())), static_cast<int>(plain.size()));
        return drain();
    }

    std::string cipherName() const
    {
        return SSL_CIPHER_get_name(SSL_get_current_cipher(ssl_));
    }

    std::string versionName() const
    {
        return SSL_get_version(ssl_);
    }

private:
    SSL_CTX* context_;
    SSL* ssl_;
    BIO* in_;
    BIO* out_;
};

class CryptorTlsUnitTest : public testing::Test
{
protected:
    CryptorTlsUnitTest()
        : cryptor_(std::make_shared<transport::SSLWrapper>())
    {
        cryptor_.init();
    }

    ~CryptorTlsUnitTest() override
    {
        cryptor_.deinit();
    }

    void handshake(TlsServer& server)
    {
        for(int round = 0; round < 10 && !cryptor_.doHandshake(); ++round)
        {
            server.feed(cryptor_.readHandshakeBuffer());
            server.handshake();
            cryptor_.writeHandshakeBuffer(common::DataConstBuffer(server.drain()));
        }
        ASSERT_TRUE(cryptor_.isActive());
        server.feed(cryptor_.readHandshakeBuffer());
        ASSERT_EQ(server.handshake(), 1);
    }

    std::string decrypt(const common::Data& frame)
    {
        common::Data output;
        const auto size = cryptor_.decrypt(output, common::DataConstBuffer(frame), static_cast<int>(frame.size()));
        EXPECT_EQ(size, output.size());
        return std::string(output.begin(), output.end());
    }

    Cryptor cryptor_;
};

// records in the order they were encrypted; encrypt the first one before calling this,
// function arguments are evaluated in no fixed order
common::Data concat(common::Data a, const common::Data& b)
{
    a.insert(a.end(), b.begin(), b.end());
    return a;
}

}

TEST_F(CryptorTlsUnitTest, Tls12Gcm_OneRecord)
{
    TlsServer server(TLS1_2_VERSION, "ECDHE-RSA-AES128-GCM-SHA256");
    handshake(server);
    EXPECT_EQ(server.cipherName(), "ECDHE-RSA-AES128-GCM-SHA256");

    EXPECT_EQ(decrypt(server.encrypt("hello")), "hello");
}

TEST_F(CryptorTlsUnitTest, Tls12Gcm_TwoRecordsInOneFrame)
{
    TlsServer server(TLS1_2_VERSION, "ECDHE-RSA-AES128-GCM-SHA256");
    handshake(server);

    const auto first = server.encrypt("hello ");
    const auto frame = concat(first, server.encrypt("world"));
    EXPECT_EQ(decrypt(frame), "hello world");
}

TEST_F(CryptorTlsUnitTest, Tls12Gcm_MessageLargerThanOneRecord)
{
    TlsServer server(TLS1_2_VERSION, "ECDHE-RSA-AES128-GCM-SHA256");
    handshake(server);

    const std::string plain(40000, 'x');
    EXPECT_EQ(decrypt(server.encrypt(plain)), plain);
}

TEST_F(CryptorTlsUnitTest, Tls12ChaCha20_OverheadOf21Bytes)
{
    TlsServer server(TLS1_2_VERSION, "ECDHE-RSA-CHACHA20-POLY1305");
    handshake(server);
    EXPECT_EQ(server.cipherName(), "ECDHE-RSA-CHACHA20-POLY1305");

    EXPECT_EQ(decrypt(server.encrypt("hello")), "hello");
    const auto first = server.encrypt("a");
    EXPECT_EQ(decrypt(concat(first, server.encrypt("bc"))), "abc");
}

TEST_F(CryptorTlsUnitTest, Tls12Cbc_PaddedRecords)
{
    // static RSA with CBC and SHA-1, the only suite some head units offer (aa-proxy-rs, src/ssl_openssl.rs)
    TlsServer server(TLS1_2_VERSION, "AES128-SHA:@SECLEVEL=0");
    handshake(server);
    EXPECT_EQ(server.cipherName(), "AES128-SHA");

    for(const std::string plain : {"a", "0123456789abcdef", "0123456789abcdefg"})
    {
        EXPECT_EQ(decrypt(server.encrypt(plain)), plain);
    }
}

TEST_F(CryptorTlsUnitTest, Tls13_OverheadOf22Bytes)
{
    TlsServer server(TLS1_3_VERSION, nullptr);
    handshake(server);
    EXPECT_EQ(server.versionName(), "TLSv1.3");

    // the first records after the handshake also carry the server's session tickets (no plain bytes)
    EXPECT_EQ(decrypt(server.encrypt("hello")), "hello");
    const auto first = server.encrypt("hello ");
    EXPECT_EQ(decrypt(concat(first, server.encrypt("world"))), "hello world");
}

TEST_F(CryptorTlsUnitTest, FrameEndingInsideRecord_Rejected)
{
    TlsServer server(TLS1_2_VERSION, "ECDHE-RSA-AES128-GCM-SHA256");
    handshake(server);

    auto frame = server.encrypt("hello");
    frame.pop_back();
    common::Data output;
    try
    {
        cryptor_.decrypt(output, common::DataConstBuffer(frame), static_cast<int>(frame.size()));
        FAIL() << "no error for a frame that ends inside a TLS record";
    }
    catch(const error::Error& e)
    {
        EXPECT_EQ(e.getCode(), error::ErrorCode::MESSENGER_FRAME_TOO_SHORT);
    }
    EXPECT_TRUE(output.empty());
}

TEST_F(CryptorTlsUnitTest, Decrypt_AppendsToExistingOutput)
{
    TlsServer server(TLS1_2_VERSION, "ECDHE-RSA-AES128-GCM-SHA256");
    handshake(server);

    // MessageInStream decrypts every frame of a message into the same payload
    common::Data output{'a', 'b'};
    const auto frame = server.encrypt("cd");
    EXPECT_EQ(cryptor_.decrypt(output, common::DataConstBuffer(frame), static_cast<int>(frame.size())), 2u);
    EXPECT_EQ(std::string(output.begin(), output.end()), "abcd");
}

}
}
}
