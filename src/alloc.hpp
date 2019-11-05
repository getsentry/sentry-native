#ifndef SENTRY_ALLOC_HPP_INCLUDED
#define SENTRY_ALLOC_HPP_INCLUDED

#include <atomic>
#include "signalsupport.hpp"

namespace sentry {

class PageAllocator {
   public:
    PageAllocator();
    ~PageAllocator() {
        free();
    }
    void free();
    void *alloc(size_t bytes);
    bool allocated_pointer(void *ptr) const;

   private:
    struct AllocationHeader {
        AllocationHeader *next_page;
        size_t num_pages;
    };
};

class SignalSafeAllocator {};

}  // namespace sentry

#endif
