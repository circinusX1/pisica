#ifndef MAIN_H
#define MAIN_H

#include "osthread.h"
#include "config.h"

extern bool         __alive;
extern std::string  _mac;
extern config       _cfg;
extern std::string  _campsw;
extern std::string  _srvpsw;
extern umutex*      _pm;
#endif //MAIN_H
