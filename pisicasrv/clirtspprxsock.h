#ifndef RTSPPRXSOCK_H
#define RTSPPRXSOCK_H

#include "connsock.h"

struct CamSock;
class Request;
class CliRtspPrxSock : public ConnSock
{
public:
    CliRtspPrxSock(ConnSock&,
                   const optionsmap& h,
                   const Request&    lhtr,
                   ConnSock* pcs);
    virtual ~CliRtspPrxSock();
    virtual int transfer(ConnSock* pc);
private:

};

#endif // RTSPSOCK_H
