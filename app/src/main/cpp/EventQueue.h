#ifndef TRIBEX_EVENTQUEUE_H
#define TRIBEX_EVENTQUEUE_H

#include <atomic>
#include <array>
#include <cstdint>

/**
 * Lock-Free Single-Producer Single-Consumer (SPSC) Ring Buffer
 * 
 * Design:
 * - Producer: Control Thread (via JNI)
 * - Consumer: Audio Thread (in render callback)
 * - No locks, no allocations in push/pop
 * - Fixed size (power of 2 for fast modulo)
 * 
 * Thread Safety:
 * - push() can be called from Control Thread only
 * - pop() can be called from Audio Thread only
 * - Index updates are atomic
 */
template<typename T, size_t Capacity>
class LockFreeQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");

public:
    LockFreeQueue() : mWriteIndex(0), mReadIndex(0) {}
    ~LockFreeQueue() = default;

    /**
     * Push item from Control Thread
     * Returns false if queue is full
     * CRITICAL: No allocations!
     */
    bool push(const T& item) {
        const size_t currentWrite = mWriteIndex.load(std::memory_order_relaxed);
        const size_t nextWrite = (currentWrite + 1) & (Capacity - 1);
        
        // Check if queue would overflow
        if (nextWrite == mReadIndex.load(std::memory_order_acquire)) {
            return false; // Queue full
        }
        
        // Write item
        mBuffer[currentWrite] = item;
        
        // Update write index (release ensures item is visible to consumer)
        mWriteIndex.store(nextWrite, std::memory_order_release);
        
        return true;
    }

    /**
     * Pop item from Audio Thread
     * Returns false if queue is empty
     * CRITICAL: No allocations!
     */
    bool pop(T& item) {
        const size_t currentRead = mReadIndex.load(std::memory_order_relaxed);
        
        // Check if queue is empty
        if (currentRead == mWriteIndex.load(std::memory_order_acquire)) {
            return false; // Queue empty
        }
        
        // Read item
        item = mBuffer[currentRead];
        
        // Update read index
        const size_t nextRead = (currentRead + 1) & (Capacity - 1);
        mReadIndex.store(nextRead, std::memory_order_release);
        
        return true;
    }

    /**
     * Clear queue (call from Control Thread only)
     * Resets both indices to 0
     */
    void clear() {
        mReadIndex.store(0, std::memory_order_relaxed);
        mWriteIndex.store(0, std::memory_order_relaxed);
    }

    /**
     * Check if queue is empty
     * Can be called from either thread
     */
    bool isEmpty() const {
        return mReadIndex.load(std::memory_order_acquire) == 
               mWriteIndex.load(std::memory_order_acquire);
    }

    /**
     * Get current size estimate
     * Note: May be slightly inaccurate due to concurrent access
     */
    size_t size() const {
        size_t write = mWriteIndex.load(std::memory_order_acquire);
        size_t read = mReadIndex.load(std::memory_order_acquire);
        return (write - read) & (Capacity - 1);
    }

    /**
     * Get capacity
     */
    static constexpr size_t capacity() {
        return Capacity;
    }

private:
    std::atomic<size_t> mWriteIndex;
    std::atomic<size_t> mReadIndex;
    std::array<T, Capacity> mBuffer;
};

#endif // TRIBEX_EVENTQUEUE_H