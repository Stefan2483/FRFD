#include "evidence_container.h"
#include <time.h>

EvidenceContainer::EvidenceContainer(FRFDStorage* storage_ptr)
    : storage(storage_ptr),
      container_open(false),
      finalized(false),
      artifact_sequence(0),
      action_sequence(0),
      collection_start_time(0),
      collection_end_time(0) {
}

EvidenceContainer::~EvidenceContainer() {
    if (container_open && !finalized) {
        finalizeContainer();
    }
}

bool EvidenceContainer::createContainer(const String& caseId, const String& responder_name) {
    if (container_open) {
        Serial.println("[EC] Container already open");
        return false;
    }

    case_id = caseId;
    responder = responder_name;
    collection_start_time = millis();

    // Create container path: /cases/CASEID_TIMESTAMP/
    unsigned long timestamp = millis();
    container_path = "/cases/" + case_id + "_" + String(timestamp);

    Serial.print("[EC] Creating container: ");
    Serial.println(container_path);

    // Create directory structure
    if (!createDirectoryStructure()) {
        Serial.println("[EC] Failed to create directory structure");
        return false;
    }

    container_open = true;

    // Log container creation
    logAction("CONTAINER_CREATED", "Evidence container initialized", "SUCCESS");

    return true;
}

bool EvidenceContainer::createDirectoryStructure() {
    if (!storage || !storage->isSDCardAvailable()) {
        return false;
    }

    // Create main container directory
    if (!storage->createDirectory(container_path)) {
        return false;
    }

    // Create subdirectories for different artifact types
    String subdirs[] = {
        "/artifacts",
        "/artifacts/memory",
        "/artifacts/registry",
        "/artifacts/logs",
        "/artifacts/network",
        "/artifacts/filesystem",
        "/artifacts/persistence",
        "/artifacts/other",
        "/metadata",
        "/reports"
    };

    for (const String& subdir : subdirs) {
        String fullPath = container_path + subdir;
        if (!storage->createDirectory(fullPath)) {
            Serial.print("[EC] Failed to create: ");
            Serial.println(fullPath);
            return false;
        }
    }

    return true;
}

String EvidenceContainer::addArtifact(const String& type, const String& filename,
                                     const uint8_t* data, size_t size, bool compress) {
    if (!container_open) {
        Serial.println("[EC] Container not open");
        return "";
    }

    if (finalized) {
        Serial.println("[EC] Container finalized, cannot add artifacts");
        return "";
    }

    // Generate unique artifact ID
    String artifactId = generateArtifactId();

    // Determine storage path based on type
    String artifactDir = container_path + "/artifacts/" + type;
    String storagePath = artifactDir + "/" + filename;

    Serial.print("[EC] Adding artifact: ");
    Serial.print(artifactId);
    Serial.print(" (");
    Serial.print(size);
    Serial.println(" bytes)");

    // Calculate hash before storage
    String hash = calculateSHA256(data, size);

    // Compress if requested and beneficial
    const uint8_t* dataToStore = data;
    size_t sizeToStore = size;
    uint8_t* compressedData = nullptr;
    bool isCompressed = false;

    if (compress && size > 1024) { // Only compress if > 1KB
        size_t compressedSize;
        compressedData = (uint8_t*)malloc(size); // Worst case same size

        if (compressedData && SimpleCompressor::compress(data, size, compressedData, size, &compressedSize)) {
            if (compressedSize < size * 0.9) { // Only use if >10% reduction
                dataToStore = compressedData;
                sizeToStore = compressedSize;
                isCompressed = true;
                storagePath += ".compressed";
                Serial.print("[EC] Compressed: ");
                Serial.print(size);
                Serial.print(" -> ");
                Serial.print(sizeToStore);
                Serial.print(" bytes (");
                Serial.print((float)sizeToStore / size * 100.0, 1);
                Serial.println("%)");
            }
        }
    }

    // Save artifact to SD card
    bool saved = storage->saveArtifact(storagePath, dataToStore, sizeToStore);

    if (compressedData) {
        free(compressedData);
    }

    if (!saved) {
        Serial.println("[EC] Failed to save artifact");
        logAction("ARTIFACT_ADD_FAILED", artifactId + ": " + filename, "FAILED");
        return "";
    }

    // Create metadata
    ArtifactMetadata meta;
    meta.artifact_id = artifactId;
    meta.artifact_type = type;
    meta.filename = filename;
    meta.storage_path = storagePath;
    meta.file_size = sizeToStore;
    meta.original_size = size;
    meta.sha256_hash = hash;
    meta.collected_at = millis();
    meta.collection_method = "HID_AUTO";
    meta.compressed = isCompressed;
    meta.integrity_verified = false; // Will verify later
    meta.error_message = "";

    artifacts.push_back(meta);

    // Save metadata to separate file
    addArtifactMetadata(artifactId, meta);

    logAction("ARTIFACT_ADDED", artifactId + ": " + filename + " (" + String(size) + " bytes)", "SUCCESS");

    return artifactId;
}

