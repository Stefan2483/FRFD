#include "compression_manager.h"

CompressionManager::CompressionManager()
    : storage(nullptr),
      default_algorithm(COMPRESS_GZIP),
      default_level(COMPRESS_LEVEL_DEFAULT),
      compression_enabled(true),
      min_file_size(1024),      // 1KB minimum
      max_buffer_size(524288) { // 512KB maximum
}

CompressionManager::~CompressionManager() {
}

void CompressionManager::begin(FRFDStorage* storage_ptr) {
    storage = storage_ptr;
    Serial.println("[CompressionManager] Initialized");
    Serial.println("[CompressionManager] Default algorithm: GZIP");
    Serial.println("[CompressionManager] Default level: " + String(default_level));
}

// ===========================
// File Compression
// ===========================

bool CompressionManager::compressFile(const String& input_path, const String& output_path) {
    return compressFile(input_path, output_path, default_algorithm, default_level);
}

bool CompressionManager::compressFile(const String& input_path, const String& output_path,
                                     CompressionAlgorithm algorithm, CompressionLevel level) {
    if (!storage || !compression_enabled) return false;

    unsigned long start_time = millis();

    Serial.println("[CompressionManager] Compressing: " + input_path);

    // Read input file
    uint8_t* input_buffer = nullptr;
    size_t input_size = 0;

    if (!readFileToBuffer(input_path, &input_buffer, &input_size)) {
        Serial.println("[CompressionManager] Failed to read input file");
        return false;
    }

    // Check minimum file size
    if (input_size < min_file_size) {
        Serial.println("[CompressionManager] File too small for compression, copying instead");
        bool result = writeBufferToFile(output_path, input_buffer, input_size);
        freeBuffer(input_buffer);
        return result;
    }

    // Allocate output buffer (worst case: same size as input + header)
    size_t max_output_size = input_size + 1024;
    uint8_t* output_buffer = (uint8_t*)malloc(max_output_size);
    if (!output_buffer) {
        Serial.println("[CompressionManager] Failed to allocate output buffer");
        freeBuffer(input_buffer);
        return false;
    }

    // Compress
    size_t output_size = max_output_size;
    bool success = compressBuffer(input_buffer, input_size, output_buffer, &output_size,
                                  algorithm, level);

    if (success) {
        // Write compressed data
        success = writeBufferToFile(output_path, output_buffer, output_size);

        if (success) {
            unsigned long compression_time = millis() - start_time;
            recordCompression(input_path, input_size, output_size, compression_time, algorithm, level);

            float ratio = calculateCompressionRatio(input_size, output_size);
            Serial.println("[CompressionManager] Compressed: " + String(input_size) +
                          " -> " + String(output_size) + " bytes (" +
                          String(ratio, 2) + "x, " + String(compression_time) + "ms)");
        }
    }

    freeBuffer(input_buffer);
    free(output_buffer);

    return success;
}

bool CompressionManager::decompressFile(const String& input_path, const String& output_path) {
    if (!storage) return false;

    Serial.println("[CompressionManager] Decompressing: " + input_path);

    // Read compressed file
    uint8_t* input_buffer = nullptr;
    size_t input_size = 0;

    if (!readFileToBuffer(input_path, &input_buffer, &input_size)) {
        Serial.println("[CompressionManager] Failed to read compressed file");
        return false;
    }

    // Allocate output buffer (estimate 3x compressed size)
    size_t max_output_size = input_size * 3;
    uint8_t* output_buffer = (uint8_t*)malloc(max_output_size);
    if (!output_buffer) {
        Serial.println("[CompressionManager] Failed to allocate output buffer");
        freeBuffer(input_buffer);
        return false;
    }

    // Decompress
    size_t output_size = max_output_size;
    bool success = decompressBuffer(input_buffer, input_size, output_buffer, &output_size);

    if (success) {
        success = writeBufferToFile(output_path, output_buffer, output_size);
        Serial.println("[CompressionManager] Decompressed: " + String(input_size) +
                      " -> " + String(output_size) + " bytes");
    }

    freeBuffer(input_buffer);
    free(output_buffer);

    return success;
}

// ===========================
// Buffer Compression
// ===========================

bool CompressionManager::compressBuffer(const uint8_t* input, size_t input_size,
                                       uint8_t* output, size_t* output_size) {
    return compressBuffer(input, input_size, output, output_size,
                         default_algorithm, default_level);
}

bool CompressionManager::compressBuffer(const uint8_t* input, size_t input_size,
                                       uint8_t* output, size_t* output_size,
                                       CompressionAlgorithm algorithm, CompressionLevel level) {
    if (!input || !output || !output_size) return false;

    switch (algorithm) {
        case COMPRESS_GZIP:
            return compressWithGzip(input, input_size, output, output_size, level);

        case COMPRESS_DEFLATE:
        case COMPRESS_ZLIB:
            return compressWithDeflate(input, input_size, output, output_size, level);

        case COMPRESS_NONE:
            if (*output_size >= input_size) {
                memcpy(output, input, input_size);
                *output_size = input_size;
                return true;
            }
            return false;

        default:
            return false;
    }
}

