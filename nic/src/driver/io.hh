#pragma once
#include <string>
#include <type_traits>
#include <unistd.h>

#include <util/log.hh>

class IO
{

public:
    explicit IO(const std::string &path, int flags)
    {
        this->_fd = open(path.c_str(), flags);
    }

    ~IO()
    {
        if (this->_fd >= 0)
        {
            close(_fd);
        }
    }

    template <typename T>
    void write_io(T value, size_t offset)
    {
        static_assert(std::is_unsigned_v<T>, "must be unsigned type");

        if (pwrite(this->_fd, &value, sizeof(value), offset) != sizeof(value))
        {
           fatal() << "Failed to write IO.";
        }

        __asm__ volatile("" : : : "memory");
    }

    template <typename T>
    T read_io(size_t offset)
    {
        static_assert(std::is_unsigned_v<T>, "must be unsigned type");
        __asm__ volatile("" : : : "memory");

        T temp;
        if (pread(this->_fd, &temp, sizeof(temp), offset) != sizeof(temp))
        {
           fatal() << "Failed to read IO.";
        }

        return temp;
    }

    operator bool() const
    {
        return this->_fd >= 0;
    }

private:
    int _fd;
};