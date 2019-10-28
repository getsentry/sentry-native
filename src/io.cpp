#include "io.hpp"
#include <fnctl.h>
#include <cstdlib>

using namespace sentry;

IoWriter::IoWriter() {
}

IoWriter::~IoWriter() {
}

FileIoWriter::FileIoWriter() : m_buflen(0), m_fd(0) {
}

FileIoWriter::~FileIoWriter() {
    flush();
}

bool FileIoWriter::open(const Path &path) {
    m_fd = open(path.as_osstr(), O_WRONLY | O_CREAT);
    return true;
}

void FileIoWriter::write(const char *buf, size_t len) {
    size_t to_write = len;
    while (to_write) {
        size_t can_write = std::min(BUF_SIZE - m_buflen, to_write);
        memcpy(m_buf + m_buflen, buf, can_write);
        m_buflen += can_write;
        to_write -= can_write;
        if (m_buflen == BUF_SIZE) {
            flush();
        }
    }
}

void FileIoWriter::flush() {
    ::write(m_fd, m_buf, m_buflen);
    m_buflen = 0;
}

MemoryIoWriter::MemoryIoWriter(size_t bufsize)
    : m_buflen(0), m_bufcap(bufsize), m_buf((char *)malloc(bufsize)) {
}

void MemoryIoWriter::write(const char *buf, size_t len) {
    size_t size_needed = m_buflen + len;
    if (size_needed < m_bufcap) {
        size_t new_bufcap = m_bufcap;
        while (new_bufcap < size_needed) {
            new_bufcap *= 1.3;
        }
        m_buf = (char *)realloc(m_buf, new_bufcap);
        m_bufcap = new_bufcap;
    }

    memcpy(m_buf + m_buflen, buf, len);
    m_buflen += len;
}

void MemoryIoWriter::flush() {
}

char *MemoryIoWriter::take() {
    char *rv = m_buf;
    m_buf = nullptr;
    return rv;
}

const char *MemoryIoWriter::buf() const {
    return m_buf;
}

size_t MemoryIoWriter::len() const {
    return m_buflen;
}
