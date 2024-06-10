#include <assert.h>
#include <cstring>
#include <iostream>
#include <memory>
#include <openssl/aes.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>
#include <tuple>

#if 1
std::string privateKey =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIIEowIBAAKCAQEAy8Dbv8prpJ/0kKhlGeJYozo2t60EG8L0561g13R29LvMR5hy\n"
    "vGZlGJpmn65+A4xHXInJYiPuKzrKUnApeLZ+vw1HocOAZtWK0z3r26uA8kQYOKX9\n"
    "Qt/DbCdvsF9wF8gRK0ptx9M6R13NvBxvVQApfc9jB9nTzphOgM4JiEYvlV8FLhg9\n"
    "yZovMYd6Wwf3aoXK891VQxTr/kQYoq1Yp+68i6T4nNq7NWC+UNVjQHxNQMQMzU6l\n"
    "WCX8zyg3yH88OAQkUXIXKfQ+NkvYQ1cxaMoVPpY72+eVthKzpMeyHkBn7ciumk5q\n"
    "gLTEJAfWZpe4f4eFZj/Rc8Y8Jj2IS5kVPjUywQIDAQABAoIBADhg1u1Mv1hAAlX8\n"
    "omz1Gn2f4AAW2aos2cM5UDCNw1SYmj+9SRIkaxjRsE/C4o9sw1oxrg1/z6kajV0e\n"
    "N/t008FdlVKHXAIYWF93JMoVvIpMmT8jft6AN/y3NMpivgt2inmmEJZYNioFJKZG\n"
    "X+/vKYvsVISZm2fw8NfnKvAQK55yu+GRWBZGOeS9K+LbYvOwcrjKhHz66m4bedKd\n"
    "gVAix6NE5iwmjNXktSQlJMCjbtdNXg/xo1/G4kG2p/MO1HLcKfe1N5FgBiXj3Qjl\n"
    "vgvjJZkh1as2KTgaPOBqZaP03738VnYg23ISyvfT/teArVGtxrmFP7939EvJFKpF\n"
    "1wTxuDkCgYEA7t0DR37zt+dEJy+5vm7zSmN97VenwQJFWMiulkHGa0yU3lLasxxu\n"
    "m0oUtndIjenIvSx6t3Y+agK2F3EPbb0AZ5wZ1p1IXs4vktgeQwSSBdqcM8LZFDvZ\n"
    "uPboQnJoRdIkd62XnP5ekIEIBAfOp8v2wFpSfE7nNH2u4CpAXNSF9HsCgYEA2l8D\n"
    "JrDE5m9Kkn+J4l+AdGfeBL1igPF3DnuPoV67BpgiaAgI4h25UJzXiDKKoa706S0D\n"
    "4XB74zOLX11MaGPMIdhlG+SgeQfNoC5lE4ZWXNyESJH1SVgRGT9nBC2vtL6bxCVV\n"
    "WBkTeC5D6c/QXcai6yw6OYyNNdp0uznKURe1xvMCgYBVYYcEjWqMuAvyferFGV+5\n"
    "nWqr5gM+yJMFM2bEqupD/HHSLoeiMm2O8KIKvwSeRYzNohKTdZ7FwgZYxr8fGMoG\n"
    "PxQ1VK9DxCvZL4tRpVaU5Rmknud9hg9DQG6xIbgIDR+f79sb8QjYWmcFGc1SyWOA\n"
    "SkjlykZ2yt4xnqi3BfiD9QKBgGqLgRYXmXp1QoVIBRaWUi55nzHg1XbkWZqPXvz1\n"
    "I3uMLv1jLjJlHk3euKqTPmC05HoApKwSHeA0/gOBmg404xyAYJTDcCidTg6hlF96\n"
    "ZBja3xApZuxqM62F6dV4FQqzFX0WWhWp5n301N33r0qR6FumMKJzmVJ1TA8tmzEF\n"
    "yINRAoGBAJqioYs8rK6eXzA8ywYLjqTLu/yQSLBn/4ta36K8DyCoLNlNxSuox+A5\n"
    "w6z2vEfRVQDq4Hm4vBzjdi3QfYLNkTiTqLcvgWZ+eX44ogXtdTDO7c+GeMKWz4XX\n"
    "uJSUVL5+CVjKLjZEJ6Qc2WZLl94xSwL71E41H4YciVnSCQxVc4Jw\n"
    "-----END RSA PRIVATE KEY-----\n\0";

std::string publicKey =
    "-----BEGIN PUBLIC KEY-----\n"
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAy8Dbv8prpJ/0kKhlGeJY\n"
    "ozo2t60EG8L0561g13R29LvMR5hyvGZlGJpmn65+A4xHXInJYiPuKzrKUnApeLZ+\n"
    "vw1HocOAZtWK0z3r26uA8kQYOKX9Qt/DbCdvsF9wF8gRK0ptx9M6R13NvBxvVQAp\n"
    "fc9jB9nTzphOgM4JiEYvlV8FLhg9yZovMYd6Wwf3aoXK891VQxTr/kQYoq1Yp+68\n"
    "i6T4nNq7NWC+UNVjQHxNQMQMzU6lWCX8zyg3yH88OAQkUXIXKfQ+NkvYQ1cxaMoV\n"
    "PpY72+eVthKzpMeyHkBn7ciumk5qgLTEJAfWZpe4f4eFZj/Rc8Y8Jj2IS5kVPjUy\n"
    "wQIDAQAB\n"
    "-----END PUBLIC KEY-----\n";