bool EvidenceContainer::addArtifactMetadata(const String& artifactId, const ArtifactMetadata& meta) {
    JsonDocument doc;

    doc["artifact_id"] = meta.artifact_id;
    doc["type"] = meta.artifact_type;
    doc["filename"] = meta.filename;
    doc["storage_path"] = meta.storage_path;
    doc["file_size"] = meta.file_size;
    doc["original_size"] = meta.original_size;
    doc["sha256"] = meta.sha256_hash;
    doc["collected_at"] = formatTimestamp(meta.collected_at);
    doc["method"] = meta.collection_method;
    doc["source_path"] = meta.source_path;
    doc["compressed"] = meta.compressed;
    doc["integrity_verified"] = meta.integrity_verified;

    if (meta.error_message.length() > 0) {
        doc["error"] = meta.error_message;
    }

    String metaPath = container_path + "/metadata/" + artifactId + ".json";
    return saveJSON(metaPath, doc);
}

bool EvidenceContainer::verifyArtifactIntegrity(const String& artifactId) {
    // Find artifact
    for (auto& artifact : artifacts) {
        if (artifact.artifact_id == artifactId) {
            // Calculate hash of stored file
            String storedHash = calculateFileSHA256(artifact.storage_path);

            if (storedHash == artifact.sha256_hash) {
                artifact.integrity_verified = true;
                return true;
            } else {
                artifact.integrity_verified = false;
                artifact.error_message = "Hash mismatch";
                validation_errors.push_back(artifactId + ": Hash verification failed");
                return false;
            }
        }
    }

    return false;
}

void EvidenceContainer::setTargetSystemInfo(const TargetSystemInfo& info) {
    target_system = info;
    logAction("SYSTEM_INFO_SET", info.os_name + " / " + info.hostname, "SUCCESS");
}

void EvidenceContainer::logAction(const String& actionType, const String& details, const String& result) {
    CollectionAction action;
    action.timestamp = millis();
    action.action_type = actionType;
    action.details = details;
    action.result = result;

    // Calculate integrity hash
    String actionData = String(action.timestamp) + actionType + details + result + String(action_sequence);
    action.integrity_hash = calculateSHA256((const uint8_t*)actionData.c_str(), actionData.length());

    actions.push_back(action);
    action_sequence++;

    // Log to serial
    Serial.print("[EC] ");
    Serial.print(actionType);
    Serial.print(": ");
    Serial.println(details);
}

bool EvidenceContainer::finalizeContainer() {
    if (!container_open) {
        return false;
    }

    if (finalized) {
        return true;
    }

    collection_end_time = millis();

    Serial.println("[EC] Finalizing container...");

    // Verify all artifacts
    bool allVerified = verifyAllArtifacts();

    // Generate manifest
    generateManifest();

    // Generate chain of custody
    generateChainOfCustody();

    // Generate master hash file
    String hashFilePath = container_path + "/hashes.sha256";
    String hashContent = "# SHA-256 Hashes - Case: " + case_id + "\n";
    hashContent += "# Generated: " + formatTimestamp(collection_end_time) + "\n\n";

    for (const auto& artifact : artifacts) {
        hashContent += artifact.sha256_hash + "  " + artifact.filename + "\n";
    }

    storage->saveArtifact(hashFilePath, (const uint8_t*)hashContent.c_str(), hashContent.length());

    finalized = true;
    logAction("CONTAINER_FINALIZED", "Evidence container sealed", allVerified ? "SUCCESS" : "WARNINGS");

    Serial.print("[EC] Container finalized: ");
    Serial.print(artifacts.size());
    Serial.println(" artifacts");

    return true;
}

