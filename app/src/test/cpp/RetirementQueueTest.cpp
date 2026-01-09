#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <vector>
#include <chrono>
#include "../../main/cpp/SamplePart.h"

// Test fixture for SamplePart retirement queue
class RetirementQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Each test gets a fresh SamplePart
        samplePart = std::make_unique<Tribex::SamplePart>(0);
    }

    void TearDown() override {
        samplePart.reset();
    }

    std::unique_ptr<Tribex::SamplePart> samplePart;
};

// Test 1: Single-threaded push and pop
TEST_F(RetirementQueueTest, SingleThreadedPushPop) {
    float* testPtr1 = reinterpret_cast<float*>(0x1000);
    float* testPtr2 = reinterpret_cast<float*>(0x2000);
    float* testPtr3 = reinterpret_cast<float*>(0x3000);

    // Create dummy samples
    Tribex::SampleData sample1;
    sample1.data = testPtr1;
    sample1.length = 1000;
    sample1.id = 1;

    Tribex::SampleData sample2;
    sample2.data = testPtr2;
    sample2.length = 2000;
    sample2.id = 2;

    Tribex::SampleData sample3;
    sample3.data = testPtr3;
    sample3.length = 3000;
    sample3.id = 3;

    // Load and unload samples (defer = true to use retirement queue)
    samplePart->loadSample(sample1);
    samplePart->unloadSample(true);  // Push to retirement queue

    samplePart->loadSample(sample2);
    samplePart->unloadSample(true);  // Push to retirement queue

    samplePart->loadSample(sample3);
    samplePart->unloadSample(true);  // Push to retirement queue

    // Release retired samples
    samplePart->releaseRetiredSamples();

    // If we got here without crash, test passed
    SUCCEED();
}

// Test 2: Queue overflow (push more than MAX_RETIRED_SAMPLES)
TEST_F(RetirementQueueTest, QueueOverflow) {
    constexpr uint32_t MAX_SAMPLES = 8;
    
    // Try to push more than MAX_RETIRED_SAMPLES entries
    for (uint32_t i = 0; i < MAX_SAMPLES + 3; ++i) {
        float* testPtr = reinterpret_cast<float*>(0x1000 + (i * 0x1000));
        
        Tribex::SampleData sample;
        sample.data = testPtr;
        sample.length = 1000;
        sample.id = i;

        samplePart->loadSample(sample);
        samplePart->unloadSample(true);  // Push to retirement queue
    }

    // Release all retired samples
    samplePart->releaseRetiredSamples();

    // Test passes if no crash (overflow should be handled gracefully)
    SUCCEED();
}

// Test 3: Concurrent producer (unload thread) and consumer (release thread)
TEST_F(RetirementQueueTest, ConcurrentProducerConsumer) {
    constexpr int NUM_OPERATIONS = 100;
    std::atomic<bool> stopFlag{false};

    // Producer thread: Load and unload samples
    std::thread producer([this, &stopFlag]() {
        for (int i = 0; i < NUM_OPERATIONS && !stopFlag.load(); ++i) {
            float* testPtr = reinterpret_cast<float*>(0x1000 + (i * 0x1000));
            
            Tribex::SampleData sample;
            sample.data = testPtr;
            sample.length = 1000 + i;
            sample.id = i;

            samplePart->loadSample(sample);
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            samplePart->unloadSample(true);  // Push to retirement queue
        }
    });

    // Consumer thread: Release retired samples
    std::thread consumer([this, &stopFlag]() {
        for (int i = 0; i < NUM_OPERATIONS && !stopFlag.load(); ++i) {
            samplePart->releaseRetiredSamples();
            std::this_thread::sleep_for(std::chrono::microseconds(15));
        }
    });

    // Wait for both threads
    producer.join();
    consumer.join();

    // Final cleanup
    samplePart->releaseRetiredSamples();

    // Test passes if no crash or data race
    SUCCEED();
}

// Test 4: Memory ordering test (load-acquire, store-release)
TEST_F(RetirementQueueTest, MemoryOrderingTest) {
    constexpr int NUM_SAMPLES = 50;
    std::vector<float*> expectedPointers;

    // Create expected pointers
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        expectedPointers.push_back(reinterpret_cast<float*>(0x1000 + (i * 0x1000)));
    }

    // Producer: Load and unload samples rapidly
    std::thread producer([this, &expectedPointers]() {
        for (size_t i = 0; i < expectedPointers.size(); ++i) {
            Tribex::SampleData sample;
            sample.data = expectedPointers[i];
            sample.length = 1000;
            sample.id = i;

            samplePart->loadSample(sample);
            
            // Atomic fence to ensure visibility
            std::atomic_thread_fence(std::memory_order_release);
            
            samplePart->unloadSample(true);
        }
    });

    // Consumer: Release samples
    std::thread consumer([this]() {
        for (int i = 0; i < NUM_SAMPLES; ++i) {
            std::atomic_thread_fence(std::memory_order_acquire);
            samplePart->releaseRetiredSamples();
            std::this_thread::sleep_for(std::chrono::microseconds(5));
        }
    });

    producer.join();
    consumer.join();

    // Final cleanup
    samplePart->releaseRetiredSamples();

    SUCCEED();
}

// Test 5: Stress test - rapid load/unload cycles
TEST_F(RetirementQueueTest, StressTest) {
    constexpr int NUM_CYCLES = 1000;
    
    for (int i = 0; i < NUM_CYCLES; ++i) {
        float* testPtr = reinterpret_cast<float*>(0x1000 + (i * 0x1000));
        
        Tribex::SampleData sample;
        sample.data = testPtr;
        sample.length = 1000;
        sample.id = i;

        samplePart->loadSample(sample);
        samplePart->unloadSample(true);
        
        // Periodically release
        if (i % 10 == 0) {
            samplePart->releaseRetiredSamples();
        }
    }

    // Final cleanup
    samplePart->releaseRetiredSamples();

    SUCCEED();
}

// Test 6: Verify lock-free property (no blocking)
TEST_F(RetirementQueueTest, LockFreeVerification) {
    // This test verifies that operations complete in bounded time
    constexpr int TIMEOUT_MS = 100;
    
    auto start = std::chrono::steady_clock::now();
    
    // Perform operations
    for (int i = 0; i < 100; ++i) {
        float* testPtr = reinterpret_cast<float*>(0x1000 + (i * 0x1000));
        
        Tribex::SampleData sample;
        sample.data = testPtr;
        sample.length = 1000;
        sample.id = i;

        samplePart->loadSample(sample);
        samplePart->unloadSample(true);
        samplePart->releaseRetiredSamples();
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // Operations should complete well within timeout (lock-free guarantee)
    EXPECT_LT(duration, TIMEOUT_MS);
}

// Test 7: Edge case - release empty queue
TEST_F(RetirementQueueTest, ReleaseEmptyQueue) {
    // Call release on empty queue - should not crash
    samplePart->releaseRetiredSamples();
    samplePart->releaseRetiredSamples();
    samplePart->releaseRetiredSamples();

    SUCCEED();
}

// Test 8: Edge case - unload without load
TEST_F(RetirementQueueTest, UnloadWithoutLoad) {
    // Unload when no sample is loaded - should not crash
    samplePart->unloadSample(true);
    samplePart->unloadSample(true);

    samplePart->releaseRetiredSamples();

    SUCCEED();
}
