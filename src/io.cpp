#include <cstdlib>

#include "internal.hpp"
#include "io.hpp"
#include "path.hpp"

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#endif

using namespace sentry;

IoWriter::IoWriter() {
}

IoWriter::~IoWriter() {
}

FileIoWriter::FileIoWriter() : m_buflen(0) {
    // on non win32 platforms we only use open/write/close so that
    // we can achieve something close to being async safe so we can
    // use this class in crashing situations.
#ifdef _WIN32
    m_file = nullptr;
#else
    m_fd = -1;
#endif
}

FileIoWriter::~FileIoWriter() {
    close();
}

#ifndef _WIN32
int mode_to_flags(const char *mode) {
    int flags = 0;
    for (; *mode; mode++) {
        switch (*mode) {
            case 'r':
                flags |= O_RDONLY;
                break;
            case 'w':
                flags |= O_WRONLY | O_CREAT | O_TRUNC;
                break;
            case 'a':
                flags |= O_WRONLY | O_CREAT | O_APPEND;
                break;
            case 'b':
                break;
            case '+':
                flags |= O_RDWR;
                break;
            default:;
        }
    }
    return flags;
}
#endif

bool FileIoWriter::open(const Path &path, const char *mode) {
#ifdef _WIN32
    m_file = path.open(mode);
    return m_file != nullptr;
#else
    m_fd = ::open(path.as_osstr(), mode_to_flags(mode),
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    return m_fd >= 0;
#endif
}

void FileIoWriter::write(const char *buf, size_t len) {
    size_t to_write = len;
    while (to_write) {
        size_t can_write = std::min(BUF_SIZE - m_buflen, to_write);
        memcpy(m_buf + m_buflen, buf, can_write);
        m_buflen += can_write;
        to_write -= can_write;
        buf += can_write;
        if (m_buflen == BUF_SIZE) {
            flush();
        }
    }
}

void FileIoWriter::flush() {
#ifdef _WIN32
    fwrite(m_buf, 1, m_buflen, m_file);
    fflush(m_file);
#else
    ::write(m_fd, m_buf, m_buflen);
#endif
    m_buflen = 0;
}

void FileIoWriter::close() {
    flush();
#ifdef _WIN32
    fclose(m_file);
#else
    ::close(m_fd);
#endif
    m_buflen = 0;
}

bool FileIoWriter::is_closed() const {
#ifdef _WIN32
    return m_file == nullptr;
#else
    return m_fd == 0;
#endif
}

MemoryIoWriter::MemoryIoWriter(size_t bufsize)
    : m_terminated(false),
      m_buf((char *)malloc(bufsize)),
      m_bufcap(bufsize),
      m_buflen(0) {
}

MemoryIoWriter::~MemoryIoWriter() {
    free(m_buf);
}

void MemoryIoWriter::flush() {
    if (!m_terminated) {
        write_char(0);
        m_buflen -= 1;
        m_terminated = true;
    }
}

void MemoryIoWriter::write(const char *buf, size_t len) {
    size_t size_needed = m_buflen + len;
    if (size_needed > m_bufcap) {
        size_t new_bufcap = m_bufcap;
        while (new_bufcap < size_needed) {
            new_bufcap = (size_t)(new_bufcap * 1.3);
        }
        m_buf = (char *)realloc(m_buf, new_bufcap);
        m_bufcap = new_bufcap;
    }

    memcpy(m_buf + m_buflen, buf, len);
    m_buflen += len;
    m_terminated = false;
}

char *MemoryIoWriter::take() {
    flush();
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

IoReader::IoReader() {
}

IoReader::~IoReader() {
}

FileIoReader::FileIoReader() : m_bufoff(0), m_buflen(0) {
}

FileIoReader::~FileIoReader() {
    close();
}

bool FileIoReader::open(const Path &path, const char *mode) {
#ifdef _WIN32
    m_file = path.open(mode);
    return m_file != nullptr;
#else
    m_fd = ::open(path.as_osstr(), mode_to_flags(mode),
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    return m_fd >= 0;
#endif
}

bool FileIoReader::is_closed() const {
#ifdef _WIN32
    return m_file == nullptr;
#else
    return m_fd == 0;
#endif
}

void FileIoReader::close() {
    if (is_closed()) {
        return;
    }
#ifdef _WIN32
    fclose(m_file);
    m_file = nullptr;
#else
    ::close(m_fd);
    m_fd = 0;
#endif
}

size_t FileIoReader::read_into(char *buf, size_t len) {
    if (m_buflen - m_bufoff == 0) {
#ifdef _WIN32
        m_buflen = fread(m_buf, 1, BUF_SIZE, m_file);
        m_bufoff = 0;
#else
        ssize_t rv = read(m_fd, m_buf, BUF_SIZE);
        if (rv < 0) {
            return 0;
        }
        m_buflen = (size_t)rv;
        m_bufoff = 0;
#endif
    }

    if (m_buflen - m_bufoff >= 0) {
        size_t n = std::min(m_buflen - m_bufoff, len);
        memcpy(buf, m_buf + m_bufoff, n);
        m_bufoff += n;
        return n;
    } else {
        return 0;
    }
}
