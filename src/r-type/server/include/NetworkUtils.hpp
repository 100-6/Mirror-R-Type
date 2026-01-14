/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** NetworkUtils - Network byte order conversion utilities
*/

#ifndef NETWORK_UTILS_HPP_
#define NETWORK_UTILS_HPP_

#include <cstdint>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <arpa/inet.h>
#endif

namespace rtype::server::netutils {

/**
 * @brief Network byte order utilities
 * Encapsulates htonl, ntohl, htons, ntohs functions
 */
class ByteOrder {
public:
    /**
     * @brief Convert 32-bit value from host to network byte order
     */
    static inline uint32_t host_to_net32(uint32_t value) {
        return htonl(value);
    }

    /**
     * @brief Convert 32-bit value from network to host byte order
     */
    static inline uint32_t net_to_host32(uint32_t value) {
        return ntohl(value);
    }

    /**
     * @brief Convert 16-bit value from host to network byte order
     */
    static inline uint16_t host_to_net16(uint16_t value) {
        return htons(value);
    }

    /**
     * @brief Convert 16-bit value from network to host byte order
     */
    static inline uint16_t net_to_host16(uint16_t value) {
        return ntohs(value);
    }

    /**
     * @brief Convert float from host to network byte order
     * Floats are sent as-is (IEEE 754 is consistent across platforms)
     */
    static inline float host_to_net_float(float value) {
        return value;  // IEEE 754 floats don't need byte swapping
    }

    /**
     * @brief Convert float from network to host byte order
     */
    static inline float net_to_host_float(float value) {
        return value;  // IEEE 754 floats don't need byte swapping
    }
};

/**
 * @brief Memory operation utilities
 * Encapsulates memcpy, memset functions for safer usage
 */
class Memory {
public:
    /**
     * @brief Copy data from source to destination
     * @param dest Destination pointer
     * @param src Source pointer
     * @param size Number of bytes to copy
     */
    template<typename T>
    static inline void copy(void* dest, const T* src, size_t size) {
        std::memcpy(dest, src, size);
    }

    /**
     * @brief Copy struct to destination
     * @param dest Destination pointer
     * @param src Source struct reference
     */
    template<typename T>
    static inline void copy_struct(void* dest, const T& src) {
        std::memcpy(dest, &src, sizeof(T));
    }

    /**
     * @brief Copy data from pointer to struct
     * @param dest Destination struct reference
     * @param src Source pointer
     */
    template<typename T>
    static inline void copy_to_struct(T& dest, const void* src) {
        std::memcpy(&dest, src, sizeof(T));
    }

    /**
     * @brief Set memory to a specific value
     * @param dest Destination pointer
     * @param value Value to set (typically 0)
     * @param size Number of bytes to set
     */
    static inline void set(void* dest, int value, size_t size) {
        std::memset(dest, value, size);
    }

    /**
     * @brief Zero out memory
     * @param dest Destination pointer
     * @param size Number of bytes to zero
     */
    static inline void zero(void* dest, size_t size) {
        std::memset(dest, 0, size);
    }

    /**
     * @brief Zero out a struct
     */
    template<typename T>
    static inline void zero_struct(T& dest) {
        std::memset(&dest, 0, sizeof(T));
    }
};

} // namespace rtype::server::netutils

#endif /* !NETWORK_UTILS_HPP_ */
