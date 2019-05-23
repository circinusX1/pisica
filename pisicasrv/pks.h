/**
# Copyright (C) 2007-2015 s(marrius9876@gmail.com)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
*/

#ifndef PROCSINGLETON_H
#define PROCSINGLETON_H


#include "sock.h"
#include <string>
#include <netinet/in.h>

class sprk
{
public:
    sprk(uint16_t port0)
            : socket_fd(-1)
              , rc(1)
              , port(port0)
    {
    }

    ~sprk()
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