bool CompressionManager::decompressBuffer(const uint8_t* input, size_t input_size,
                                         uint8_t* output, size_t* output_size) {
    if (!input || !output || !output_size) return false;

    // Try gzip first (most common)
    if (decompressWithGzip(input, input_size, output, output_size)) {
        return true;
    }

    // Try deflate
    if (decompressWithDeflate(input, input_size, output, output_size)) {
        return true;
    }

    return false;
}

// ===========================
// Batch Operations
// ===========================

bool CompressionManager::compressDirectory(const String& dir_path, const String& output_dir) {
    if (!storage) return false;

    // This would require directory traversal implementation
    // For now, return false as placeholder
    Serial.println("[CompressionManager] Directory compression not yet implemented");
    return false;
}

bool CompressionManager::compressMultipleFiles(const std::vector<String>& files, const String& output_dir) {
    uint32_t successful = 0;
    uint32_t failed = 0;

    for (const auto& file : files) {
        String output_file = output_dir + "/" + file + ".gz";
        if (compressFile(file, output_file)) {
            successful++;
        } else {
            failed++;
        }
    }

    Serial.println("[CompressionManager] Batch compression: " + String(successful) +
                   " succeeded, " + String(failed) + " failed");

    return failed == 0;
}

// ===========================
// Statistics
// ===========================

CompressionStats CompressionManager::getLastCompressionStats() {
    return last_stats;
}

CompressionReport CompressionManager::generateCompressionReport() {
    CompressionReport report;
    report.total_files_compressed = compression_stats.size();
    report.total_original_bytes = 0;
    report.total_compressed_bytes = 0;
    report.total_bytes_saved = 0;
    report.total_time_ms = 0;

    for (const auto& stats : compression_stats) {
        report.total_original_bytes += stats.original_size;
        report.total_compressed_bytes += stats.compressed_size;
        report.total_time_ms += stats.compression_time_ms;
        report.file_stats.push_back(stats);
    }

    report.total_bytes_saved = report.total_original_bytes - report.total_compressed_bytes;
    report.overall_compression_ratio =
        calculateCompressionRatio(report.total_original_bytes, report.total_compressed_bytes);
    report.overall_space_saved_percent =
        (float)report.total_bytes_saved / report.total_original_bytes * 100.0;

    return report;
}

void CompressionManager::clearStatistics() {
    compression_stats.clear();
}

// ===========================
// Configuration
// ===========================

void CompressionManager::setDefaultAlgorithm(CompressionAlgorithm algorithm) {
    default_algorithm = algorithm;
}

void CompressionManager::setDefaultLevel(CompressionLevel level) {
    default_level = level;
}

// ===========================
// Utility
// ===========================

bool CompressionManager::isFileCompressed(const String& file_path) {
    if (!storage) return false;

    // Check file magic bytes
    File file = storage->openFile(file_path, FILE_READ);
    if (!file || file.size() < 2) {
        if (file) file.close();
        return false;
    }

    uint8_t magic[2];
    file.read(magic, 2);
    file.close();

    // Gzip magic: 0x1f 0x8b
    if (magic[0] == 0x1f && magic[1] == 0x8b) {
        return true;
    }

    return false;
}

uint32_t CompressionManager::estimateCompressedSize(uint32_t original_size) {
    // Conservative estimate: 60% of original size
    return (uint32_t)(original_size * 0.6);
}

float CompressionManager::calculateCompressionRatio(uint32_t original_size, uint32_t compressed_size) {
    if (compressed_size == 0) return 0.0;
    return (float)original_size / compressed_size;
}

String CompressionManager::getAlgorithmName(CompressionAlgorithm algorithm) {
    switch (algorithm) {
        case COMPRESS_NONE: return "None";
        case COMPRESS_GZIP: return "GZIP";
        case COMPRESS_DEFLATE: return "DEFLATE";
        case COMPRESS_ZLIB: return "ZLIB";
        default: return "Unknown";
    }
}

// ===========================
// Compression Helpers
// ===========================

bool CompressionManager::compressWithGzip(const uint8_t* input, size_t input_size,
                                         uint8_t* output, size_t* output_size, int level) {
    z_stream stream;
    memset(&stream, 0, sizeof(stream));

    // Initialize for gzip (windowBits + 16 for gzip header)
    int ret = deflateInit2(&stream, level, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK) {
        Serial.println("[CompressionManager] deflateInit2 failed: " + String(ret));
        return false;
    }

    stream.next_in = (Bytef*)input;
    stream.avail_in = input_size;
    stream.next_out = (Bytef*)output;
    stream.avail_out = *output_size;

    ret = deflate(&stream, Z_FINISH);
    if (ret != Z_STREAM_END) {
        Serial.println("[CompressionManager] deflate failed: " + String(ret));
        deflateEnd(&stream);
        return false;
    }

    *output_size = stream.total_out;
    deflateEnd(&stream);

    return true;
}

