#include "server.h"
#include <signal.h>


int main(){
    signal(SIGPIPE, SIG_IGN);

    Server srv;
    srv.Start(9000);

    return 0;
}
