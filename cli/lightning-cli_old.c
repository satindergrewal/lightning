/*
 * Helper to submit via JSON-RPC and get back response.
 */
/*#include "configdir.h"
#include "json.h"
#include "version.h"*/
#include <ccan/err/err.h>
#include <ccan/opt/opt.h>
#include <ccan/read_write_all/read_write_all.h>
#include <ccan/str/str.h>
#include <ccan/tal/str/str.h>
#include <common/configdir.h>
#include <common/json.h>
#include <common/version.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

int cli_main(char *buffer,int32_t maxsize,int argc, char *argv[],char *cmd);

int main(int argc, char *argv[])
{
    int32_t retval = -1,maxsize = 1000000; char *buffer = malloc(maxsize);
    if ( buffer != 0 )
    {
        if ( cli_main(buffer,maxsize,argc,argv,0) == 0 )
        {
            printf("%s",buffer);
            retval = 1;
        }
        else
        {
            printf("error %s",buffer);
        }
        free(buffer);
    }
    return(retval);
}

