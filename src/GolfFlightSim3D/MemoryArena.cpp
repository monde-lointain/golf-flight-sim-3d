bool MemoryArena::initialize(size_t size)
{
    void *memory = calloc(1, size);
    if (memory == NULL)
    {
        spdlog::error("Failed to allocate required memory for the arena.");
        return false;
    }

    start = (u8 *)memory;
    end = start + size;
    current = start;

    spdlog::debug("Allocated {} bytes for arena.", size);
    spdlog::debug("Arena start: {}.", fmt::ptr(start));
    spdlog::debug("Arena end: {}.", fmt::ptr(end));

    return true;
}

u8 *MemoryArena::allocateFromArena(size_t size)
{
    spdlog::debug("Allocating {} bytes", size);

    assert(current + size <= end);
    u8 *allocated_memory = current;
    current += size;

    spdlog::debug("{} bytes left ({:.2f}% used)", end - current, (f32)(current - start) / (f32)(end - start));

    return allocated_memory;
}