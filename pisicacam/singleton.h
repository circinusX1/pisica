
#ifndef PROCSINGLETON_H
#define PROCSINGLETON_H

#include "sock.h"
#include <string>
#include <netinet/in.h>

/**
 * @brief The SingleProc class
 * limits this instance to one process inthe memory, second(s) instances
 * would be shells into thie engine
 */
class SingleProc
{
public:
    SingleProc(uint16_t port0)
            : socket_fd(-1)
              , rc(1)
              , port(port0)
    {
    }

    ~SingleProc()
    {
        if (socket_fd != -1)
        {
            ::close(socket_fd);
        }
    }

    bool operator()()
    {
        if (socket_fd == -1 || rc)
        {
            socket_fd = -1;
            rc = 1;

            if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
            {
                throw strerror(errno);
            }
            else
            {
                struct sockaddr_in name;
                name.sin_family = AF_INET;
                name.sin_port = htons (port);
                name.sin_addr.s_addr = htonl (INADDR_ANY);
                rc = bind (socket_fd, (struct sockaddr *) &name, sizeof (name));
            }
        }
        return (socket_fd != -1 && rc == 0);
    }

    std::string GetLockFileName()
    {
        return "port " + std::to_string(port);
    }

private:
    int socket_fd = -1;
    int rc;
    uint16_t port;
};



#endif // PROCSINGLETON_H
