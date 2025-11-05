#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <Arduino.h>
#include <mbedtls/aes.h>
#include <mbedtls/sha256.h>
#include <mbedtls/md.h>

class FRFDEncryption {
private:
    mbedtls_aes_context aes_ctx;
    uint8_t encryption_key[32]; // 256-bit key
    uint8_t iv[16]; // Initialization vector
    bool keySet;

    // Key derivation
    void deriveKey(const String& password, const uint8_t* salt, size_t saltLen);

public:
    FRFDEncryption();
    ~FRFDEncryption();

    // Key management
    bool setKeyFromPassword(const String& password);
    bool setKeyFromBytes(const uint8_t* key, size_t keyLen);
    bool generateRandomKey();
    void clearKey();

    // Encryption/Decryption
    bool encryptData(const uint8_t* input, size_t inputLen, uint8_t* output, size_t& outputLen);
    bool decryptData(const uint8_t* input, size_t inputLen, uint8_t* output, size_t& outputLen);

    // File operations
    bool encryptFile(const String& inputPath, const String& outputPath);
    bool decryptFile(const String& inputPath, const String& outputPath);

    // Hashing
    String sha256Hash(const uint8_t* data, size_t length);
    String sha256Hash(const String& data);

    // Key export (encrypted)
    String exportKey();
    bool importKey(const String& keyData);

    // Utilities
    bool isKeySet();
    String getKeyFingerprint();
};

#endif // ENCRYPTION_H
