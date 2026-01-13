#pragma once

#include <cstdint>
#include <chrono>
#include <mutex>
#include <string>

namespace rtype::protocol {

/**
 * @brief Thread-safe compression statistics tracker
 *
 * Tracks compression metrics across all packets for monitoring and debugging.
 * Statistics include:
 * - Number of packets compressed vs total
 * - Total bytes saved
 * - Average compression ratio
 * - Average compression time
 *
 * Thread-safe for use in multi-threaded server/client environments.
 */
class CompressionStats {
public:
    /**
     * @brief Aggregated compression metrics
     */
    struct Metrics {
        size_t total_packets_sent = 0;          ///< Total packets sent (compressed + uncompressed)
        size_t compressed_packets = 0;          ///< Number of packets actually compressed
        size_t bytes_before_compression = 0;    ///< Total original payload bytes
        size_t bytes_after_compression = 0;     ///< Total bytes after compression (or original if not compressed)
        std::chrono::microseconds total_compression_time{0};  ///< Cumulative compression time

        /**
         * @brief Calculate overall compression ratio
         * @return Ratio (0.0 to 1.0, lower is better compression)
         */
        float get_compression_ratio() const {
            if (bytes_before_compression == 0)
                return 1.0f;
            return static_cast<float>(bytes_after_compression) / static_cast<float>(bytes_before_compression);
        }

        /**
         * @brief Calculate average compression time per compressed packet
         * @return Average time in microseconds
         */
        float get_avg_compression_time_us() const {
            if (compressed_packets == 0)
                return 0.0f;
            return static_cast<float>(total_compression_time.count()) / static_cast<float>(compressed_packets);
        }

        /**
         * @brief Calculate total bytes saved by compression
         * @return Bytes saved
         */
        size_t get_bytes_saved() const {
            if (bytes_after_compression >= bytes_before_compression)
                return 0;
            return bytes_before_compression - bytes_after_compression;
        }

        /**
         * @brief Calculate compression effectiveness percentage
         * @return Percentage of packets that were compressed (0-100)
         */
        float get_compression_rate() const {
            if (total_packets_sent == 0)
                return 0.0f;
            return (static_cast<float>(compressed_packets) / static_cast<float>(total_packets_sent)) * 100.0f;
        }
    };

    /**
     * @brief Record a compression operation
     *
     * @param original_size Original payload size
     * @param final_size Final size (compressed or original if compression not used)
     * @param compression_time Time taken for compression attempt
     * @param was_compressed True if compression was actually applied
     */
    static void record_compression(size_t original_size,
                                    size_t final_size,
                                    std::chrono::microseconds compression_time,
                                    bool was_compressed);

    /**
     * @brief Get current aggregated metrics
     *
     * Thread-safe snapshot of current statistics.
     *
     * @return Copy of current metrics
     */
    static Metrics get_metrics();

    /**
     * @brief Reset all statistics to zero
     *
     * Useful for periodic logging or testing.
     */
    static void reset();

    /**
     * @brief Generate human-readable statistics report
     *
     * @return Formatted string with all metrics
     */
    static std::string get_report();
};

}
