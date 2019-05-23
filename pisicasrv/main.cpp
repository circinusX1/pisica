#define VERSION "1.0.0"

#include "osthread.h"
#include "sock.h"
#include "skbase.h"
#include "sksrv.h"
#include "sks.h"
#include "skcamsq.h"
#include "pks.h"
#include "logfile.h"


bool        __aware=true;
std::string _zs;
int         __cam_port = 8788;
/**
 * @brief ControlC
 * @param i
 */
static void ControlC (int i)
{
    std::cerr << "Ctrl+C signal:" << i << ", user exit";
    __aware = false;
}

/**
 * @brief ControlP
 * @param i
 */
static void ControlP (int i)
{
    std::cerr << "Ctrl+P signl: " << i << ", would continue";
}

/**
 * @brief Sigs
 */
static void Sigs()
{
     vf v(vf::STACK);

#ifndef DEBUG
    signal(SIGINT,  ControlC);
    signal(SIGABRT, ControlC);
    signal(SIGTERM, ControlC);
    signal(SIGKILL, ControlC);
#endif //
    signal(SIGPIPE, SIG_IGN);
    signal(SIGBUS, ControlP);
    signal(SIGTRAP, ControlP);

}

static void loop_callback();


int main(int argc, char *argv[])
{
    sprk ps (12741);    //singleton, limit this instance to 1

    if(!ps())                 // second instance
    {
        LI("already running");
        return -1;
    }

    if(argc < 4)
    {
        LI(argv[0] <<  "CAM_PORT CLI_PORT PASSWORD");
        exit(-1);
    }
    skcamsq q;
    sks p(q);
    sklsn l(p,q);

    LI(strweb_time());
    Sigs();
    p.start_thread();
    __cam_port = ::atoi(argv[2]);
    l.spin(argv[3], ::atoi(argv[1]), __cam_port,loop_callback);
    p.stop_thread();
}

static void loop_callback()
{
    ;
}

