#include "integrity_checker.h"

IntegrityChecker::IntegrityChecker()
    : storage(nullptr),
      tamper_monitoring_enabled(true) {
}

IntegrityChecker::~IntegrityChecker() {
}

void IntegrityChecker::begin(FRFDStorage* storage_ptr) {
    storage = storage_ptr;
    collector_id = "FRFD_" + String(ESP.getEfuseMac(), HEX);

    Serial.println("[IntegrityChecker] Initialized");
    Serial.println("[IntegrityChecker] Collector ID: " + collector_id);
}

// ===========================
// Hash Calculation
// ===========================

String IntegrityChecker::calculateMD5(const String& file_path) {
    return calculateHash(file_path, HASH_MD5);
}

String IntegrityChecker::calculateSHA1(const String& file_path) {
    return calculateHash(file_path, HASH_SHA1);
}

String IntegrityChecker::calculateSHA256(const String& file_path) {
    return calculateHash(file_path, HASH_SHA256);
}

String IntegrityChecker::calculateHash(const String& file_path, HashAlgorithm algorithm) {
    uint8_t hash_output[64]; // Maximum hash size (SHA512)
    size_t hash_size = 0;

    switch (algorithm) {
        case HASH_MD5: hash_size = 16; break;
        case HASH_SHA1: hash_size = 20; break;
        case HASH_SHA256: hash_size = 32; break;
        case HASH_SHA512: hash_size = 64; break;
    }

    if (!hashFile(file_path, algorithm, hash_output)) {
        return "";
    }

    return bytesToHex(hash_output, hash_size);
}

String IntegrityChecker::calculateBufferMD5(const uint8_t* buffer, size_t length) {
    uint8_t hash[16];
    mbedtls_md5_context md5_ctx;

    mbedtls_md5_init(&md5_ctx);
    mbedtls_md5_starts(&md5_ctx);
    mbedtls_md5_update(&md5_ctx, buffer, length);
    mbedtls_md5_finish(&md5_ctx, hash);
    mbedtls_md5_free(&md5_ctx);

    return bytesToHex(hash, 16);
}

String IntegrityChecker::calculateBufferSHA256(const uint8_t* buffer, size_t length) {
    uint8_t hash[32];
    mbedtls_sha256_context sha256_ctx;

    mbedtls_sha256_init(&sha256_ctx);
    mbedtls_sha256_starts(&sha256_ctx, 0); // 0 for SHA256, 1 for SHA224
    mbedtls_sha256_update(&sha256_ctx, buffer, length);
    mbedtls_sha256_finish(&sha256_ctx, hash);
    mbedtls_sha256_free(&sha256_ctx);

    return bytesToHex(hash, 32);
}

// ===========================
// Integrity Recording
// ===========================

bool IntegrityChecker::recordArtifact(const String& file_path, const String& evidence_id) {
    if (!storage) return false;

    File file = storage->openFile(file_path, FILE_READ);
    if (!file) {
        Serial.println("[IntegrityChecker] Failed to open file: " + file_path);
        return false;
    }

    IntegrityRecord record;
    record.file_path = file_path;
    record.file_size = file.size();
    file.close();

    // Calculate hashes
    Serial.println("[IntegrityChecker] Calculating hashes for: " + file_path);
    record.md5_hash = calculateMD5(file_path);
    record.sha1_hash = calculateSHA1(file_path);
    record.sha256_hash = calculateSHA256(file_path);

    record.timestamp_created = millis();
    record.timestamp_verified = 0;
    record.collector_id = collector_id;
    record.evidence_id = evidence_id;
    record.status = VALIDATION_UNKNOWN;

    integrity_records[file_path] = record;

    // Add to chain of custody
    addCustodyEntry(evidence_id, "COLLECTED", collector_id,
                   "Artifact collected and hashed: " + file_path);

    Serial.println("[IntegrityChecker] Recorded artifact: " + file_path);
    Serial.println("  MD5: " + record.md5_hash);
    Serial.println("  SHA256: " + record.sha256_hash);

    return true;
}

bool IntegrityChecker::recordArtifactWithHashes(const String& file_path, const String& evidence_id,
                                               const String& md5, const String& sha1, const String& sha256) {
    if (!storage) return false;

    File file = storage->openFile(file_path, FILE_READ);
    if (!file) return false;

    IntegrityRecord record;
    record.file_path = file_path;
    record.file_size = file.size();
    file.close();

    record.md5_hash = md5;
    record.sha1_hash = sha1;
    record.sha256_hash = sha256;
    record.timestamp_created = millis();
    record.timestamp_verified = 0;
    record.collector_id = collector_id;
    record.evidence_id = evidence_id;
    record.status = VALIDATION_UNKNOWN;

    integrity_records[file_path] = record;

    return true;
}

