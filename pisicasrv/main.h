#ifndef MM_H
#define MM_H

#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include "logfile.h"

extern bool __alive;
extern std::string _zs;
extern int __cam_port;
#define EOS '\0'


inline size_t split(const std::string& str,
                    char delim, std::vector<std::string>& a,
                    char rm=0)
{
    if(str.empty())
        return 0;

    std:: string token;
    size_t prev = 0, pos = 0;
    size_t sl = str.length();
    do
    {
        pos = str.find(delim, prev);
        if (pos == std::string::npos)
            pos=sl;
        token = str.substr(prev, pos-prev);
        if (!token.empty())
        {
            if(rm)
            {
                size_t rmp = token.find(rm);
                if(rmp != std::string::npos)
                {
                    token=token.substr(0,rmp);
                }
            }
            a.push_back(token);
        }
        prev = pos + 1;
    }while (pos < sl && prev < sl);
    return a.size();
}

struct UrlInfo
{
    UrlInfo(){port=80;}
    char  host[128];
    char  path[128];
    char  username[32];
    char  password[32];
    int   port;
};

inline int ParseUrl(const char *u, UrlInfo *urlinfo)
{
    char surl[256];
    char* url=surl;

    ::strcpy(url,u);
    char *s = strstr(url, "://");
    int ret;

    if (s) url = s + strlen("://");
    memcpy(urlinfo->path, (void *)"/\0", 2);

    if (strchr(url, '@') != NULL)
    {
        ret = sscanf(url, "%[^:]:%[^@]", urlinfo->username, urlinfo->password);
        if (ret < 2) return -1;
        url = strchr(url, '@') + 1;
    }
    else
    {
        urlinfo->username[0] = '\0';
        urlinfo->password[0] = '\0';
    }

    if (strchr(url, ':') != NULL)
    {
        ret = sscanf(url, "%[^:]:%hu/%s", urlinfo->host,
                     (short unsigned int*)&urlinfo->port, urlinfo->path+1);
        if (urlinfo->port < 1) return -1;
        ret -= 1;
    }
    else
    {
        urlinfo->port = 80;
        ret = sscanf(url, "%[^/]/%s", urlinfo->host, urlinfo->path+1);
    }
    if (ret < 1) return -1;

    return 0;
}

#define NPOS std::string::npos
#define DELETE_PTR(p)   if(p) delete p; p=nullptr
#define OUT std::cout
extern std::string Campas;
extern std::string Camtok;
extern std::string Srvpas;
#endif // MM_H
