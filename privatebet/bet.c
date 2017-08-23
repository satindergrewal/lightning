/******************************************************************************
 * Copyright © 2014-2018 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

// https://lists.linuxfoundation.org/pipermail/lightning-dev/2016-January/000403.html
// ^ is multisig

#include "bet.h"
char *LN_idstr,Host_ipaddr[64],Host_peerid[67],BET_ORACLEURL[64] = "127.0.0.1:7797";
uint16_t LN_port;
int32_t Gamestart,Gamestarted,Lastturni,Maxrounds = 3,Maxplayers = 10;
uint8_t BET_logs[256],BET_exps[510];
bits256 *Debug_privkeys;
struct BET_shardsinfo *BET_shardsinfos;
portable_mutex_t LP_peermutex,LP_commandmutex,LP_networkmutex,LP_psockmutex,LP_messagemutex,BET_shardmutex;
int32_t LP_canbind,IAMLP,IAMHOST,IAMORACLE;
struct LP_peerinfo  *LP_peerinfos,*LP_mypeer;
bits256 Mypubkey,Myprivkey,Clientrhash,Hostrhashes[CARDS777_MAXPLAYERS+1];
char Host_channel[64];

#include "../../SuperNET/iguana/exchanges/LP_network.c"
#include "../../SuperNET/iguana/exchanges/LP_secp.c"
#include "../../SuperNET/iguana/exchanges/LP_bitcoin.c"

void randombytes_buf(void * const buf, const size_t size)
{
    OS_randombytes((void *)buf,(int32_t)size);
}

#include "gfshare.c"
#include "cards777.c"
#include "network.c"
#include "oracle.c"
#include "commands.c"
#include "table.c"
#include "payment.c"
#include "client.c"
#include "host.c"
#include "states.c"

// original shuffle with player 2 encrypting to destplayer
// autodisconnect
// payments/bets -> separate dealer from pub0
// virtualize games
// privatebet host -> publish to BET chain
// tableid management -> leave, select game, start game

int main(int argc,const char *argv[])
{
    uint16_t tmp,rpcport = 7797,port = 7797+1;
    char connectaddr[128],bindaddr[128],smartaddr[64],randphrase[32],*modestr,*hostip,*passphrase=0,*retstr; cJSON *infojson,*argjson,*reqjson,*deckjson; uint64_t randvals; bits256 privkey,pubkey,pubkeys[64],privkeys[64]; uint8_t pubkey33[33],taddr=0,pubtype=60; uint32_t i,n,range,numplayers; int32_t testmode=0,pubsock=-1,subsock=-1,pullsock=-1,pushsock=-1; long fsize; struct privatebet_info *BET,*BET2;
    hostip = "127.0.0.1";
    libgfshare_init();
    OS_init();
    portable_mutex_init(&LP_peermutex);
    portable_mutex_init(&LP_commandmutex);
    portable_mutex_init(&LP_networkmutex);
    portable_mutex_init(&LP_psockmutex);
    portable_mutex_init(&LP_messagemutex);
    portable_mutex_init(&BET_shardmutex);
    sleep(1);
    if ( (infojson= chipsln_getinfo()) != 0 )
    {
        if ( (LN_idstr= clonestr(jstr(infojson,"id"))) == 0 || strlen(LN_idstr) != 66 )
            printf("need 33 byte secp pubkey\n"), exit(-1);
        LN_port = juint(infojson,"port");
        printf("getinfo.(%s)\n",jprint(infojson,1));
    } else printf("need to have CHIPS and lightning running\n"), exit(-1);
    printf("help.(%s)\n",jprint(chipsln_help(),1));
    printf("LN_idstr.(%s)\n",LN_idstr);
    if ( argc > 1 )
    {
        if ( (argjson= cJSON_Parse(argv[1])) != 0 )
        {
            hostip = jstr(argjson,"hostip");
            if ( (tmp= juint(argjson,"hostport")) != 0 )
                port = tmp;
            if ( (tmp= juint(argjson,"rpcport")) != 0 )
                rpcport = tmp;
            if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)stats_rpcloop,(void *)&rpcport) != 0 )
            {
                printf("error launching stats rpcloop for port.%u\n",port);
                exit(-1);
            }
            if ( (modestr= jstr(argjson,"mode")) != 0 )
            {
                if ( strcmp(modestr,"host") == 0 )
                {
                    if ( hostip == 0 && system("curl -s4 checkip.amazonaws.com > /tmp/myipaddr") == 0 )
                    {
                        if ( (hostip= OS_filestr(&fsize,"/tmp/myipaddr")) != 0 && hostip[0] != 0 )
                        {
                            n = (int32_t)strlen(hostip);
                            if ( hostip[n-1] == '\n' )
                                hostip[--n] = 0;
                        } else printf("error getting myipaddr\n");
                    }
                    BET_transportname(1,bindaddr,hostip,port);
                    pubsock = BET_nanosock(1,bindaddr,NN_PUB);
                    BET_transportname(1,bindaddr,hostip,port+1);
                    pullsock = BET_nanosock(1,bindaddr,NN_PULL);
                    IAMHOST = 1;
                    safecopy(Host_peerid,LN_idstr,sizeof(Host_peerid));
                    safecopy(Host_ipaddr,hostip,sizeof(Host_ipaddr));
                    // publish to BET chain
                }
                else if ( strcmp(modestr,"oracle") == 0 )
                {
                    IAMORACLE = 1;
                    while ( 1 )     // just respond to oracle requests
                        sleep(777);
                }
            }
            printf("BET API running on %s:%u pub.%d sub.%d; pull.%d push.%d\n",hostip,port,pubsock,subsock,pullsock,pushsock);
            BET = calloc(1,sizeof(*BET));
            BET2 = calloc(1,sizeof(*BET2));
            BET->pubsock = pubsock;
            BET->pullsock = pullsock;
            BET->subsock = subsock;
            BET->pushsock = pushsock;
            BET->maxplayers = (Maxplayers < CARDS777_MAXPLAYERS) ? Maxplayers : CARDS777_MAXPLAYERS;
            BET->maxchips = CARDS777_MAXCHIPS;
            BET->chipsize = CARDS777_CHIPSIZE;
            *BET2 = *BET;
            if ( passphrase == 0 || passphrase[0] == 0 )
            {
                FILE *fp;
                if ( (fp= fopen("passphrase","rb")) == 0 )
                {
                    OS_randombytes((void *)&randvals,sizeof(randvals));
                    sprintf(randphrase,"%llu",(long long)randvals);
                    printf("randphrase.(%s)\n",randphrase);
                    if ( (fp= fopen("passphrase","wb")) != 0 )
                    {
                        fwrite(randphrase,1,strlen(randphrase),fp);
                        fclose(fp);
                    }
                    passphrase = randphrase;
                }
                else
                {
                    printf("found passphrase file\n");
                    fread(randphrase,1,sizeof(randphrase),fp);
                    passphrase = randphrase;
                    fclose(fp);
                }
            }
            printf("passphrase.(%s) pushsock.%d subsock.%d hostip.(%s)\n",passphrase,pushsock,subsock,hostip);
            conv_NXTpassword(privkey.bytes,pubkey.bytes,(uint8_t *)passphrase,(int32_t)strlen(passphrase));
            bitcoin_priv2pub(bitcoin_ctx(),pubkey33,smartaddr,privkey,taddr,pubtype);
            Mypubkey = pubkey;
            Myprivkey = privkey;
            if ( IAMHOST != 0 )
            {
                BET_betinfo_set(BET,"demo",36,0,Maxplayers);
                if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)BET_hostloop,(void *)BET) != 0 )
                {
                    printf("error launching BET_hostloop for pub.%d pull.%d\n",BET->pubsock,BET->pullsock);
                    exit(-1);
                }
            }
            else
            {
                if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)BET_clientloop,(void *)BET) != 0 )
                {
                    printf("error launching BET_clientloop for sub.%d\n",BET->subsock);
                    exit(-1);
                }
            }
            while ( 1 )
            {
                sleep(1);
                // update display
            }
            //BET_cmdloop(privkey,smartaddr,pubkey33,pubkey,BET2);
            /*if ( hostip == 0 || hostip[0] == 0 )
             hostip = "127.0.0.1";
             BET_transportname(0,connectaddr,hostip,port);
             printf("connect %s\n",connectaddr);
             subsock = BET_nanosock(0,connectaddr,NN_SUB);
             BET_transportname(0,connectaddr,hostip,port+1);
             pushsock = BET_nanosock(0,connectaddr,NN_PUSH);
             sleep(1);*/
            // printf("connect.(%s)\n",jprint(chipsln_connect(hostip,port,LN_idstr),1));
            //BET_mainloop(pubsock,pullsock,subsock,pushsock,jstr(argjson,"passphrase"));
        }
    }
    else
    {
        printf("no argjson, default to testmode\n");
        while ( testmode != 0 )
        {
            OS_randombytes((uint8_t *)&range,sizeof(range));
            OS_randombytes((uint8_t *)&numplayers,sizeof(numplayers));
            range = (range % CARDS777_MAXCARDS) + 1;
            numplayers = (numplayers % (CARDS777_MAXPLAYERS-1)) + 2;
            for (i=0; i<numplayers; i++)
                privkeys[i] = curve25519_keypair(&pubkeys[i]);
            //Debug_privkeys = privkeys;
            deckjson = 0;
            if ( (reqjson= BET_createdeck_request(pubkeys,numplayers,range)) != 0 )
            {
                if ( (retstr= BET_oracle_request("createdeck",reqjson)) != 0 )
                {
                    if ( (deckjson= cJSON_Parse(retstr)) != 0 )
                    {
                        printf("BET_roundstart numcards.%d numplayers.%d\n",range,numplayers);
                        BET_roundstart(-1,deckjson,range,privkeys,pubkeys,numplayers,privkeys[0]);
                        printf("finished BET_roundstart numcards.%d numplayers.%d\n",range,numplayers);
                    }
                    free(retstr);
                }
                free_json(reqjson);
            }
            if ( deckjson != 0 )
                free_json(deckjson);
            {
                int32_t permi[CARDS777_MAXCARDS],permis[CARDS777_MAXPLAYERS][CARDS777_MAXCARDS];
                memset(permi,0,sizeof(permi));
                memset(permis,0,sizeof(permis));
                for (i=0; i<numplayers; i++)
                    BET_permutation(permis[i],range);
                BET_permutation_sort(permi,permis,numplayers,range);
            }
        }
    }
    sleep(1);
    return 0;
}