bool CompressionManager::compressWithDeflate(const uint8_t* input, size_t input_size,
                                            uint8_t* output, size_t* output_size, int level) {
    z_stream stream;
    memset(&stream, 0, sizeof(stream));

    // Initialize for deflate
    int ret = deflateInit(&stream, level);
    if (ret != Z_OK) {
        return false;
    }

    stream.next_in = (Bytef*)input;
    stream.avail_in = input_size;
    stream.next_out = (Bytef*)output;
    stream.avail_out = *output_size;

    ret = deflate(&stream, Z_FINISH);
    if (ret != Z_STREAM_END) {
        deflateEnd(&stream);
        return false;
    }

    *output_size = stream.total_out;
    deflateEnd(&stream);

    return true;
}

bool CompressionManager::decompressWithGzip(const uint8_t* input, size_t input_size,
                                           uint8_t* output, size_t* output_size) {
    z_stream stream;
    memset(&stream, 0, sizeof(stream));

    // Initialize for gzip (windowBits + 16 for gzip header)
    int ret = inflateInit2(&stream, 15 + 16);
    if (ret != Z_OK) {
        return false;
    }

    stream.next_in = (Bytef*)input;
    stream.avail_in = input_size;
    stream.next_out = (Bytef*)output;
    stream.avail_out = *output_size;

    ret = inflate(&stream, Z_FINISH);
    if (ret != Z_STREAM_END && ret != Z_OK) {
        inflateEnd(&stream);
        return false;
    }

    *output_size = stream.total_out;
    inflateEnd(&stream);

    return true;
}

bool CompressionManager::decompressWithDeflate(const uint8_t* input, size_t input_size,
                                              uint8_t* output, size_t* output_size) {
    z_stream stream;
    memset(&stream, 0, sizeof(stream));

    // Initialize for deflate
    int ret = inflateInit(&stream);
    if (ret != Z_OK) {
        return false;
    }

    stream.next_in = (Bytef*)input;
    stream.avail_in = input_size;
    stream.next_out = (Bytef*)output;
    stream.avail_out = *output_size;

    ret = inflate(&stream, Z_FINISH);
    if (ret != Z_STREAM_END && ret != Z_OK) {
        inflateEnd(&stream);
        return false;
    }

    *output_size = stream.total_out;
    inflateEnd(&stream);

    return true;
}

// ===========================
// File I/O Helpers
// ===========================

bool CompressionManager::readFileToBuffer(const String& file_path, uint8_t** buffer, size_t* size) {
    if (!storage) return false;

    File file = storage->openFile(file_path, FILE_READ);
    if (!file) return false;

    *size = file.size();
    if (*size > max_buffer_size) {
        Serial.println("[CompressionManager] File too large: " + String(*size) + " bytes");
        file.close();
        return false;
    }

    *buffer = (uint8_t*)malloc(*size);
    if (!*buffer) {
        Serial.println("[CompressionManager] Failed to allocate buffer");
        file.close();
        return false;
    }

    size_t bytes_read = file.read(*buffer, *size);
    file.close();

    if (bytes_read != *size) {
        free(*buffer);
        *buffer = nullptr;
        return false;
    }

    return true;
}

bool CompressionManager::writeBufferToFile(const String& file_path, const uint8_t* buffer, size_t size) {
    if (!storage) return false;

    File file = storage->openFile(file_path, FILE_WRITE);
    if (!file) {
        Serial.println("[CompressionManager] Failed to open output file");
        return false;
    }

    size_t bytes_written = file.write(buffer, size);
    file.close();

    return bytes_written == size;
}

void CompressionManager::freeBuffer(uint8_t* buffer) {
    if (buffer) {
        free(buffer);
    }
}

// ===========================
// Statistics Helpers
// ===========================

void CompressionManager::recordCompression(const String& file_path, uint32_t original_size,
                                          uint32_t compressed_size, unsigned long time_ms,
                                          CompressionAlgorithm algorithm, CompressionLevel level) {
    CompressionStats stats;
    stats.file_path = file_path;
    stats.original_size = original_size;
    stats.compressed_size = compressed_size;
    stats.compression_ratio = calculateCompressionRatio(original_size, compressed_size);
    stats.space_saved_percent = (float)(original_size - compressed_size) / original_size * 100.0;
    stats.compression_time_ms = time_ms;
    stats.algorithm = algorithm;
    stats.level = level;

    compression_stats.push_back(stats);
    last_stats = stats;
}
