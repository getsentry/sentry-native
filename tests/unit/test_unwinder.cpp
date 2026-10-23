#include <cstdlib>
#include <new>

// replaces the global operator new to let tests fail C++ allocations
static size_t g_fail_size = 0;

extern "C" void
fail_operator_new(size_t min_size)
{
    g_fail_size = min_size;
}

void *
operator new(size_t size)
{
    if (g_fail_size && size >= g_fail_size) {
        throw std::bad_alloc();
    }
    void *ptr = std::malloc(size ? size : 1);
    if (!ptr) {
        throw std::bad_alloc();
    }
    return ptr;
}

void
operator delete(void *ptr) noexcept
{
    std::free(ptr);
}
