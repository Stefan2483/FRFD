#ifndef EVIDENCE_CONTAINER_H
#define EVIDENCE_CONTAINER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include <mbedtls/sha256.h>
#include "storage.h"

/**
 * @brief Artifact metadata following NIST SP 800-86 guidelines
 */
struct ArtifactMetadata {
    String artifact_id;           // Unique identifier
    String artifact_type;         // memory, registry, logs, network, etc.
    String filename;              // Original filename
    String storage_path;          // Path on SD card
    uint32_t file_size;           // Size in bytes
    String sha256_hash;           // Integrity hash
    unsigned long collected_at;   // Timestamp (ms since boot)
    String collection_method;     // HID_AUTO, MANUAL, SCRIPT
    String source_path;           // Original path on target system
    bool integrity_verified;      // Hash verified after transfer
    bool compressed;              // Whether artifact is compressed
    uint32_t original_size;       // Size before compression
    String error_message;         // Error if collection failed
};

/**
 * @brief Collection action log entry
 */
struct CollectionAction {
    unsigned long timestamp;
    String action_type;
    String details;
    String result;
    String integrity_hash;
};

/**
 * @brief Target system information
 */
struct TargetSystemInfo {
    String os_name;
    String os_version;
    String hostname;
    String ip_address;
    String mac_address;
    unsigned long system_time;
    String timezone;
    bool is_admin;
};

/**
 * @brief Evidence Container
 *
 * Implements forensically sound evidence packaging following:
 * - NIST SP 800-86 (Guide to Integrating Forensic Techniques)
 * - ISO/IEC 27037:2012 (Digital Evidence Guidelines)
 * - RFC 3227 (Evidence Collection and Archiving)
 *
 * Features:
 * - Organized directory structure
 * - Complete metadata for each artifact
 * - Chain of custody tracking
 * - Integrity verification (SHA-256)
 * - Write-once semantics
 * - Compression support
 * - Error logging
 */
class EvidenceContainer {
public:
    EvidenceContainer(FRFDStorage* storage);
    ~EvidenceContainer();

    // Container lifecycle
    bool createContainer(const String& caseId, const String& responder = "");
    bool openContainer(const String& caseId);
    bool finalizeContainer();
    bool isOpen() const { return container_open; }

    // Artifact management
    String addArtifact(const String& type, const String& filename,
                      const uint8_t* data, size_t size, bool compress = true);
    bool addArtifactMetadata(const String& artifactId, const ArtifactMetadata& meta);
    bool verifyArtifactIntegrity(const String& artifactId);
    bool removeArtifact(const String& artifactId);

    // System information
    void setTargetSystemInfo(const TargetSystemInfo& info);
    TargetSystemInfo getTargetSystemInfo() const { return target_system; }

    // Action logging
    void logAction(const String& actionType, const String& details, const String& result);
    std::vector<CollectionAction> getActionLog() const { return actions; }

    // Chain of custody
    bool generateChainOfCustody();
    bool generateManifest();
    bool signEvidence(const String& signature);

    // Statistics
    uint32_t getArtifactCount() const { return artifacts.size(); }
    uint32_t getTotalSize() const;
    uint32_t getCompressedSize() const;
    float getCompressionRatio() const;
    unsigned long getCollectionDuration() const;

    // Validation
    bool validateContainer();
    bool verifyAllArtifacts();
    std::vector<String> getValidationErrors() const { return validation_errors; }

    // Getters
    String getCaseId() const { return case_id; }
    String getContainerPath() const { return container_path; }
    const std::vector<ArtifactMetadata>& getArtifacts() const { return artifacts; }

private:
    FRFDStorage* storage;

    // Container state
    bool container_open;
    bool finalized;
    String case_id;
    String container_path;
    String responder;

    // Timing
    unsigned long collection_start_time;
    unsigned long collection_end_time;

    // Target system
    TargetSystemInfo target_system;

    // Artifacts
    std::vector<ArtifactMetadata> artifacts;
    uint32_t artifact_sequence;

    // Actions
    std::vector<CollectionAction> actions;
    uint32_t action_sequence;

    // Validation
    std::vector<String> validation_errors;

    // Helper methods
    String generateArtifactId();
    String calculateSHA256(const uint8_t* data, size_t length);
    String calculateFileSHA256(const String& filepath);
    bool createDirectoryStructure();
    bool compressData(const uint8_t* input, size_t inputLen, uint8_t** output, size_t* outputLen);
    String getCurrentTimestamp();
    String formatTimestamp(unsigned long timestamp);
    bool saveJSON(const String& filename, const JsonDocument& doc);
    bool loadJSON(const String& filename, JsonDocument& doc);
};

// Compression helper (simple RLE or use zlib if available)
class SimpleCompressor {
public:
    static bool compress(const uint8_t* input, size_t inputLen,
                        uint8_t* output, size_t maxOutputLen, size_t* outputLen);
    static bool decompress(const uint8_t* input, size_t inputLen,
                          uint8_t* output, size_t maxOutputLen, size_t* outputLen);
};

#endif // EVIDENCE_CONTAINER_H
