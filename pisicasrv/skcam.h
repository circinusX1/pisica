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
#ifndef SKCAM_H
#define SKCAM_H

#include <set>
#include "skbase.h"
#include "skcamsq.h"

class skcam;
class skweb;

class skcam : public skbase
{
public:
    skcam(skbase&);
    virtual ~skcam();
    virtual void bind(skweb* b, bool addremove);
    virtual int ioio(const std::vector<skbase*>& clis);
    virtual bool destroy(bool be=true);
private:
    void _shoot(const vf& vf);
    bool _deframe();

private:
    std::set<skweb*>  _pclis;
    size_t      _cap=0;
    size_t      _bytesaccum=0;
    skcamsq     _q;
    vf       _vf;
    std::string _recordname;
    time_t      _now;
    size_t      _bps;
};

#endif // SKCAM_H
