/**
 * @file test_circular_buffer.cpp
 * @brief Unit tests for circular buffer implementation
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstring>
#include <vector>
#include <thread>
#include <atomic>

// Include the C header
extern "C" {
#include "../../app/src/utils/circular_buffer.c"  // Include implementation directly for testing
}

using ::testing::ElementsAre;
using ::testing::Each;

class CircularBufferTest : public ::testing::Test {
protected:
    static constexpr size_t BUFFER_SIZE = 256;
    uint8_t buffer[BUFFER_SIZE];
    circular_buffer_t cb;

    void SetUp() override {
        memset(buffer, 0, BUFFER_SIZE);
        circular_buffer_init(&cb, buffer, BUFFER_SIZE);
    }

    void TearDown() override {
        // Clean up if needed
    }
};

TEST_F(CircularBufferTest, InitializationTest) {
    EXPECT_EQ(cb.buffer, buffer);
    EXPECT_EQ(cb.size, BUFFER_SIZE);
    EXPECT_EQ(cb.head, 0u);
    EXPECT_EQ(cb.tail, 0u);
    EXPECT_TRUE(circular_buffer_is_empty(&cb));
    EXPECT_FALSE(circular_buffer_is_full(&cb));
}

TEST_F(CircularBufferTest, InitializationWithNullPointers) {
    circular_buffer_t cb_test;
    EXPECT_EQ(circular_buffer_init(nullptr, buffer, BUFFER_SIZE), -1);
    EXPECT_EQ(circular_buffer_init(&cb_test, nullptr, BUFFER_SIZE), -1);
    EXPECT_EQ(circular_buffer_init(&cb_test, buffer, 0), -1);
}

TEST_F(CircularBufferTest, WriteAndReadSingleByte) {
    uint8_t write_data = 0x42;
    uint8_t read_data = 0;

    EXPECT_EQ(circular_buffer_write(&cb, &write_data, 1), 1u);
    EXPECT_FALSE(circular_buffer_is_empty(&cb));
    EXPECT_EQ(circular_buffer_available(&cb), 1u);

    EXPECT_EQ(circular_buffer_read(&cb, &read_data, 1), 1u);
    EXPECT_EQ(read_data, write_data);
    EXPECT_TRUE(circular_buffer_is_empty(&cb));
}

TEST_F(CircularBufferTest, WriteAndReadMultipleBytes) {
    uint8_t write_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint8_t read_data[5] = {0};

    EXPECT_EQ(circular_buffer_write(&cb, write_data, 5), 5u);
    EXPECT_EQ(circular_buffer_available(&cb), 5u);

    EXPECT_EQ(circular_buffer_read(&cb, read_data, 5), 5u);
    EXPECT_THAT(std::vector<uint8_t>(read_data, read_data + 5),
                ElementsAre(0x01, 0x02, 0x03, 0x04, 0x05));
    EXPECT_TRUE(circular_buffer_is_empty(&cb));
}

TEST_F(CircularBufferTest, PeekDoesNotRemoveData) {
    uint8_t write_data[] = {0xAA, 0xBB, 0xCC};
    uint8_t peek_data[3] = {0};
    uint8_t read_data[3] = {0};

    circular_buffer_write(&cb, write_data, 3);

    EXPECT_EQ(circular_buffer_peek(&cb, peek_data, 3), 3u);
    EXPECT_THAT(std::vector<uint8_t>(peek_data, peek_data + 3),
                ElementsAre(0xAA, 0xBB, 0xCC));
    EXPECT_EQ(circular_buffer_available(&cb), 3u);

    EXPECT_EQ(circular_buffer_read(&cb, read_data, 3), 3u);
    EXPECT_THAT(std::vector<uint8_t>(read_data, read_data + 3),
                ElementsAre(0xAA, 0xBB, 0xCC));
    EXPECT_TRUE(circular_buffer_is_empty(&cb));
}

TEST_F(CircularBufferTest, WrapAroundWrite) {
    // Fill buffer almost to the end
    std::vector<uint8_t> initial_data(BUFFER_SIZE - 10, 0x11);
    circular_buffer_write(&cb, initial_data.data(), initial_data.size());
    
    // Read some data to make room at the beginning
    std::vector<uint8_t> dummy(20);
    circular_buffer_read(&cb, dummy.data(), 20);

    // Write data that will wrap around
    std::vector<uint8_t> wrap_data(30, 0x22);
    size_t written = circular_buffer_write(&cb, wrap_data.data(), wrap_data.size());
    // One-byte-free ring buffer semantics can limit wrap write by one
    EXPECT_EQ(written, 29u);

    // Verify wrapped data
    std::vector<uint8_t> read_all(BUFFER_SIZE);
    size_t read_count = circular_buffer_read(&cb, read_all.data(), BUFFER_SIZE);
    EXPECT_EQ(read_count, 255u);
}

TEST_F(CircularBufferTest, FullBufferBehavior) {
    // Fill buffer (remember it keeps 1 byte free)
    std::vector<uint8_t> data(BUFFER_SIZE - 1, 0x33);
    size_t written = circular_buffer_write(&cb, data.data(), data.size());
    EXPECT_EQ(written, BUFFER_SIZE - 1);
    EXPECT_TRUE(circular_buffer_is_full(&cb));

    // Try to write more - should fail
    uint8_t extra = 0x44;
    EXPECT_EQ(circular_buffer_write(&cb, &extra, 1), 0u);
}

TEST_F(CircularBufferTest, FreeSpaceCalculation) {
    EXPECT_EQ(circular_buffer_free_space(&cb), BUFFER_SIZE - 1);

    std::vector<uint8_t> data(100, 0x55);
    circular_buffer_write(&cb, data.data(), data.size());
    EXPECT_EQ(circular_buffer_free_space(&cb), BUFFER_SIZE - 101);

    circular_buffer_read(&cb, data.data(), 50);
    EXPECT_EQ(circular_buffer_free_space(&cb), BUFFER_SIZE - 51);
}

TEST_F(CircularBufferTest, ResetBuffer) {
    std::vector<uint8_t> data(100, 0x66);
    circular_buffer_write(&cb, data.data(), data.size());
    EXPECT_FALSE(circular_buffer_is_empty(&cb));

    circular_buffer_reset(&cb);
    EXPECT_TRUE(circular_buffer_is_empty(&cb));
    EXPECT_EQ(cb.head, 0u);
    EXPECT_EQ(cb.tail, 0u);
}

TEST_F(CircularBufferTest, GetReadBlockLinear) {
    uint8_t *data_ptr = nullptr;
    size_t len = 0;

    // Test empty buffer
    EXPECT_EQ(circular_buffer_get_read_block(&cb, &data_ptr, &len), -1);

    // Write some data
    std::vector<uint8_t> data(50, 0x77);
    circular_buffer_write(&cb, data.data(), data.size());

    // Get read block
    EXPECT_EQ(circular_buffer_get_read_block(&cb, &data_ptr, &len), 0);
    EXPECT_EQ(len, 50u);
    EXPECT_EQ(data_ptr[0], 0x77);

    // Advance and verify
    circular_buffer_advance_read(&cb, 25);
    EXPECT_EQ(circular_buffer_available(&cb), 25u);
}

TEST_F(CircularBufferTest, GetWriteBlockLinear) {
    uint8_t *data_ptr = nullptr;
    size_t len = 0;

    // Get write block on empty buffer
    EXPECT_EQ(circular_buffer_get_write_block(&cb, &data_ptr, &len), 0);
    EXPECT_EQ(len, BUFFER_SIZE - 1);  // Keeps one byte free

    // Write directly to the block
    memset(data_ptr, 0x88, 100);
    circular_buffer_advance_write(&cb, 100);
    EXPECT_EQ(circular_buffer_available(&cb), 100u);

    // Verify data
    std::vector<uint8_t> read_data(100);
    circular_buffer_read(&cb, read_data.data(), 100);
    EXPECT_THAT(read_data, Each(0x88));
}

TEST_F(CircularBufferTest, GetBlocksWithWrapAround) {
    // Fill buffer near the end
    cb.head = BUFFER_SIZE - 10;
    cb.tail = BUFFER_SIZE - 10;

    // Write data that wraps
    std::vector<uint8_t> data(20, 0x99);
    circular_buffer_write(&cb, data.data(), data.size());

    // Get read block - should only return until buffer end
    uint8_t *read_ptr = nullptr;
    size_t read_len = 0;
    EXPECT_EQ(circular_buffer_get_read_block(&cb, &read_ptr, &read_len), 0);
    EXPECT_EQ(read_len, 10u);  // Only until end of buffer

    // Advance and get next block
    circular_buffer_advance_read(&cb, read_len);
    EXPECT_EQ(circular_buffer_get_read_block(&cb, &read_ptr, &read_len), 0);
    EXPECT_EQ(read_len, 10u);  // Wrapped portion
}

TEST_F(CircularBufferTest, ConcurrentWriteRead) {
    std::atomic<bool> stop{false};
    std::atomic<size_t> total_written{0};
    std::atomic<size_t> total_read{0};

    // Writer thread
    std::thread writer([&]() {
        uint8_t data = 0;
        while (!stop) {
            size_t written = circular_buffer_write(&cb, &data, 1);
            if (written > 0) {
                total_written++;
                data++;
            }
            std::this_thread::yield();
        }
    });

    // Reader thread
    std::thread reader([&]() {
        uint8_t data;
        uint8_t expected = 0;
        while (!stop || circular_buffer_available(&cb) > 0) {
            size_t read = circular_buffer_read(&cb, &data, 1);
            if (read > 0) {
                EXPECT_EQ(data, expected);
                expected++;
                total_read++;
            }
            std::this_thread::yield();
        }
    });

    // Let them run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    stop = true;

    writer.join();
    reader.join();

    EXPECT_EQ(total_written.load(), total_read.load());
    EXPECT_TRUE(circular_buffer_is_empty(&cb));
}

TEST_F(CircularBufferTest, PartialReadWrite) {
    // Try to write more than available space
    std::vector<uint8_t> big_data(BUFFER_SIZE * 2, 0xAA);
    size_t written = circular_buffer_write(&cb, big_data.data(), big_data.size());
    EXPECT_EQ(written, BUFFER_SIZE - 1);  // Should only write what fits

    // Try to read more than available
    std::vector<uint8_t> read_data(BUFFER_SIZE * 2);
    size_t read = circular_buffer_read(&cb, read_data.data(), read_data.size());
    EXPECT_EQ(read, BUFFER_SIZE - 1);  // Should only read what's available
}

// Test invalid parameters
TEST_F(CircularBufferTest, InvalidParameters) {
    uint8_t data = 0;

    EXPECT_EQ(circular_buffer_write(nullptr, &data, 1), 0u);
    EXPECT_EQ(circular_buffer_write(&cb, nullptr, 1), 0u);
    EXPECT_EQ(circular_buffer_write(&cb, &data, 0), 0u);

    EXPECT_EQ(circular_buffer_read(nullptr, &data, 1), 0u);
    EXPECT_EQ(circular_buffer_read(&cb, nullptr, 1), 0u);
    EXPECT_EQ(circular_buffer_read(&cb, &data, 0), 0u);

    EXPECT_EQ(circular_buffer_available(nullptr), 0u);
    EXPECT_EQ(circular_buffer_free_space(nullptr), 0u);
}
