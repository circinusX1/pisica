#ifndef CAMPRX_SOCK
#define CAMPRX_SOCK

#include "connsock.h"

class CliSock;

#define PRX_BUFF  1024

class CamPrxSock : public ConnSock
{
public:
    CamPrxSock(ConnSock& ,
            const optionsmap& h);
    virtual ~CamPrxSock();
    void closeconn();
    virtual void bind(CliSock* b);
    int transfer(ConnSock* cli);


private:
    uint8_t*    _buff=nullptr;
    CliSock*    _pcli=nullptr;
};

#endif // CAMPRX_SOCK
