#ifndef COMPRESSION_MANAGER_H
#define COMPRESSION_MANAGER_H

#include <Arduino.h>
#include <vector>
#include "storage.h"
#include "zlib.h"

/**
 * @brief Compression Algorithm Types
 */
enum CompressionAlgorithm {
    COMPRESS_NONE,
    COMPRESS_GZIP,
    COMPRESS_DEFLATE,
    COMPRESS_ZLIB
};

/**
 * @brief Compression Level
 */
enum CompressionLevel {
    COMPRESS_LEVEL_NONE = 0,
    COMPRESS_LEVEL_FAST = 1,
    COMPRESS_LEVEL_DEFAULT = 6,
    COMPRESS_LEVEL_BEST = 9
};

/**
 * @brief Compression Statistics
 */
struct CompressionStats {
    String file_path;
    uint32_t original_size;
    uint32_t compressed_size;
    float compression_ratio;
    float space_saved_percent;
    unsigned long compression_time_ms;
    CompressionAlgorithm algorithm;
    CompressionLevel level;
};

/**
 * @brief Compression Report
 */
struct CompressionReport {
    uint32_t total_files_compressed;
    uint64_t total_original_bytes;
    uint64_t total_compressed_bytes;
    uint64_t total_bytes_saved;
    float overall_compression_ratio;
    float overall_space_saved_percent;
    unsigned long total_time_ms;
    std::vector<CompressionStats> file_stats;
};

/**
 * @brief Compression Manager
 *
 * Handles compression and decompression of forensic artifacts
 * Reduces bandwidth and storage requirements for evidence transfer
 */
class CompressionManager {
public:
    CompressionManager();
    ~CompressionManager();

    // Initialization
    void begin(FRFDStorage* storage_ptr);

    // File Compression
    bool compressFile(const String& input_path, const String& output_path);
    bool compressFile(const String& input_path, const String& output_path,
                     CompressionAlgorithm algorithm, CompressionLevel level);
    bool decompressFile(const String& input_path, const String& output_path);

    // Buffer Compression
    bool compressBuffer(const uint8_t* input, size_t input_size,
                       uint8_t* output, size_t* output_size);
    bool compressBuffer(const uint8_t* input, size_t input_size,
                       uint8_t* output, size_t* output_size,
                       CompressionAlgorithm algorithm, CompressionLevel level);
    bool decompressBuffer(const uint8_t* input, size_t input_size,
                         uint8_t* output, size_t* output_size);

    // Batch Operations
    bool compressDirectory(const String& dir_path, const String& output_dir);
    bool compressMultipleFiles(const std::vector<String>& files, const String& output_dir);
    bool decompressMultipleFiles(const std::vector<String>& files, const String& output_dir);

    // Archive Creation
    bool createCompressedArchive(const std::vector<String>& files, const String& archive_path);
    bool extractCompressedArchive(const String& archive_path, const String& extract_dir);

    // Streaming Compression
    void* createCompressionStream(CompressionAlgorithm algorithm, CompressionLevel level);
    bool compressStreamChunk(void* stream, const uint8_t* input, size_t input_size,
                            uint8_t* output, size_t* output_size, bool finish);
    void destroyCompressionStream(void* stream);

    // Statistics
    CompressionStats getLastCompressionStats();
    CompressionReport generateCompressionReport();
    void clearStatistics();

    // Configuration
    void setDefaultAlgorithm(CompressionAlgorithm algorithm);
    void setDefaultLevel(CompressionLevel level);
    void enableCompression(bool enabled) { compression_enabled = enabled; }
    void setMinFileSize(uint32_t size) { min_file_size = size; }
    void setMaxBufferSize(uint32_t size) { max_buffer_size = size; }

    // Utility
    bool isFileCompressed(const String& file_path);
    uint32_t estimateCompressedSize(uint32_t original_size);
    float calculateCompressionRatio(uint32_t original_size, uint32_t compressed_size);
    String getAlgorithmName(CompressionAlgorithm algorithm);

private:
    FRFDStorage* storage;

    CompressionAlgorithm default_algorithm;
    CompressionLevel default_level;
    bool compression_enabled;
    uint32_t min_file_size;
    uint32_t max_buffer_size;

    std::vector<CompressionStats> compression_stats;
    CompressionStats last_stats;

    // Compression helpers
    bool compressWithGzip(const uint8_t* input, size_t input_size,
                         uint8_t* output, size_t* output_size, int level);
    bool compressWithDeflate(const uint8_t* input, size_t input_size,
                            uint8_t* output, size_t* output_size, int level);
    bool decompressWithGzip(const uint8_t* input, size_t input_size,
                           uint8_t* output, size_t* output_size);
    bool decompressWithDeflate(const uint8_t* input, size_t input_size,
                              uint8_t* output, size_t* output_size);

    // File I/O helpers
    bool readFileToBuffer(const String& file_path, uint8_t** buffer, size_t* size);
    bool writeBufferToFile(const String& file_path, const uint8_t* buffer, size_t size);
    void freeBuffer(uint8_t* buffer);

    // Statistics helpers
    void recordCompression(const String& file_path, uint32_t original_size,
                          uint32_t compressed_size, unsigned long time_ms,
                          CompressionAlgorithm algorithm, CompressionLevel level);
};

#endif // COMPRESSION_MANAGER_H