bool EvidenceContainer::generateManifest() {
    JsonDocument doc;

    doc["case_id"] = case_id;
    doc["responder"] = responder;
    doc["container_version"] = "1.0";
    doc["created_at"] = formatTimestamp(collection_start_time);
    doc["finalized_at"] = formatTimestamp(collection_end_time);
    doc["duration_ms"] = collection_end_time - collection_start_time;

    // Device info
    doc["device"]["device_id"] = "FRFD-001"; // Should get from config
    doc["device"]["firmware_version"] = "0.5.0";

    // Target system
    if (target_system.hostname.length() > 0) {
        doc["target"]["os"] = target_system.os_name;
        doc["target"]["version"] = target_system.os_version;
        doc["target"]["hostname"] = target_system.hostname;
        doc["target"]["ip_address"] = target_system.ip_address;
        doc["target"]["is_admin"] = target_system.is_admin;
    }

    // Statistics
    doc["statistics"]["artifact_count"] = artifacts.size();
    doc["statistics"]["total_size"] = getTotalSize();
    doc["statistics"]["compressed_size"] = getCompressedSize();
    doc["statistics"]["compression_ratio"] = getCompressionRatio();
    doc["statistics"]["action_count"] = actions.size();

    // Artifacts summary
    JsonArray artifactArray = doc["artifacts"].to<JsonArray>();
    for (const auto& artifact : artifacts) {
        JsonObject obj = artifactArray.add<JsonObject>();
        obj["id"] = artifact.artifact_id;
        obj["type"] = artifact.artifact_type;
        obj["filename"] = artifact.filename;
        obj["size"] = artifact.file_size;
        obj["sha256"] = artifact.sha256_hash;
        obj["verified"] = artifact.integrity_verified;
    }

    String manifestPath = container_path + "/manifest.json";
    return saveJSON(manifestPath, doc);
}

bool EvidenceContainer::generateChainOfCustody() {
    JsonDocument doc;

    doc["case_id"] = case_id;
    doc["collection_start"] = formatTimestamp(collection_start_time);
    doc["collection_end"] = formatTimestamp(collection_end_time);

    // Collector info
    doc["collector"]["device_id"] = "FRFD-001";
    doc["collector"]["firmware_version"] = "0.5.0";
    doc["collector"]["operator"] = responder;

    // Target system
    if (target_system.hostname.length() > 0) {
        doc["target_system"]["os"] = target_system.os_name + " " + target_system.os_version;
        doc["target_system"]["hostname"] = target_system.hostname;
        doc["target_system"]["ip_address"] = target_system.ip_address;
        doc["target_system"]["timestamp"] = formatTimestamp(target_system.system_time);
    }

    // Actions (full audit trail)
    JsonArray actionArray = doc["actions"].to<JsonArray>();
    for (const auto& action : actions) {
        JsonObject obj = actionArray.add<JsonObject>();
        obj["timestamp"] = formatTimestamp(action.timestamp);
        obj["action"] = action.action_type;
        obj["details"] = action.details;
        obj["result"] = action.result;
        obj["integrity_hash"] = action.integrity_hash;
    }

    // Artifacts
    JsonArray artifactArray = doc["artifacts"].to<JsonArray>();
    for (const auto& artifact : artifacts) {
        JsonObject obj = artifactArray.add<JsonObject>();
        obj["id"] = artifact.artifact_id;
        obj["type"] = artifact.artifact_type;
        obj["filename"] = artifact.filename;
        obj["size"] = artifact.file_size;
        obj["sha256"] = artifact.sha256_hash;
        obj["collected_at"] = formatTimestamp(artifact.collected_at);
        obj["method"] = artifact.collection_method;
        obj["integrity_verified"] = artifact.integrity_verified;
    }

    // Integrity
    doc["integrity"]["verified"] = verifyAllArtifacts();
    doc["integrity"]["manifest_hash"] = ""; // Would calculate from manifest.json
    doc["integrity"]["total_artifacts"] = artifacts.size();
    doc["integrity"]["verification_errors"] = validation_errors.size();

    String cocPath = container_path + "/chain_of_custody.json";
    return saveJSON(cocPath, doc);
}

