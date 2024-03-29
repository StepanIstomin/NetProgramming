#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

//
// System calls interceptor for the networking spoiling...
//

extern "C"
{
#include <dlfcn.h>
#include <unistd.h>
}

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>

static void init(void) __attribute__((constructor));

typedef ssize_t (*write_t)(int fd, const void* buf, size_t count);
typedef int (*socket_t)(int domain, int type, int protocol);
typedef int (*close_t)(int fd);

static close_t  old_close;
static socket_t old_socket;
static write_t  old_write;

static int socket_fd = -1;

static std::ofstream fout;

void init(void)
{
    srand(time(nullptr));
    printf("Interceptor library loaded.\n");

    old_close  = reinterpret_cast<close_t>(dlsym(RTLD_NEXT, "close"));
    old_write  = reinterpret_cast<write_t>(dlsym(RTLD_NEXT, "write"));
    old_socket = reinterpret_cast<socket_t>(dlsym(RTLD_NEXT, "socket"));
}

extern "C"
{

    int close(int fd)
    {
        if (fd == socket_fd)
        {
            printf("> close() on the socket was called!\n");
            socket_fd = -1;
        }

        fout.close();

        return old_close(fd);
    }

    ssize_t write(int fd, const void* buf, size_t count)
    {
        auto char_buf = reinterpret_cast<const char*>(buf);

        if (char_buf && (count > 1) && (fd == socket_fd))
        {
            printf("> write() on the socket was called with a string!\n");

            fout.write(char_buf, count - 1);
            fout << "\n";
            fout.flush();
        }

        return old_write(fd, buf, count);
    }

    int socket(int domain, int type, int protocol)
    {
        int cur_socket_fd = old_socket(domain, type, protocol);
        fout.open("/home/kotuk/Desktop/calls.txt", std::ios_base::app);

        if (-1 == socket_fd)
        {
            printf("> socket() was called, fd = %d!\n", cur_socket_fd);
            socket_fd = cur_socket_fd;
        }
        else
        {
            printf("> socket() was called, but socket was opened already...\n");
        }

        return cur_socket_fd;
    }

} // extern "C"