IntegrityRecord IntegrityChecker::getIntegrityRecord(const String& file_path) {
    if (integrity_records.find(file_path) != integrity_records.end()) {
        return integrity_records[file_path];
    }
    return IntegrityRecord();
}

std::vector<IntegrityRecord> IntegrityChecker::getAllRecords() {
    std::vector<IntegrityRecord> records;
    for (const auto& pair : integrity_records) {
        records.push_back(pair.second);
    }
    return records;
}

// ===========================
// Validation
// ===========================

ValidationStatus IntegrityChecker::validateArtifact(const String& file_path) {
    if (integrity_records.find(file_path) == integrity_records.end()) {
        Serial.println("[IntegrityChecker] No integrity record found for: " + file_path);
        return VALIDATION_UNKNOWN;
    }

    IntegrityRecord& record = integrity_records[file_path];

    Serial.println("[IntegrityChecker] Validating: " + file_path);

    // Check if file still exists
    if (!storage) return VALIDATION_FAIL;

    File file = storage->openFile(file_path, FILE_READ);
    if (!file) {
        record.status = VALIDATION_FAIL;
        record.validation_message = "File not found";
        Serial.println("[IntegrityChecker] FAIL: File not found");
        return VALIDATION_FAIL;
    }

    // Check file size
    if (file.size() != record.file_size) {
        record.status = VALIDATION_FAIL;
        record.validation_message = "File size mismatch";
        file.close();
        Serial.println("[IntegrityChecker] FAIL: File size mismatch");
        return VALIDATION_FAIL;
    }
    file.close();

    // Recalculate and compare hashes
    String current_md5 = calculateMD5(file_path);
    String current_sha256 = calculateSHA256(file_path);

    if (!compareHashes(current_md5, record.md5_hash)) {
        record.status = VALIDATION_FAIL;
        record.validation_message = "MD5 hash mismatch - file has been modified";
        Serial.println("[IntegrityChecker] FAIL: MD5 mismatch");
        return VALIDATION_FAIL;
    }

    if (!compareHashes(current_sha256, record.sha256_hash)) {
        record.status = VALIDATION_FAIL;
        record.validation_message = "SHA256 hash mismatch - file has been modified";
        Serial.println("[IntegrityChecker] FAIL: SHA256 mismatch");
        return VALIDATION_FAIL;
    }

    // Validation passed
    record.status = VALIDATION_PASS;
    record.validation_message = "Integrity verified";
    record.timestamp_verified = millis();

    Serial.println("[IntegrityChecker] PASS: Integrity verified");

    // Add to chain of custody
    addCustodyEntry(record.evidence_id, "VERIFIED", collector_id,
                   "Integrity verification passed");

    return VALIDATION_PASS;
}

ValidationStatus IntegrityChecker::validateArtifactHash(const String& file_path,
                                                       const String& expected_hash,
                                                       HashAlgorithm algorithm) {
    String current_hash = calculateHash(file_path, algorithm);

    if (compareHashes(current_hash, expected_hash)) {
        return VALIDATION_PASS;
    } else {
        return VALIDATION_FAIL;
    }
}

bool IntegrityChecker::validateAllArtifacts() {
    Serial.println("[IntegrityChecker] Validating all artifacts...");

    uint32_t validated = 0;
    uint32_t failed = 0;

    for (auto& pair : integrity_records) {
        ValidationStatus status = validateArtifact(pair.first);
        if (status == VALIDATION_PASS) {
            validated++;
        } else {
            failed++;
        }
    }

    Serial.println("[IntegrityChecker] Validation complete: " + String(validated) +
                   " passed, " + String(failed) + " failed");

    return failed == 0;
}

