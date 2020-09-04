# Buddy allocator

A Simple single-header header-only template implementation of a buddy allocator

## Example usage

```c++
#include "BuddyAllocator.h"

int main()
{
    // The allocator uses a pool of 1 << 10 bytes to satisfy allocation requests.
    // template <uint8_t MAX_POWER, uint8_t ALIGNMENT = std::alignment_of_v<uint64_t>>
    ub::BuddyAllocator<10> pool;

    void* p = pool.alloc(240);
    ...
    pool.free();
}
```

## Description

Buddy allocators are described in detail elsewhere, so I leave that as an exercise for the reader.

This implementation is written in C++17 but inspired by the C allocator written by Tianfu Ma found [here](https://github.com/matianfu/buddy). 

- The allocator internally holds a contiguous buffer whose size is a power of two. The size is determined by a template parameter, `MAX_POWER`. 

- The alignment of the internal buffer can also be set with a second template parameter, `ALIGNMENT`, which defaults to the alignment of a `uint64_t` (it seemed reasonable).

- In general, a request for `N` bytes will allocate a buffer whose size is the power of two just larger than `N`:

   - The allocator uses the byte immediately before the returned address to hold metadata about the allocated block (`log2()` of its size). This is required for deallocation. There are less intrusive schemes, but this is simple and consumes little additional memory. 

   - As a consequence of the deallocation scheme, a request for `N` bytes will actually allocate `N + 1` bytes. This means that a user request for 15 bytes will actually consume 16 bytes in the pool, and a request for 16 bytes will actually consume 32 bytes. 

   - A buffer overrun can easily trash the metadata associated with a different allocation.

   - The minimum user allocation is 1 byte, but the allocator internally needs chunks large enough to hold a pointer (for making a freelist) and a byte (for de-allocation metadata). This means that a request for 1 byte would return a pointer to a buffer of 8 bytes on many embedded systems. More on PCs.

The repository includes a version of Catch2 to support testing. The tests repeatedly perform allocations to exhaust the allocator and make a series of sanity checks on the buffers that are returned. The template does not include any helper functions to interrogate its internals for testing purposes.

You could theoretically use a buddy allocator as a general purpose replacement for malloc, but the binary nature of the blocks sizes could make this very wasteful. It probably makes most sense for short-lived allocations whose sizes are quite variable, such as structures for passing data to event loops or other threads.


