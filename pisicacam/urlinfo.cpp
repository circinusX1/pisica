
#include "main.h"

#include "urlinfo.h"

bool urlinfo::urlParse(const char *u)
{
    char surl[256];
    char* url=surl;

    ::strcpy(url,u);
    char *s = strstr(url, "://");
    int ret;

    if (s) url = s + strlen("://");
    memcpy(this->path, (void *)"/\0", 2);

    if (strchr(url, '@') != NULL)
    {
        ret = sscanf(url, "%[^:]:%[^@]", this->username, this->password);
        if (ret < 2) return -1;
        url = strchr(url, '@') + 1;
    }
    else
    {
        this->username[0] = '\0';
        this->password[0] = '\0';
    }

    if (strchr(url, ':') != NULL)
    {
        ret = sscanf(url, "%[^:]:%hu/%s", this->host,
                     (short unsigned int*)&this->port, this->path+1);
        if (this->port < 1) return -1;
        ret -= 1;
    }
    else
    {
        this->port = 80;
        ret = sscanf(url, "%[^/]/%s", this->host, this->path+1);
    }
    if (ret < 1) return false;

    return true;
}
