#ifndef SENTRY_IO_HPP_INCLUDED
#define SENTRY_IO_HPP_INCLUDED

#include "internal.hpp"
#include "path.hpp"

namespace sentry {

class IoWriter {
   public:
    IoWriter();
    virtual ~IoWriter();

    virtual void write(const char *buf, size_t len) = 0;
    virtual void flush() = 0;
};

class FileIoWriter : public IoWriter {
   public:
    FileIoWriter();
    bool open(const Path &path);
    void write(const char *buf, size_t len);
    void flush();

   private:
    void flush_buf();

    static const size_t BUF_SIZE = 1024;
    int m_fd;
    char m_buf[BUF_SIZE];
    size_t m_buflen;
};

class MemoryIoWriter : public IoWriter {
   public:
    MemoryIoWriter(size_t bufsize = 128);
    void write(const char *buf, size_t len);
    void flush();

    char *take();
    const char *buf() const;
    size_t len() const;

   private:
    char *m_buf;
    size_t m_bufcap;
    size_t m_buflen;
};

}  // namespace sentry

#endif
