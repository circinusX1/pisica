#ifndef URLINFO_H
#define URLINFO_H


struct urlinfo
{
    urlinfo(const char *u){
        port=80;host[0]=0;
        urlParse(u);
    }
    char  host[128];
    char  path[128];
    char  username[32];
    char  password[32];
    int   port;
    bool urlParse(const char *u);
};



#endif // URLINFO_H