ValidationReport IntegrityChecker::generateValidationReport() {
    ValidationReport report;
    report.report_id = "VAL_" + String(millis());
    report.timestamp = millis();
    report.total_files_checked = integrity_records.size();
    report.files_passed = 0;
    report.files_failed = 0;
    report.files_warning = 0;
    report.files_missing = 0;

    for (const auto& pair : integrity_records) {
        const IntegrityRecord& record = pair.second;

        switch (record.status) {
            case VALIDATION_PASS:
                report.files_passed++;
                break;
            case VALIDATION_FAIL:
                report.files_failed++;
                report.failed_files.push_back(record.file_path);
                break;
            case VALIDATION_WARNING:
                report.files_warning++;
                report.warning_files.push_back(record.file_path);
                break;
            default:
                break;
        }
    }

    report.summary = "Validated " + String(report.total_files_checked) + " artifacts. ";
    report.summary += String(report.files_passed) + " passed, ";
    report.summary += String(report.files_failed) + " failed, ";
    report.summary += String(report.files_warning) + " warnings.";

    return report;
}

// ===========================
// Chain of Custody
// ===========================

void IntegrityChecker::initializeChainOfCustody(const String& case_id, const String& collector_name) {
    current_case_id = case_id;

    CustodyEntry entry;
    entry.entry_id = generateEntryId();
    entry.evidence_id = "CASE_" + case_id;
    entry.action = "CASE_INITIALIZED";
    entry.actor = collector_name;
    entry.location = getCurrentLocation();
    entry.timestamp = millis();
    entry.notes = "Forensic collection case initialized";

    custody_entries.push_back(entry);

    Serial.println("[IntegrityChecker] Initialized chain of custody for case: " + case_id);
}

void IntegrityChecker::addCustodyEntry(const String& evidence_id, const String& action,
                                      const String& actor, const String& notes) {
    CustodyEntry entry;
    entry.entry_id = generateEntryId();
    entry.evidence_id = evidence_id;
    entry.action = action;
    entry.actor = actor;
    entry.location = getCurrentLocation();
    entry.timestamp = millis();
    entry.notes = notes;

    custody_entries.push_back(entry);
}

void IntegrityChecker::recordCollection(const String& evidence_id, const String& file_path) {
    IntegrityRecord record = getIntegrityRecord(file_path);

    CustodyEntry entry;
    entry.entry_id = generateEntryId();
    entry.evidence_id = evidence_id;
    entry.action = "COLLECTED";
    entry.actor = collector_id;
    entry.location = getCurrentLocation();
    entry.timestamp = millis();
    entry.notes = "Collected: " + file_path;
    entry.hash_after = record.sha256_hash;

    custody_entries.push_back(entry);
}

void IntegrityChecker::recordVerification(const String& evidence_id, bool passed) {
    CustodyEntry entry;
    entry.entry_id = generateEntryId();
    entry.evidence_id = evidence_id;
    entry.action = "VERIFIED";
    entry.actor = collector_id;
    entry.location = getCurrentLocation();
    entry.timestamp = millis();
    entry.notes = passed ? "Verification PASSED" : "Verification FAILED";

    custody_entries.push_back(entry);
}

void IntegrityChecker::recordTransfer(const String& evidence_id, const String& recipient) {
    CustodyEntry entry;
    entry.entry_id = generateEntryId();
    entry.evidence_id = evidence_id;
    entry.action = "TRANSFERRED";
    entry.actor = collector_id;
    entry.location = getCurrentLocation();
    entry.timestamp = millis();
    entry.notes = "Transferred to: " + recipient;

    custody_entries.push_back(entry);
}

void IntegrityChecker::recordAnalysis(const String& evidence_id, const String& analyst) {
    CustodyEntry entry;
    entry.entry_id = generateEntryId();
    entry.evidence_id = evidence_id;
    entry.action = "ANALYZED";
    entry.actor = analyst;
    entry.location = getCurrentLocation();
    entry.timestamp = millis();
    entry.notes = "Forensic analysis performed";

    custody_entries.push_back(entry);
}

std::vector<CustodyEntry> IntegrityChecker::getChainOfCustody(const String& evidence_id) {
    std::vector<CustodyEntry> chain;

    for (const auto& entry : custody_entries) {
        if (entry.evidence_id == evidence_id) {
            chain.push_back(entry);
        }
    }

    return chain;
}

std::vector<CustodyEntry> IntegrityChecker::getAllCustodyEntries() {
    return custody_entries;
}

// ===========================
// Evidence Container
// ===========================

bool IntegrityChecker::createContainer(const String& container_id, const String& case_id) {
    EvidenceContainer container;
    container.container_id = container_id;
    container.case_id = case_id;
    container.collector_name = collector_id;
    container.collection_start = millis();
    container.sealed = false;
    container.tampered = false;

    containers[container_id] = container;

    Serial.println("[IntegrityChecker] Created evidence container: " + container_id);

    return true;
}

