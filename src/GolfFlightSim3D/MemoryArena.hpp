struct MemoryArena
{
public:
    bool initialize(size_t size);
    u8 *allocateFromArena(size_t size);

private:
    u8 *start;
    u8 *end;
    u8 *current;
};