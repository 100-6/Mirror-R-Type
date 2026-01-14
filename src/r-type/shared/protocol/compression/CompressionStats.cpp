#include "CompressionStats.hpp"
#include <sstream>
#include <iomanip>
#include <memory>

namespace rtype::protocol {

// Use function-local static to avoid DLL unload issues on Windows
// This ensures proper initialization order and cleanup
static std::mutex& get_stats_mutex_impl() {
    static std::mutex mutex;
    return mutex;
}

static CompressionStats::Metrics& get_metrics_impl() {
    static CompressionStats::Metrics metrics;
    return metrics;
}

void CompressionStats::record_compression(size_t original_size,
                                           size_t final_size,
                                           std::chrono::microseconds compression_time,
                                           bool was_compressed) {
    std::lock_guard<std::mutex> lock(get_stats_mutex_impl());
    auto& m = get_metrics_impl();

    m.total_packets_sent++;
    m.bytes_before_compression += original_size;
    m.bytes_after_compression += final_size;

    if (was_compressed) {
        m.compressed_packets++;
        m.total_compression_time += compression_time;
    }
}

CompressionStats::Metrics CompressionStats::get_metrics() {
    std::lock_guard<std::mutex> lock(get_stats_mutex_impl());
    return get_metrics_impl();
}

void CompressionStats::reset() {
    std::lock_guard<std::mutex> lock(get_stats_mutex_impl());
    get_metrics_impl() = Metrics();
}

std::string CompressionStats::get_report() {
    Metrics m = get_metrics_impl();
    std::ostringstream oss;

    oss << std::fixed << std::setprecision(2);
    oss << "[Compression Stats]\n";
    oss << "  Total packets sent: " << m.total_packets_sent << "\n";
    oss << "  Compressed packets: " << m.compressed_packets
        << " (" << m.get_compression_rate() << "%)\n";
    oss << "  Bytes before: " << m.bytes_before_compression
        << " (" << (m.bytes_before_compression / 1024) << " KB)\n";
    oss << "  Bytes after:  " << m.bytes_after_compression
        << " (" << (m.bytes_after_compression / 1024) << " KB)\n";
    oss << "  Bytes saved:  " << m.get_bytes_saved()
        << " (" << (m.get_bytes_saved() / 1024) << " KB)\n";
    oss << "  Compression ratio: " << (m.get_compression_ratio() * 100.0f) << "%\n";
    oss << "  Avg compression time: " << m.get_avg_compression_time_us() << " µs\n";
    return oss.str();
}

}