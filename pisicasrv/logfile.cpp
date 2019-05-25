/* Copyright (C) 2018 Zirexix.Ltd - All Rights Reserved
* You may use, distribute and modify this code under the
* terms of the ZIREXIX license, (author: Marius Chincisan)
*
* You should have received a copy of the ZIREXIX license with
* this file. If not, please visit www.pegmatis.com
*/


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <mutex>
#include "logfile.h"

int Debug = 0x3;


static std::string LogFile = "/var/log/pisica/pisica.log";
static int    lmanage();
static size_t LogFileLastSz;
static size_t MaxLogSize=20000000;
static int    Max_LogFiles=40;
static int    OnceIn100=0;
std::mutex    MuxLog;         //log file mutex
/**
 * @brief strfl, logs LOG in log file
 * @param ss
 * @param fatal
 */
void strfl(std::stringstream& ss, bool fatal)
{
    std::lock_guard<std::mutex> guard(MuxLog);

    bool chown = false;
    FILE *log = ::fopen(LogFile.c_str(), "ab");
    if (!log){
        log = fopen(LogFile.c_str(), "wb");
        chown=true;
    }
    if (!log)
    {
        std::cout << ("Can not open /var/log/pisica/pisica.log for writing.\n");
        std::cout.flush();
        std::cout << "\r\n";
        return;
    }
    ::fwrite(ss.str().c_str(),sizeof(char),ss.str().length(),log);
    std::cout << ss.str().c_str();
    if(OnceIn100%100==0)
    {
        LogFileLastSz = ::ftell(log);
    }
    ::fclose(log);

    if(chown){
        chmod(LogFile.c_str(), S_IRWXO|S_IRWXG|S_IRWXU);
        system("chown ubuntu:www-data /var/log/pisica/pisica.log");
    }
    if(OnceIn100>10000)
    {
        if(LogFileLastSz > MaxLogSize)
        {
            lmanage();
        }
        OnceIn100=0;
    }
    ++OnceIn100;
}


static int lmanage()
{
    if(!LogFile.empty())
    {
        char fp[128];
        char fps[128];
        char dir[128] = {0};
        char fname[128] = {0};

        ::strcpy(fp,  LogFile.c_str());
        ::strcpy(fname, LogFile.substr(LogFile.find_last_of('/')).c_str());
        ::strcpy(dir, ::dirname(fp));

        if(::strchr(fname,'-')!=0)
            *(::strchr(fname,'-'))=0;
        if(LogFileLastSz > MaxLogSize)
        {
            LogFileLastSz = 0;
            for(int k=Max_LogFiles; k>0; k-- )
            {
                ::sprintf(fps,"%s%s-%d",dir,fname,k);
                ::sprintf(fp,"%s%s-%d",dir,fname,k-1);
                if(k == Max_LogFiles)
                {
                    if(::access(fps,0)==0)
                        ::unlink(fps);
                }
                else
                {
                    if(::access(fp,0)==0)
                    {
                        ::rename(fp, fps);
                    }
                }
            }
            ::rename(LogFile.c_str(), fp);

            FILE* pf = ::fopen(LogFile.c_str(),"wb");
            if(pf)
            {
                ::fclose(pf);
                chmod(LogFile.c_str(), S_IRWXO|S_IRWXG|S_IRWXU);
                system("chown marius:www-data /var/log/pisica/pisica.log");
            }
        }
    }
    return 0;
}



IO::IO(const char* ch):op(ch)
{
    LI("K: " << ch << "{");
}

IO::~IO()
{
    LI("K: " << op << "}");
}
