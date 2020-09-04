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
//
// This is a C++ implementation modelled, more or less, on the C buddy allocator 
// written by Tianfu Ma (matianfu@gmail.com), which be found at 
// https://github.com/matianfu/buddy. This code is licenced under the 
// same terms.
//
///////////////////////////////////////////////////////////////////////////////
#include <type_traits>
#include <cstdint>
#include <algorithm>


namespace ub {


// Simple buddy allocator template adapted from a C implementation. Mainly intended 
// for embedded applications, but could be used on any platform.
// 
// Contains some nasty old-school head-scratchy pointer manipulations.
// Allocations are made from a single contiguous block of size 1 << MAX_POWER.
// That block can be aligned as you wish, but defaults to something that should 
// usually work. You can have multiple instances with different size in the same 
// program. The allocator could live on the stack, as a global object, as a member 
// of some other class. It can be dynamically allocated if that makes sense. 
template <uint8_t MAX_POWER, uint8_t ALIGNMENT = std::alignment_of_v<uint64_t>>
class BuddyAllocator
{
    // A convenience function for determining the minimum possibly block size.
    static constexpr uint8_t log2(uint32_t size)
    {
        uint8_t result = 0;
        while ((1U << result) < size) 
        {
            ++result;
        }
        return result;
    }

public:
    // Minimum block size must be capable of storing a pointer and another byte for the
    // order = log2(size). The scheme is a bit nasty because the order of an allocation is 
    // stored in the last byte of the preceding block in the pool (with a dummy variable/padding
    // for the zeroth block). This means the returned pointer is nicely aligned, and avoids keeping 
    // a separate array or bit field to hold the order information. 
    static constexpr uint8_t MIN_ORDER = log2(sizeof(void*) + 1);
    static constexpr uint8_t MAX_ORDER = MAX_POWER;

    static_assert((1U << MIN_ORDER) >= (sizeof(void*) + 1));
    static_assert(MAX_ORDER >= MIN_ORDER);

    BuddyAllocator()
    {
        // The base state is a single large block which will be sub-divided as
        // allocations are made.
        m_freelists[MAX_ORDER - MIN_ORDER] = &m_buffer[0];
    }

    // Returns a block with size the smallest power of two which will hold the 
    // request. Internally allocates size + 1, with the extra byte used to store 
    // the order - the power of two that was needed - to help with free().
    void* alloc(uint32_t size)
    {
        // Find the power of 2 needed to satisfy the request. Add one for metadata.
        uint8_t order = std::max(MIN_ORDER, log2(size + 1));

        // Confirm the request is not too large.
        if ((size == 0) || (order > MAX_ORDER))
        { 
            return nullptr;
        }
  
        // Find the first free block we can use, it may be larger than we need.
        uint8_t* block = m_freelists[order - MIN_ORDER];
        uint8_t  index = order;
        while ((block == nullptr) && (index < MAX_ORDER))
        {
            ++index;
            block = m_freelists[index - MIN_ORDER];
        }

        // Confirm the request can be satisfied.
        if (block == nullptr)
        {
            return nullptr;
        }

        // Store any buddies in the relevant free lists. 
        m_freelists[index - MIN_ORDER] = *reinterpret_cast<uint8_t**>(block);
        while (index > order)
        {
            --index;
            uint8_t* buddy = buddy_of(block, index);
            // Equivalent to having a linked list of struct Pointer { Pointer* next; }; 
            *reinterpret_cast<uint8_t**>(buddy) = m_freelists[index - MIN_ORDER];
            m_freelists[index - MIN_ORDER] = buddy;
        }

        // Store the order so that we know how to free this pointer later.
        *(block - 1) = order;
        return block;
    }

    void free(void* pointer)
    {
        if (pointer == nullptr)
        {
            return;
        }

        uint8_t* block = static_cast<uint8_t*>(pointer);

        // Retrieve the order - indicates the size of the allocation.
        uint8_t order = *(block - 1);

        while (true)
        {
            // Is the buddy block already free?
            uint8_t* buddy = buddy_of(block, order);            
            // Use a pointer to pointer so we can modify the value later.
            uint8_t** ptr = &m_freelists[order - MIN_ORDER];
            while ((*ptr != nullptr) && (*ptr != buddy))
            {
                ptr = reinterpret_cast<uint8_t**>(*ptr);
            }

            // If the buddy was not found in the free list we are done.
            if (*ptr == nullptr)
            {
                // Equivalent to having a linked list of struct Pointer { Pointer* next; }; 
                *reinterpret_cast<uint8_t**>(block) = m_freelists[order - MIN_ORDER];
                m_freelists[order - MIN_ORDER] = block;
                return;
            }

            // The buddy was found in the free list. We will coalesce.
            // Remove the buddy from the free list by assigning the next item in the list.
            *ptr = *reinterpret_cast<uint8_t**>(*ptr);

            // Take the lower address of the block and its buddy for adding into the 
            // next free list.
            block = std::min(block, buddy);
            ++order;
        }
    }

private:
    uint8_t* buddy_of(uint8_t* ptr, uint8_t order)
    {
        uint32_t size = 1U << order;
        uint8_t* base = &m_buffer[0];
        return base + ((ptr - base) ^ size);
    }

private:
    // Each power of 2 has it's own free list of buddies not yet coalesced. 
    uint8_t* m_freelists[MAX_ORDER - MIN_ORDER + 1]{};
    // This is needed to account for the order storage in the block with the lowest address.
    uint8_t  m_dummy{};
    // Static buffer used to supply all the allocations.
    alignas(ALIGNMENT)
    uint8_t  m_buffer[1 << MAX_ORDER]{};
};


} // namespace ub {
