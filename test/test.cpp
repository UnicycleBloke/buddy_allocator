///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2020 Alan Chambers (unicycle.bloke@gmail.com)
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
///////////////////////////////////////////////////////////////////////////////
#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"
#include "include/BuddyAllocator.h"
#include <iostream>
#include <vector>
#include <cstdlib>
#include <algorithm>


TEST_CASE("Fixed allocation until exhaustion", "[Buddy]") 
{
    constexpr uint8_t MAX_ORDER = 14; // => 16KB
    ub::BuddyAllocator<MAX_ORDER> pool;

    struct Block
    {
        uint8_t* ptr;
        uint32_t size;
        uint8_t  count;
    };

    CHECK(pool.alloc(0) == nullptr);
    CHECK(pool.alloc(1 << pool.MAX_ORDER) == nullptr);

    // Start from 1 as this requests a single byte.
    for (uint8_t i = 1; i < pool.MAX_ORDER; ++i)
    {
        // "- 1" to account for order storage so that no space is wasted. 
        uint32_t size  = (1 << i) - 1;
        uint8_t  count = 0;

        std::vector<Block> blocks;
        while (true)
        {
            auto ptr = static_cast<uint8_t*>(pool.alloc(size));
            if (!ptr) 
               break;

            blocks.push_back({ptr, size, count});
            // Fill the allocated buffer to check for overwrites.
            std::memset(ptr, count++, size);
        }

        // Fixed size means we know exactly how many allocations we will get.
        CHECK(blocks.size() == (1 << (pool.MAX_ORDER - std::max(pool.MIN_ORDER, i))));

        // Confirm that all the allocations have different addresses.
        CHECK(std::unique(blocks.begin(), blocks.end(), [](Block a, Block b){ return a.ptr == b.ptr; }) == blocks.end());

        // Confirm that the pointers are in a sensible range for a contiguous buffer.
        uint8_t* min_ptr = std::min_element(blocks.begin(), blocks.end(), [](Block a, Block b){ return a.ptr < b.ptr; })->ptr;
        uint8_t* max_ptr = std::min_element(blocks.begin(), blocks.end(), [](Block a, Block b){ return a.ptr > b.ptr; })->ptr;
        CHECK(((blocks.size() == 1) || (max_ptr > min_ptr)));
        CHECK((max_ptr - min_ptr) <= (1 << pool.MAX_ORDER));

        // Confirm that the pointers lie within the allocator.
        uint8_t* base_ptr = reinterpret_cast<uint8_t*>(&pool);
        uint8_t* last_ptr = base_ptr + sizeof(pool);
        CHECK(max_ptr > base_ptr);
        CHECK(max_ptr < last_ptr);
        CHECK(min_ptr > base_ptr);
        CHECK(min_ptr < last_ptr);

        // Shuffle so that we deallocate in a different order.
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(blocks.begin(), blocks.end(), g);
        
        // Check for overwrites and free the allocations.
        for (auto block: blocks)
        {
            uint8_t order = *(block.ptr - 1);
            CHECK(order >= pool.MIN_ORDER);
            CHECK(order <= pool.MAX_ORDER);
            CHECK((1U << order) == (std::max(block.size + 1U, 1U << pool.MIN_ORDER)));
            CHECK(std::all_of(block.ptr, block.ptr + block.size, [block](uint8_t b) { return b == block.count; }));
            pool.free(block.ptr);
        }
    }
}


TEST_CASE("Random allocation till exhausation", "[Buddy]") 
{
    constexpr uint8_t MAX_ORDER = 14; // => 16KB
    ub::BuddyAllocator<MAX_ORDER> pool;

    struct Block
    {
        uint8_t* ptr;
        uint32_t size;
        uint8_t  count;
    };

    // Simple Montecarlo testing - randomly allocate until the pool is exhausted,
    // deallocated, and repeat. 
    for (uint16_t i = 0; i < 1'000; ++i)
    {
        uint32_t total_size = 0;
        uint8_t  count      = 0;

        std::vector<Block> blocks;
        while (total_size < (1 << MAX_ORDER))
        {
            uint32_t size = (1U << (std::rand() % (pool.MAX_ORDER - pool.MIN_ORDER + 1) + pool.MIN_ORDER)) - 1;

            uint8_t* ptr = static_cast<uint8_t*>(pool.alloc(size));
            if (ptr)
            {
                total_size += size + 1;
                blocks.push_back({ptr, size, count});
                // Fill the allocated buffer to check for overwrites.
                std::memset(ptr, count++, size);
            }
        }

        // Confirm that all the allocations have different addresses.
        CHECK(std::unique(blocks.begin(), blocks.end(), [](Block a, Block b){ return a.ptr == b.ptr; }) == blocks.end());

        // Confirm that the pointers are in a sensible range for a contiguous buffer.
        uint8_t* min_ptr = std::min_element(blocks.begin(), blocks.end(), [](Block a, Block b){ return a.ptr < b.ptr; })->ptr;
        uint8_t* max_ptr = std::min_element(blocks.begin(), blocks.end(), [](Block a, Block b){ return a.ptr > b.ptr; })->ptr;
        CHECK(((blocks.size() == 1) || (max_ptr > min_ptr)));
        CHECK((max_ptr - min_ptr) <= (1 << pool.MAX_ORDER));

        // Confirm that the pointers lie within the allocator.
        uint8_t* base_ptr = reinterpret_cast<uint8_t*>(&pool);
        uint8_t* last_ptr = base_ptr + sizeof(pool);
        CHECK(max_ptr > base_ptr);
        CHECK(max_ptr < last_ptr);
        CHECK(min_ptr > base_ptr);
        CHECK(min_ptr < last_ptr);

        // Shuffle so that we deallocate in a different order.
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(blocks.begin(), blocks.end(), g);
        
        // Check for overwrites and free the allocations.
        for (auto block: blocks)
        {
            uint8_t order = *(block.ptr - 1);
            CHECK(order >= pool.MIN_ORDER);
            CHECK(order <= pool.MAX_ORDER);
            CHECK((1U << order) == (std::max(block.size + 1U, 1U << pool.MIN_ORDER)));
            CHECK(std::all_of(block.ptr, block.ptr + block.size, [block](uint8_t b) { return b == block.count; }));
            pool.free(block.ptr);
        }
    }
}


