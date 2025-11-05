#ifndef INTEGRITY_CHECKER_H
#define INTEGRITY_CHECKER_H

#include <Arduino.h>
#include <vector>
#include <map>
#include "storage.h"
#include "mbedtls/md5.h"
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"

/**
 * @brief Hash Algorithm Types
 */
enum HashAlgorithm {
    HASH_MD5,
    HASH_SHA1,
    HASH_SHA256,
    HASH_SHA512
};

/**
 * @brief Validation Status
 */
enum ValidationStatus {
    VALIDATION_PASS,
    VALIDATION_FAIL,
    VALIDATION_WARNING,
    VALIDATION_UNKNOWN
};

/**
 * @brief File Integrity Record
 */
struct IntegrityRecord {
    String file_path;
    uint32_t file_size;
    String md5_hash;
    String sha1_hash;
    String sha256_hash;
    unsigned long timestamp_created;
    unsigned long timestamp_verified;
    String collector_id;
    String evidence_id;
    ValidationStatus status;
    String validation_message;
};

/**
 * @brief Chain of Custody Entry
 */
struct CustodyEntry {
    String entry_id;
    String evidence_id;
    String action;              // collected, verified, transferred, analyzed, etc.
    String actor;               // Who performed the action
    String location;            // Where the action occurred
    unsigned long timestamp;
    String notes;
    String hash_before;
    String hash_after;
};

/**
 * @brief Evidence Container
 */
struct EvidenceContainer {
    String container_id;
    String case_id;
    String collector_name;
    String target_system;
    unsigned long collection_start;
    unsigned long collection_end;
    std::vector<IntegrityRecord> artifacts;
    std::vector<CustodyEntry> chain_of_custody;
    String container_hash;
    bool sealed;
    bool tampered;
};

/**
 * @brief Validation Report
 */
struct ValidationReport {
    String report_id;
    unsigned long timestamp;
    uint32_t total_files_checked;
    uint32_t files_passed;
    uint32_t files_failed;
    uint32_t files_warning;
    uint32_t files_missing;
    std::vector<String> failed_files;
    std::vector<String> warning_files;
    String summary;
};

/**
 * @brief Integrity Checker & Chain of Custody Manager
 *
 * Ensures forensic integrity through cryptographic hashing,
 * validation, and chain of custody tracking
 */
class IntegrityChecker {
public:
    IntegrityChecker();
    ~IntegrityChecker();

    // Initialization
    void begin(FRFDStorage* storage_ptr);

    // Hash Calculation
    String calculateMD5(const String& file_path);
    String calculateSHA1(const String& file_path);
    String calculateSHA256(const String& file_path);
    String calculateHash(const String& file_path, HashAlgorithm algorithm);
    String calculateBufferMD5(const uint8_t* buffer, size_t length);
    String calculateBufferSHA256(const uint8_t* buffer, size_t length);

    // Integrity Recording
    bool recordArtifact(const String& file_path, const String& evidence_id);
    bool recordArtifactWithHashes(const String& file_path, const String& evidence_id,
                                  const String& md5, const String& sha1, const String& sha256);
    IntegrityRecord getIntegrityRecord(const String& file_path);
    std::vector<IntegrityRecord> getAllRecords();

    // Validation
    ValidationStatus validateArtifact(const String& file_path);
    ValidationStatus validateArtifactHash(const String& file_path, const String& expected_hash,
                                         HashAlgorithm algorithm);
    bool validateAllArtifacts();
    ValidationReport generateValidationReport();

    // Chain of Custody
    void initializeChainOfCustody(const String& case_id, const String& collector_name);
    void addCustodyEntry(const String& evidence_id, const String& action,
                        const String& actor, const String& notes);
    void recordCollection(const String& evidence_id, const String& file_path);
    void recordVerification(const String& evidence_id, bool passed);
    void recordTransfer(const String& evidence_id, const String& recipient);
    void recordAnalysis(const String& evidence_id, const String& analyst);
    std::vector<CustodyEntry> getChainOfCustody(const String& evidence_id);
    std::vector<CustodyEntry> getAllCustodyEntries();

    // Evidence Container
    bool createContainer(const String& container_id, const String& case_id);
    bool addToContainer(const String& container_id, const String& file_path);
    bool sealContainer(const String& container_id);
    bool verifyContainer(const String& container_id);
    EvidenceContainer getContainer(const String& container_id);
    bool exportContainer(const String& container_id, const String& output_path);

    // Tamper Detection
    bool detectTampering(const String& file_path);
    std::vector<String> scanForTamperedFiles();
    void enableTamperMonitoring(bool enabled);

    // Forensic Manifest
    bool generateManifest(const String& output_path);
    bool loadManifest(const String& manifest_path);
    bool validateAgainstManifest(const String& manifest_path);

    // Export
    String exportIntegrityRecordsJSON();
    String exportChainOfCustodyJSON();
    String exportContainerJSON(const String& container_id);
    bool saveIntegrityDatabase(const String& filename);
    bool loadIntegrityDatabase(const String& filename);

    // Statistics
    uint32_t getTotalRecords() const { return integrity_records.size(); }
    uint32_t getTotalCustodyEntries() const { return custody_entries.size(); }
    uint32_t getValidatedCount() const;
    uint32_t getFailedCount() const;

private:
    FRFDStorage* storage;

    std::map<String, IntegrityRecord> integrity_records;
    std::vector<CustodyEntry> custody_entries;
    std::map<String, EvidenceContainer> containers;

    String collector_id;
    String current_case_id;
    bool tamper_monitoring_enabled;

    // Hash helpers
    String bytesToHex(const uint8_t* bytes, size_t length);
    bool hashFile(const String& file_path, HashAlgorithm algorithm, uint8_t* output);

    // Validation helpers
    bool compareHashes(const String& hash1, const String& hash2);
    ValidationStatus checkFileIntegrity(const IntegrityRecord& record);

    // Chain of custody helpers
    String generateEntryId();
    String generateEvidenceId();
    String getCurrentLocation();

    // Container helpers
    String calculateContainerHash(const EvidenceContainer& container);
    bool writeContainerManifest(const EvidenceContainer& container, const String& path);
};

#endif // INTEGRITY_CHECKER_H