#endif

static RSA *create_priv_RSA(std::string key) {
  RSA *rsa = NULL;
  const char *c_string = key.c_str();
  BIO *keybio = BIO_new_mem_buf((void *)c_string, -1);
  if (keybio == NULL) {
    return 0;
  }
  rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL);
  BIO_free(keybio);
  return rsa;
}

static RSA *create_pub_RSA(std::string key) {
  RSA *rsa = NULL;
  BIO *keybio;
  const char *c_string = key.c_str();
  keybio = BIO_new_mem_buf((void *)c_string, -1);
  if (keybio == NULL) {
    return 0;
  }
  rsa = PEM_read_bio_RSA_PUBKEY(keybio, &rsa, NULL, NULL);
  BIO_free(keybio);
  return rsa;
}

static bool RSA_sign(RSA *rsa, const unsigned char *msg, size_t msg_len,
                     unsigned char **enc_msg, size_t *enc_msg_len) {
  EVP_MD_CTX *m_RSA_sign_ctx = EVP_MD_CTX_create();
  EVP_PKEY *pri_key = EVP_PKEY_new();
  EVP_PKEY_assign_RSA(pri_key, rsa);
  if (EVP_DigestSignInit(m_RSA_sign_ctx, NULL, EVP_sha256(), NULL, pri_key) <=
      0) {
    return false;
  }
  if (EVP_DigestSignUpdate(m_RSA_sign_ctx, msg, msg_len) <= 0) {
    return false;
  }
  if (EVP_DigestSignFinal(m_RSA_sign_ctx, NULL, enc_msg_len) <= 0) {
    return false;
  }
  *enc_msg = (unsigned char *)malloc(*enc_msg_len);
  if (EVP_DigestSignFinal(m_RSA_sign_ctx, *enc_msg, enc_msg_len) <= 0) {
    return false;
  }
  EVP_MD_CTX_free(m_RSA_sign_ctx);
  EVP_PKEY_free(pri_key);
  return true;
}

static bool RSA_verify_signature(RSA *rsa, unsigned char *msg_hash,
                                 size_t msg_hash_len, const char *msg,
                                 size_t msg_len, bool *authentic) {
  *authentic = false;
  EVP_PKEY *pub_key = EVP_PKEY_new();
  EVP_PKEY_assign_RSA(pub_key, rsa);
  EVP_MD_CTX *m_RSA_verify_ctx = EVP_MD_CTX_create();

  if (EVP_DigestVerifyInit(m_RSA_verify_ctx, NULL, EVP_sha256(), NULL,
                           pub_key) <= 0) {
    return false;
  }
  if (EVP_DigestVerifyUpdate(m_RSA_verify_ctx, msg, msg_len) <= 0) {
    return false;
  }
  int auth_status =
      EVP_DigestVerifyFinal(m_RSA_verify_ctx, msg_hash, msg_hash_len);
  if (auth_status == 1) {
    *authentic = true;
    EVP_MD_CTX_free(m_RSA_verify_ctx);
    EVP_PKEY_free(pub_key);
    return true;
  } else if (auth_status == 0) {
    *authentic = false;
    EVP_PKEY_free(pub_key);
    EVP_MD_CTX_free(m_RSA_verify_ctx);
    return true;
  } else {
    *authentic = false;
    EVP_PKEY_free(pub_key);
    EVP_MD_CTX_free(m_RSA_verify_ctx);
    return false;
  }
}

static std::tuple<std::unique_ptr<char[]>, size_t>
sign_message(std::string private_key, std::string plain_text) {
  RSA *private_RSA = create_priv_RSA(private_key);
  unsigned char *enc_message;
  size_t enc_message_len;
  RSA_sign(private_RSA, (unsigned char *)plain_text.c_str(),
           plain_text.length(), &enc_message, &enc_message_len);
  auto signature = std::make_unique<char[]>(enc_message_len);
  ::memcpy(signature.get(), enc_message, enc_message_len);
  free(enc_message);
  std::cout << enc_message_len << "\n";
  return {std::move(signature), enc_message_len};
}

static bool verify_signature(std::string public_key, std::string plain_text,
                             char *signature, size_t signature_sz) {
  RSA *publicRSA = create_pub_RSA(publicKey);
  bool authentic;
  // Base64Decode(signatureBase64, &encMessage, &encMessageLength);
  bool result = RSA_verify_signature(
      publicRSA, reinterpret_cast<unsigned char *>(signature), signature_sz,
      plain_text.c_str(), plain_text.length(), &authentic);
  return result && authentic;
}