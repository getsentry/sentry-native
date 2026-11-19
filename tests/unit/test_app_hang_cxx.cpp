#include <cstdlib>
#include <stdexcept>

namespace {

[[noreturn]] __attribute__((noinline)) void
throw_nested(int depth)
{
    if (depth > 0) {
        throw_nested(depth - 1);
    }
    throw std::runtime_error("app-hang stress");
}

} // namespace

// Keeps throwing and catching C++ exceptions, which takes the unwinder's
// FDE cache lock and allocates the exception objects.
extern "C" void
sentry__test_throw_catch_loop(volatile long *keep_running)
{
    while (__atomic_load_n(keep_running, __ATOMIC_SEQ_CST)) {
        try {
            throw_nested(8);
        } catch (const std::exception &) {
        }
    }
}

// Keeps allocating and freeing blocks of varying sizes.
extern "C" void
sentry__test_malloc_loop(volatile long *keep_running)
{
    size_t size = 16;
    while (__atomic_load_n(keep_running, __ATOMIC_SEQ_CST)) {
        void *blocks[16];
        for (size_t i = 0; i < 16; i++) {
            blocks[i] = malloc(size);
            size = size * 3 % 65521 + 16;
        }
        for (size_t i = 0; i < 16; i++) {
            free(blocks[i]);
        }
    }
}
