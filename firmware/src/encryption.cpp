#include "encryption.h"
#include <esp_random.h>

FRFDEncryption::FRFDEncryption() {
    mbedtls_aes_init(&aes_ctx);
    keySet = false;
    memset(encryption_key, 0, sizeof(encryption_key));
    memset(iv, 0, sizeof(iv));
}

FRFDEncryption::~FRFDEncryption() {
    mbedtls_aes_free(&aes_ctx);
    clearKey();
}

void FRFDEncryption::deriveKey(const String& password, const uint8_t* salt, size_t saltLen) {
    // Use PBKDF2 with SHA-256
    mbedtls_md_context_t md_ctx;
    mbedtls_md_init(&md_ctx);

    const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_setup(&md_ctx, md_info, 1);

    // PBKDF2 with 10000 iterations
    mbedtls_pkcs5_pbkdf2_hmac(&md_ctx,
                               (const unsigned char*)password.c_str(),
                               password.length(),
                               salt,
                               saltLen,
                               10000,
                               32,
                               encryption_key);

    mbedtls_md_free(&md_ctx);
}

bool FRFDEncryption::setKeyFromPassword(const String& password) {
    Serial.println("[Encryption] Deriving key from password...");

    if (password.length() < 8) {
        Serial.println("[Encryption] Password too short (min 8 characters)");
        return false;
    }

    // Generate random salt
    uint8_t salt[16];
    esp_fill_random(salt, sizeof(salt));

    // Derive key
    deriveKey(password, salt, sizeof(salt));

    // Generate random IV
    esp_fill_random(iv, sizeof(iv));

    // Set AES key
    int ret = mbedtls_aes_setkey_enc(&aes_ctx, encryption_key, 256);
    if (ret != 0) {
        Serial.printf("[Encryption] Failed to set AES key: %d\n", ret);
        return false;
    }

    keySet = true;
    Serial.println("[Encryption] Key set successfully");
    Serial.printf("[Encryption] Key fingerprint: %s\n", getKeyFingerprint().c_str());

    return true;
}

bool FRFDEncryption::setKeyFromBytes(const uint8_t* key, size_t keyLen) {
    if (keyLen != 32) {
        Serial.println("[Encryption] Invalid key length (must be 32 bytes)");
        return false;
    }

    memcpy(encryption_key, key, 32);

    // Generate random IV
    esp_fill_random(iv, sizeof(iv));

    int ret = mbedtls_aes_setkey_enc(&aes_ctx, encryption_key, 256);
    if (ret != 0) {
        Serial.printf("[Encryption] Failed to set AES key: %d\n", ret);
        return false;
    }

    keySet = true;
    Serial.println("[Encryption] Key set from bytes");
    return true;
}

bool FRFDEncryption::generateRandomKey() {
    Serial.println("[Encryption] Generating random 256-bit key...");

    // Generate random key
    esp_fill_random(encryption_key, sizeof(encryption_key));

    // Generate random IV
    esp_fill_random(iv, sizeof(iv));

    int ret = mbedtls_aes_setkey_enc(&aes_ctx, encryption_key, 256);
    if (ret != 0) {
        Serial.printf("[Encryption] Failed to set AES key: %d\n", ret);
        return false;
    }

    keySet = true;
    Serial.println("[Encryption] Random key generated");
    Serial.printf("[Encryption] Key fingerprint: %s\n", getKeyFingerprint().c_str());

    return true;
}

void FRFDEncryption::clearKey() {
    // Securely clear key material
    memset(encryption_key, 0, sizeof(encryption_key));
    memset(iv, 0, sizeof(iv));
    keySet = false;
}

bool FRFDEncryption::encryptData(const uint8_t* input, size_t inputLen, uint8_t* output, size_t& outputLen) {
    if (!keySet) {
        Serial.println("[Encryption] Key not set");
        return false;
    }

    // Calculate padded length (AES block size = 16)
    size_t paddedLen = ((inputLen + 15) / 16) * 16;

    if (outputLen < paddedLen + 16) { // +16 for IV
        Serial.println("[Encryption] Output buffer too small");
        return false;
    }

    // Copy IV to output
    memcpy(output, iv, 16);

    // Create padded input
    uint8_t* paddedInput = (uint8_t*)malloc(paddedLen);
    if (!paddedInput) {
        Serial.println("[Encryption] Memory allocation failed");
        return false;
    }

    memcpy(paddedInput, input, inputLen);

    // Apply PKCS7 padding
    uint8_t paddingValue = paddedLen - inputLen;
    for (size_t i = inputLen; i < paddedLen; i++) {
        paddedInput[i] = paddingValue;
    }

    // Encrypt using AES-256-CBC
    uint8_t iv_copy[16];
    memcpy(iv_copy, iv, 16);

    int ret = mbedtls_aes_crypt_cbc(&aes_ctx,
                                     MBEDTLS_AES_ENCRYPT,
                                     paddedLen,
                                     iv_copy,
                                     paddedInput,
                                     output + 16);

    free(paddedInput);

    if (ret != 0) {
        Serial.printf("[Encryption] Encryption failed: %d\n", ret);
        return false;
    }

    outputLen = paddedLen + 16; // Encrypted data + IV
    return true;
}