bool IntegrityChecker::addToContainer(const String& container_id, const String& file_path) {
    if (containers.find(container_id) == containers.end()) {
        return false;
    }

    if (containers[container_id].sealed) {
        Serial.println("[IntegrityChecker] Cannot add to sealed container");
        return false;
    }

    IntegrityRecord record = getIntegrityRecord(file_path);
    if (record.file_path.isEmpty()) {
        // Record doesn't exist, create it
        String evidence_id = generateEvidenceId();
        recordArtifact(file_path, evidence_id);
        record = getIntegrityRecord(file_path);
    }

    containers[container_id].artifacts.push_back(record);

    return true;
}

bool IntegrityChecker::sealContainer(const String& container_id) {
    if (containers.find(container_id) == containers.end()) {
        return false;
    }

    EvidenceContainer& container = containers[container_id];
    container.collection_end = millis();
    container.sealed = true;
    container.container_hash = calculateContainerHash(container);

    Serial.println("[IntegrityChecker] Sealed container: " + container_id);
    Serial.println("  Container hash: " + container.container_hash);

    addCustodyEntry(container_id, "SEALED", collector_id,
                   "Evidence container sealed with " + String(container.artifacts.size()) + " artifacts");

    return true;
}

bool IntegrityChecker::verifyContainer(const String& container_id) {
    if (containers.find(container_id) == containers.end()) {
        return false;
    }

    EvidenceContainer& container = containers[container_id];

    // Recalculate container hash
    String current_hash = calculateContainerHash(container);

    if (compareHashes(current_hash, container.container_hash)) {
        container.tampered = false;
        Serial.println("[IntegrityChecker] Container verification PASSED");
        return true;
    } else {
        container.tampered = true;
        Serial.println("[IntegrityChecker] Container verification FAILED - tampering detected!");
        return false;
    }
}

EvidenceContainer IntegrityChecker::getContainer(const String& container_id) {
    if (containers.find(container_id) != containers.end()) {
        return containers[container_id];
    }
    return EvidenceContainer();
}

// ===========================
// Tamper Detection
// ===========================

bool IntegrityChecker::detectTampering(const String& file_path) {
    ValidationStatus status = validateArtifact(file_path);
    return status == VALIDATION_FAIL;
}

std::vector<String> IntegrityChecker::scanForTamperedFiles() {
    std::vector<String> tampered_files;

    Serial.println("[IntegrityChecker] Scanning for tampered files...");

    for (const auto& pair : integrity_records) {
        if (detectTampering(pair.first)) {
            tampered_files.push_back(pair.first);
            Serial.println("[IntegrityChecker] TAMPERED: " + pair.first);
        }
    }

    return tampered_files;
}

void IntegrityChecker::enableTamperMonitoring(bool enabled) {
    tamper_monitoring_enabled = enabled;
}

// ===========================
// Export Methods
// ===========================

String IntegrityChecker::exportIntegrityRecordsJSON() {
    String json = "{\n  \"records\": [\n";

    size_t count = 0;
    for (const auto& pair : integrity_records) {
        const IntegrityRecord& record = pair.second;

        json += "    {\n";
        json += "      \"file_path\": \"" + record.file_path + "\",\n";
        json += "      \"file_size\": " + String(record.file_size) + ",\n";
        json += "      \"md5\": \"" + record.md5_hash + "\",\n";
        json += "      \"sha1\": \"" + record.sha1_hash + "\",\n";
        json += "      \"sha256\": \"" + record.sha256_hash + "\",\n";
        json += "      \"evidence_id\": \"" + record.evidence_id + "\",\n";
        json += "      \"collector_id\": \"" + record.collector_id + "\",\n";
        json += "      \"timestamp_created\": " + String(record.timestamp_created) + "\n";
        json += "    }";

        if (count < integrity_records.size() - 1) json += ",";
        json += "\n";
        count++;
    }

    json += "  ],\n";
    json += "  \"total_records\": " + String(integrity_records.size()) + "\n";
    json += "}\n";

    return json;
}

