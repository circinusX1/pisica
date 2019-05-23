#ifndef LOGF_H
#define LOGF_H

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include "osthread.h"
extern int Debug;

void strfl(std::stringstream& ss, bool fatal=false);

#define LI(x) if(Debug & 0x1) \
do{\
    std::stringstream ss; ss << str_time() <<":"<< pthread_self() << "I: "<< x << "\r\n";\
    strfl(ss); \
}while(0);


//-----------------------------------------------------------------------------
// WARNING LOGING
#define LW(x) if(Debug & 0x2)\
do{\
    std::stringstream ss; ss <<str_time() <<":"<< pthread_self() <<  "W: " << x << "\r\n";\
    strfl(ss); \
}while(0);


//-----------------------------------------------------------------------------
// ERROR LOGING
#define LE(x) \
do{\
    std::stringstream ss; ss <<str_time() <<":"<< pthread_self() << " E: " << x << "\r\n";\
    strfl(ss); \
}while(0);


//-----------------------------------------------------------------------------
// // TRACE LOGING
#define LD(x) if(Debug & 0x08)\
do{\
    std::stringstream ss; ss <<str_time() <<":"<< pthread_self() << " D: "<< x << "\r\n";\
    strfl(ss); \
}while(0);

//-----------------------------------------------------------------------------
// OUTPUT LOGING
#define LO(x) if(Debug & 0x10)\
do{\
    std::stringstream ss; ss <<str_time() <<":"<< pthread_self() << " O: " << x << "\r\n";\
    strfl(ss); \
}while(0);

struct IO
{
    std::string op;
    IO(const char* ch);
    ~IO();
};


#endif // LOGS_H
