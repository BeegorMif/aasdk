#pragma once

#include <gmock/gmock.h>
#include <aasdk/Transport/ISSLWrapper.hpp>


namespace aasdk
{
namespace transport
{
namespace ut
{

class SSLWrapperMock: public ISSLWrapper
{
public:
    MOCK_METHOD1(readCertificate, X509*(const std::string& certificate));
    MOCK_METHOD1(readPrivateKey, EVP_PKEY*(const std::string& privateKey));
    MOCK_METHOD0(getMethod, const SSL_METHOD*());
    MOCK_METHOD1(createContext, SSL_CTX*(const SSL_METHOD* method));
    MOCK_METHOD2(useCertificate, bool(SSL_CTX* context, X509* certificate));
    MOCK_METHOD2(usePrivateKey, bool(SSL_CTX* context, EVP_PKEY* privateKey));
    MOCK_METHOD1(createInstance, SSL*(SSL_CTX* context));
    MOCK_METHOD1(checkPrivateKey, bool(SSL* ssl));
    MOCK_METHOD0(createBIOs, std::pair<BIO*, BIO*>());
    MOCK_METHOD3(setBIOs, void(SSL* ssl, const BIOs& bIOs, size_t maxBufferSize));
    MOCK_METHOD1(setConnectState, void(SSL* ssl));
    MOCK_METHOD1(doHandshake, int(SSL* ssl));
    MOCK_METHOD1(free, void(SSL* ssl));
    MOCK_METHOD1(free, void(SSL_CTX* context));
    MOCK_METHOD1(free, void(BIO* bio));
    MOCK_METHOD1(free, void(X509* certificate));
    MOCK_METHOD1(free, void(EVP_PKEY* privateKey));
    MOCK_METHOD1(bioCtrlPending, size_t(BIO* b));
    MOCK_METHOD3(bioRead, int(BIO* b, void* data, int len));
    MOCK_METHOD3(bioWrite, int(BIO* b, const void* data, int len));
    MOCK_METHOD1(getAvailableBytes, int(const SSL* ssl));
    MOCK_METHOD3(sslRead, int(SSL* ssl, void* buf, int num));
    MOCK_METHOD3(sslWrite, int(SSL* ssl, const void* buf, int num));
    MOCK_METHOD2(getError, int(SSL* ssl, int returnCode));
    MOCK_METHOD1(hasPending, bool(const SSL* ssl));
};

}
}
}