bool FRFDEncryption::decryptData(const uint8_t* input, size_t inputLen, uint8_t* output, size_t& outputLen) {
    if (!keySet) {
        Serial.println("[Encryption] Key not set");
        return false;
    }

    if (inputLen < 17) { // At least IV + 1 block
        Serial.println("[Encryption] Input too short");
        return false;
    }

    // Extract IV
    uint8_t extracted_iv[16];
    memcpy(extracted_iv, input, 16);

    // Decrypt
    size_t encryptedLen = inputLen - 16;

    int ret = mbedtls_aes_crypt_cbc(&aes_ctx,
                                     MBEDTLS_AES_DECRYPT,
                                     encryptedLen,
                                     extracted_iv,
                                     input + 16,
                                     output);

    if (ret != 0) {
        Serial.printf("[Encryption] Decryption failed: %d\n", ret);
        return false;
    }

    // Remove PKCS7 padding
    uint8_t paddingValue = output[encryptedLen - 1];
    if (paddingValue > 0 && paddingValue <= 16) {
        outputLen = encryptedLen - paddingValue;
    } else {
        outputLen = encryptedLen;
    }

    return true;
}

bool FRFDEncryption::encryptFile(const String& inputPath, const String& outputPath) {
    Serial.printf("[Encryption] Encrypting file: %s -> %s\n", inputPath.c_str(), outputPath.c_str());

    // This would read the file, encrypt it, and write to output
    // Placeholder for actual implementation
    return false;
}

bool FRFDEncryption::decryptFile(const String& inputPath, const String& outputPath) {
    Serial.printf("[Encryption] Decrypting file: %s -> %s\n", inputPath.c_str(), outputPath.c_str());

    // This would read the encrypted file and decrypt it
    // Placeholder for actual implementation
    return false;
}

String FRFDEncryption::sha256Hash(const uint8_t* data, size_t length) {
    uint8_t hash[32];

    mbedtls_sha256_context sha_ctx;
    mbedtls_sha256_init(&sha_ctx);
    mbedtls_sha256_starts(&sha_ctx, 0); // 0 = SHA-256 (not SHA-224)
    mbedtls_sha256_update(&sha_ctx, data, length);
    mbedtls_sha256_finish(&sha_ctx, hash);
    mbedtls_sha256_free(&sha_ctx);

    // Convert to hex string
    String hashStr = "";
    for (int i = 0; i < 32; i++) {
        char hex[3];
        sprintf(hex, "%02x", hash[i]);
        hashStr += hex;
    }

    return hashStr;
}

String FRFDEncryption::sha256Hash(const String& data) {
    return sha256Hash((const uint8_t*)data.c_str(), data.length());
}

String FRFDEncryption::exportKey() {
    if (!keySet) {
        return "";
    }

    // Export key as hex string (WARNING: Store securely!)
    String keyHex = "";
    for (int i = 0; i < 32; i++) {
        char hex[3];
        sprintf(hex, "%02x", encryption_key[i]);
        keyHex += hex;
    }

    return keyHex;
}

bool FRFDEncryption::importKey(const String& keyData) {
    if (keyData.length() != 64) { // 32 bytes * 2 hex chars
        Serial.println("[Encryption] Invalid key data length");
        return false;
    }

    // Convert hex string to bytes
    uint8_t key[32];
    for (int i = 0; i < 32; i++) {
        String byteStr = keyData.substring(i * 2, i * 2 + 2);
        key[i] = strtol(byteStr.c_str(), NULL, 16);
    }

    return setKeyFromBytes(key, 32);
}

bool FRFDEncryption::isKeySet() {
    return keySet;
}

String FRFDEncryption::getKeyFingerprint() {
    if (!keySet) {
        return "No key set";
    }

    // Return first 8 bytes of key hash as fingerprint
    String hash = sha256Hash(encryption_key, 32);
    return hash.substring(0, 16);
}
