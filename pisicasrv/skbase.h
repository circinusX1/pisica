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
#ifndef connsock_H
#define connsock_H

#include <string>
#include <map>
#include "sock.h"
#include "vf.h"
#include "config.h"

extern std::string empty_string;
struct optionsmap : public std::map<std::string,std::string>
{
    const std::string& options(const char* name)const
    {
        const auto a = this->find(name);
        if(a != this->end())
            return (*a).second;
        return empty_string;
    }
};


class skbase : public tcp_cli_sock
{
public:
    typedef enum _STYPE{
        NONE = 0,
        CAM=5,
        CLIENT=0xFF,
    }STYPE;


    skbase(skbase& r, STYPE t);
    skbase(STYPE t):_t(t){ COUT_("SKBASE " << _t);};
    virtual ~skbase();
    virtual bool    isopen();
    virtual bool destroy(bool be=true);
    virtual int snd(const uint8_t* b, size_t room, uint32_t extra=0);
    virtual int snd(const char* b, size_t room, uint32_t extra=0);
    virtual int ioio(const std::vector<skbase*>& clis){return 0;};
    int         oi(char* outin, size_t len,size_t maxlen);
    int recdata();
    const std::string name()const{return _name;}
    void name(const std::string m){_name=m;}
    STYPE type()const {return _t;}

protected:
    std::string _mac;
    std::string _name;
    STYPE       _t;          // avoid dynamic casts
};

#endif // connsock_H
