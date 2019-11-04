#ifndef SENTRY_IO_HPP_INCLUDED
#define SENTRY_IO_HPP_INCLUDED

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

    void write_int32(int32_t val) {
        char buf[40];
        size_t len = snprintf(buf, sizeof(buf), "%lld", (long long)val);
        write(buf, len);
    }

    void write_double(double val) {
        char buf[50];
        size_t len = snprintf(buf, sizeof(buf), "%g", val);
        write(buf, len);
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
    ~MemoryIoWriter();
    void write(const char *buf, size_t len);
    void flush();

    char *take();
    const char *buf() const;
    size_t len() const;

   private:
    bool m_terminated;
    char *m_buf;
    size_t m_bufcap;
    size_t m_buflen;
};

}  // namespace sentry

#endif