String IntegrityChecker::exportChainOfCustodyJSON() {
    String json = "{\n  \"chain_of_custody\": [\n";

    for (size_t i = 0; i < custody_entries.size(); i++) {
        const CustodyEntry& entry = custody_entries[i];

        json += "    {\n";
        json += "      \"entry_id\": \"" + entry.entry_id + "\",\n";
        json += "      \"evidence_id\": \"" + entry.evidence_id + "\",\n";
        json += "      \"action\": \"" + entry.action + "\",\n";
        json += "      \"actor\": \"" + entry.actor + "\",\n";
        json += "      \"location\": \"" + entry.location + "\",\n";
        json += "      \"timestamp\": " + String(entry.timestamp) + ",\n";
        json += "      \"notes\": \"" + entry.notes + "\"\n";
        json += "    }";

        if (i < custody_entries.size() - 1) json += ",";
        json += "\n";
    }

    json += "  ],\n";
    json += "  \"total_entries\": " + String(custody_entries.size()) + "\n";
    json += "}\n";

    return json;
}

// ===========================
// Statistics
// ===========================

uint32_t IntegrityChecker::getValidatedCount() const {
    uint32_t count = 0;
    for (const auto& pair : integrity_records) {
        if (pair.second.status == VALIDATION_PASS) {
            count++;
        }
    }
    return count;
}

uint32_t IntegrityChecker::getFailedCount() const {
    uint32_t count = 0;
    for (const auto& pair : integrity_records) {
        if (pair.second.status == VALIDATION_FAIL) {
            count++;
        }
    }
    return count;
}

// ===========================
// Helper Methods
// ===========================

String IntegrityChecker::bytesToHex(const uint8_t* bytes, size_t length) {
    String hex = "";
    for (size_t i = 0; i < length; i++) {
        if (bytes[i] < 16) hex += "0";
        hex += String(bytes[i], HEX);
    }
    return hex;
}

bool IntegrityChecker::hashFile(const String& file_path, HashAlgorithm algorithm, uint8_t* output) {
    if (!storage) return false;

    File file = storage->openFile(file_path, FILE_READ);
    if (!file) return false;

    // Initialize hash context based on algorithm
    if (algorithm == HASH_MD5) {
        mbedtls_md5_context md5_ctx;
        mbedtls_md5_init(&md5_ctx);
        mbedtls_md5_starts(&md5_ctx);

        uint8_t buffer[512];
        while (file.available()) {
            size_t bytes_read = file.read(buffer, sizeof(buffer));
            mbedtls_md5_update(&md5_ctx, buffer, bytes_read);
        }

        mbedtls_md5_finish(&md5_ctx, output);
        mbedtls_md5_free(&md5_ctx);
    }
    else if (algorithm == HASH_SHA1) {
        mbedtls_sha1_context sha1_ctx;
        mbedtls_sha1_init(&sha1_ctx);
        mbedtls_sha1_starts(&sha1_ctx);

        uint8_t buffer[512];
        while (file.available()) {
            size_t bytes_read = file.read(buffer, sizeof(buffer));
            mbedtls_sha1_update(&sha1_ctx, buffer, bytes_read);
        }

        mbedtls_sha1_finish(&sha1_ctx, output);
        mbedtls_sha1_free(&sha1_ctx);
    }
    else if (algorithm == HASH_SHA256) {
        mbedtls_sha256_context sha256_ctx;
        mbedtls_sha256_init(&sha256_ctx);
        mbedtls_sha256_starts(&sha256_ctx, 0);

        uint8_t buffer[512];
        while (file.available()) {
            size_t bytes_read = file.read(buffer, sizeof(buffer));
            mbedtls_sha256_update(&sha256_ctx, buffer, bytes_read);
        }

        mbedtls_sha256_finish(&sha256_ctx, output);
        mbedtls_sha256_free(&sha256_ctx);
    }

    file.close();
    return true;
}

bool IntegrityChecker::compareHashes(const String& hash1, const String& hash2) {
    return hash1.equalsIgnoreCase(hash2);
}

String IntegrityChecker::generateEntryId() {
    static uint32_t entry_counter = 0;
    return "COC_" + String(millis()) + "_" + String(entry_counter++);
}

String IntegrityChecker::generateEvidenceId() {
    static uint32_t evidence_counter = 0;
    return "EVD_" + String(millis()) + "_" + String(evidence_counter++);
}

String IntegrityChecker::getCurrentLocation() {
    return "FRFD_Device_" + String(ESP.getEfuseMac(), HEX);
}

String IntegrityChecker::calculateContainerHash(const EvidenceContainer& container) {
    // Create a concatenated string of all artifact hashes
    String combined = container.container_id + container.case_id;

    for (const auto& artifact : container.artifacts) {
        combined += artifact.sha256_hash;
    }

    // Calculate SHA256 of the combined string
    return calculateBufferSHA256((const uint8_t*)combined.c_str(), combined.length());
}