bool EvidenceContainer::verifyAllArtifacts() {
    bool allValid = true;

    for (const auto& artifact : artifacts) {
        if (!const_cast<EvidenceContainer*>(this)->verifyArtifactIntegrity(artifact.artifact_id)) {
            allValid = false;
        }
    }

    return allValid;
}

uint32_t EvidenceContainer::getTotalSize() const {
    uint32_t total = 0;
    for (const auto& artifact : artifacts) {
        total += artifact.original_size;
    }
    return total;
}

uint32_t EvidenceContainer::getCompressedSize() const {
    uint32_t total = 0;
    for (const auto& artifact : artifacts) {
        total += artifact.file_size;
    }
    return total;
}

float EvidenceContainer::getCompressionRatio() const {
    uint32_t original = getTotalSize();
    uint32_t compressed = getCompressedSize();

    if (original == 0) return 1.0;

    return (float)compressed / (float)original;
}

unsigned long EvidenceContainer::getCollectionDuration() const {
    if (collection_end_time == 0) {
        return millis() - collection_start_time;
    }
    return collection_end_time - collection_start_time;
}

// ============================================================================
// Helper Methods
// ============================================================================

String EvidenceContainer::generateArtifactId() {
    artifact_sequence++;
    char buffer[32];
    sprintf(buffer, "artifact_%03d", artifact_sequence);
    return String(buffer);
}

String EvidenceContainer::calculateSHA256(const uint8_t* data, size_t length) {
    unsigned char hash[32];
    mbedtls_sha256_context ctx;

    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, data, length);
    mbedtls_sha256_finish(&ctx, hash);
    mbedtls_sha256_free(&ctx);

    String hash_str = "";
    for (int i = 0; i < 32; i++) {
        char hex[3];
        sprintf(hex, "%02x", hash[i]);
        hash_str += hex;
    }

    return hash_str;
}

String EvidenceContainer::calculateFileSHA256(const String& filepath) {
    // Simplified - would need to read file in chunks for large files
    // For now, return empty if file operations not available
    return "";
}

String EvidenceContainer::formatTimestamp(unsigned long timestamp) {
    // Format as relative time or ISO 8601 if RTC available
    unsigned long seconds = timestamp / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;

    char buffer[32];
    sprintf(buffer, "T+%02lu:%02lu:%02lu", hours, minutes % 60, seconds % 60);
    return String(buffer);
}

bool EvidenceContainer::saveJSON(const String& filename, const JsonDocument& doc) {
    String jsonString;
    serializeJsonPretty(doc, jsonString);

    return storage->saveArtifact(filename, (const uint8_t*)jsonString.c_str(), jsonString.length());
}

// ============================================================================
// Simple Compression (RLE-based)
// ============================================================================

bool SimpleCompressor::compress(const uint8_t* input, size_t inputLen,
                                uint8_t* output, size_t maxOutputLen, size_t* outputLen) {
    // Simple Run-Length Encoding
    size_t outPos = 0;
    size_t inPos = 0;

    while (inPos < inputLen && outPos < maxOutputLen - 2) {
        uint8_t current = input[inPos];
        uint8_t count = 1;

        // Count consecutive identical bytes
        while (inPos + count < inputLen && input[inPos + count] == current && count < 255) {
            count++;
        }

        if (count >= 3) {
            // Use RLE for runs of 3 or more
            output[outPos++] = 0xFF; // RLE marker
            output[outPos++] = count;
            output[outPos++] = current;
            inPos += count;
        } else {
            // Copy literal
            output[outPos++] = current;
            inPos++;
        }
    }

    *outputLen = outPos;
    return (outPos < inputLen); // Only successful if we achieved compression
}

bool SimpleCompressor::decompress(const uint8_t* input, size_t inputLen,
                                  uint8_t* output, size_t maxOutputLen, size_t* outputLen) {
    size_t outPos = 0;
    size_t inPos = 0;

    while (inPos < inputLen && outPos < maxOutputLen) {
        if (input[inPos] == 0xFF && inPos + 2 < inputLen) {
            // RLE encoded
            uint8_t count = input[inPos + 1];
            uint8_t value = input[inPos + 2];

            for (uint8_t i = 0; i < count && outPos < maxOutputLen; i++) {
                output[outPos++] = value;
            }

            inPos += 3;
        } else {
            // Literal byte
            output[outPos++] = input[inPos++];
        }
    }

    *outputLen = outPos;
    return true;
}
