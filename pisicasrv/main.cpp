#define VERSION "1.0.0"

#include "osthread.h"
#include "sock.h"
#include "skbase.h"
#include "sksrv.h"
#include "sks.h"
#include "skcamsq.h"
#include "pks.h"
#include "logfile.h"

std::string Campas;
std::string Camtok;
std::string Srvpas;
bool        __alive=true;
std::string _zs;
int         __cam_port = 8788;
/**
 * @brief ControlC
 * @param i
 */
static void ControlC (int i)
{
    std::cerr << "Ctrl+C signal:" << i << ", user exit";
    __alive = false;
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
    sprk ps (42741);    //singleton, limit this instance to 1

    if(!ps())                 // second instance
    {
        std::cout << "already running\n";
        return -1;
    }

    if(argc != 6)
    {
        std::cout<< argv[0] <<  " CAM_PORT CLI_PORT PASSWORD campas camtok\n";
        exit(-1);
    }
    skcamsq q;
    sks     p(q);
    sksrv   l(p,q);


    Srvpas = argv[3];
    Campas = argv[4];  // make this coming from camera
    Camtok = argv[5];
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

