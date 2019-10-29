#ifndef SENTRY_IO_HPP_INCLUDED
#define SENTRY_IO_HPP_INCLUDED

#include <sstream>

namespace sentry {

class Path;

class IoWriter {
   public:
    IoWriter();
    virtual ~IoWriter();

    virtual void write(const char *buf, size_t len) = 0;
    void write_char(char c) {
        write(&c, 1);
    }
    void write_str(const std::string &s) {
        write(s.c_str(), s.size());
    }
    template <typename T>
    void write_fmt(T val) {
        std::string s = std::to_string(val);
        write_str(s);
    }

    virtual void flush(){};
    virtual void close(){};
};

class FileIoWriter : public IoWriter {
   public:
    FileIoWriter();
    ~FileIoWriter();
    bool open(const Path &path);
    bool is_closed() const;
    void write(const char *buf, size_t len);
    void flush();
    void close();

   private:
    static const size_t BUF_SIZE = 1024;
#ifdef _WIN32
    FILE *m_file;
#else
    int m_fd;
#endif
    char m_buf[BUF_SIZE];
    size_t m_buflen;
};

class MemoryIoWriter : public IoWriter {
   public:
    MemoryIoWriter(size_t bufsize = 128);
    void write(const char *buf, size_t len);

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
