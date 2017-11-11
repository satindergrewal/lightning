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

// jl777: oracle needs to include other data like deckid also, timestamp! thanks cryptographer
// dealer needs to timestamp and sign
// players need to sign their actions and gameeval
// deterministic sort
// new method for layered dealing, old method for layered shuffle
//libscott [11:08 PM]
//the observer is the chain. the state machine doesnt need to be executed on chain, but the HEAD state of the game should be notarised on a regular basis
//considering the case where the dealer uniquely generates the blinding value for each card and generates the M of N shard of it and distributes it among the players...


//[9:09]
//to get to know the card at any given time the player must know atleast M shards from it's peers..

//[11:08]
//Ie, it's the responsibility of each player to notarise that state after each move is made

// redo unpaid deletes
//  from external: git submodule add https://github.com/ianlancetaylor/libbacktrace.git

#include "bet.h"  
char *LN_idstr,Host_ipaddr[64],Host_peerid[67],BET_ORACLEURL[64] = "127.0.0.1:7797";
uint16_t LN_port;
int32_t Gamestart,Gamestarted,Lastturni,Maxrounds = 3,Maxplayers = 10;
uint8_t BET_logs[256],BET_exps[510];
bits256 *Debug_privkeys;
struct BET_shardsinfo *BET_shardsinfos;
portable_mutex_t LP_gcmutex,LP_peermutex,LP_commandmutex,LP_networkmutex,LP_psockmutex,LP_messagemutex,BET_shardmutex;
int32_t LP_canbind,IAMLP,IAMHOST,IAMORACLE;
struct LP_peerinfo  *LP_peerinfos,*LP_mypeer;
bits256 Mypubkey,Myprivkey,Clientrhash,Hostrhashes[CARDS777_MAXPLAYERS+1];
char Host_channel[64];
struct rpcrequest_info *LP_garbage_collector;
int32_t permis_d[CARDS777_MAXCARDS],permis_b[CARDS777_MAXCARDS];
bits256 *allshares=NULL;
uint8_t sharenrs[256];

char *issue_LP_psock(char *destip,uint16_t destport,int32_t ispaired)
{
    char url[512],*retstr;
    sprintf(url,"http://%s:%u/api/stats/psock?ispaired=%d",destip,destport-1,ispaired);
    //return(LP_issue_curl("psock",destip,destport,url));
    retstr = issue_curlt(url,LP_HTTP_TIMEOUT*3);
    printf("issue_LP_psock got (%s) from %s\n",retstr,destip);
    return(retstr);
}
int32_t LP_numpeers()
{
    printf("this needs to be fixed\n");
    return(9);
}
struct LP_millistats
{
    double lastmilli,millisum,threshold;
    uint32_t count;
    char name[64];
} LP_psockloop_stats,LP_reserved_msgs_stats,utxosQ_loop_stats,command_rpcloop_stats,queue_loop_stats,prices_loop_stats,LP_coinsloop_stats,LP_coinsloopBTC_stats,LP_coinsloopKMD_stats,LP_pubkeysloop_stats,LP_privkeysloop_stats,LP_swapsloop_stats,LP_gcloop_stats;

void LP_millistats_update(struct LP_millistats *mp)
{
    double elapsed,millis;
    if ( mp == 0 )
    {
        if ( IAMLP != 0 )
        {
            mp = &LP_psockloop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        }
        mp = &LP_reserved_msgs_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &utxosQ_loop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &command_rpcloop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &queue_loop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &prices_loop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &LP_coinsloop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &LP_coinsloopBTC_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &LP_coinsloopKMD_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &LP_pubkeysloop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &LP_privkeysloop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &LP_swapsloop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
        mp = &LP_gcloop_stats, printf("%32s lag %10.2f millis, threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,OS_milliseconds() - mp->lastmilli,mp->threshold,mp->millisum/(mp->count > 0 ? mp->count: 1),mp->count);
    }
    else
    {
        if ( mp->lastmilli == 0. )
            mp->lastmilli = OS_milliseconds();
        else
        {
            mp->count++;
            millis = OS_milliseconds();
            elapsed = (millis - mp->lastmilli);
            mp->millisum += elapsed;
            if ( mp->threshold != 0. && elapsed > mp->threshold )
            {
                //if ( IAMLP == 0 )
                printf("%32s elapsed %10.2f millis > threshold %10.2f, ave %10.2f millis, count.%u\n",mp->name,elapsed,mp->threshold,mp->millisum/mp->count,mp->count);
            }
            mp->lastmilli = millis;
        }
    }
}

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


int32_t players_init(int32_t numplayers,int32_t numcards,bits256 deckid);


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
    if ( argc > 1 )
    {
        if ( (infojson= chipsln_getinfo()) != 0 )
        {
            if ( (LN_idstr= clonestr(jstr(infojson,"id"))) == 0 || strlen(LN_idstr) != 66 )
                printf("need 33 byte secp pubkey\n"), exit(-1);
            LN_port = juint(infojson,"port");
            printf("getinfo.(%s)\n",jprint(infojson,1));
        } else printf("need to have CHIPS and lightning running\n"), exit(-1);
        printf("help.(%s)\n",jprint(chipsln_help(),1));
        printf("LN_idstr.(%s)\n",LN_idstr);
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
        while ( testmode != 1 )
        {
        	testmode=1;
            OS_randombytes((uint8_t *)&range,sizeof(range));
            OS_randombytes((uint8_t *)&numplayers,sizeof(numplayers));
            range = (range % CARDS777_MAXCARDS) + 1;
            numplayers = (numplayers % (CARDS777_MAXPLAYERS-1)) + 2;
            players_init(numplayers,numplayers,rand256(0));
            continue;
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

struct pair256 { bits256 priv,prod; };

bits256 curve25519_fieldelement(bits256 hash)
{
    hash.bytes[0] &= 0xf8, hash.bytes[31] &= 0x7f, hash.bytes[31] |= 0x40;
    return(hash);
}

bits256 card_rand256(int32_t privkeyflag,int8_t index)
{
    bits256 randval;
    OS_randombytes(randval.bytes,sizeof(randval));
    if ( privkeyflag != 0 )
        randval.bytes[0] &= 0xf8, randval.bytes[31] &= 0x7f, randval.bytes[31] |= 0x40;
    randval.bytes[30] = index;
    return(randval);
}

struct pair256 deckgen_common(struct pair256 *randcards,int32_t numcards)
{
    int32_t i; struct pair256 key,tmp; bits256 basepoint;
    basepoint = curve25519_basepoint9();
    key.priv = rand256(1), key.prod = fmul_donna(key.priv,basepoint);
    for (i=0; i<numcards; i++)
    {
        tmp.priv = card_rand256(1,i);
        tmp.prod = fmul_donna(tmp.priv,basepoint);
        randcards[i] = tmp;
    }
    return(key);
}

void dekgen_vendor_perm(int numcards)
{
	 BET_permutation(permis_d,numcards);
		
}
void blinding_vendor_perm(int numcards)
{
	BET_permutation(permis_b,numcards);
	 
}
struct pair256 deckgen_player(bits256 *playerprivs,bits256 *playercards,int32_t *permis,int32_t numcards)
{
    int32_t i; struct pair256 key,randcards[256];
    key = deckgen_common(randcards,numcards);
    BET_permutation(permis,numcards);
    for (i=0; i<numcards; i++)
    {
        playerprivs[i] = randcards[permis[i]].priv;
        playercards[i] = fmul_donna(playerprivs[i],key.prod);
    }
    return(key);
}

void deckgen_vendor(bits256 *cardprods,bits256 *finalcards,int32_t numcards,bits256 *playercards,bits256 deckid) // given playercards[], returns cardprods[] and finalcards[]
{
    static struct pair256 randcards[256]; static bits256 active_deckid;
    int32_t i,permis[256]; bits256 hash,xoverz,tmp[256];
    if ( bits256_cmp(deckid,active_deckid) != 0 )
        deckgen_common(randcards,numcards);
    for (i=0; i<numcards; i++)
    {
        xoverz = xoverz_donna(fmul_donna(playercards[i],randcards[i].priv));
        vcalc_sha256(0,hash.bytes,xoverz.bytes,sizeof(xoverz));
        tmp[i] = fmul_donna(curve25519_fieldelement(hash),randcards[i].priv);
    }
	
   // BET_permutation(permis,numcards);
    for (i=0; i<numcards; i++)
    {
        finalcards[i] = tmp[permis_d[i]];
        cardprods[i] = randcards[i].prod; // same cardprods[] returned for each player
    }
}

void blinding_vendor(bits256 *blindings,bits256 *blindedcards,bits256 *finalcards,int32_t numcards,int32_t numplayers,int32_t playerid,bits256 deckid)
{
    //static bits256 *allshares;
    int32_t i,j,k,M,permi,permis[256]; uint8_t space[8192]; bits256 *cardshares;
    //BET_permutation(permis,numcards);
    
    for (i=0; i<numcards; i++)
    {
        blindings[i] = rand256(1);
        blindedcards[i] = fmul_donna(finalcards[permis_b[i]],blindings[i]);
    }
    if ( 1 ) // for later
    {
        M = (numplayers/2) + 1;
        gfshare_calc_sharenrs(sharenrs,numplayers,deckid.bytes,sizeof(deckid)); // same for all players for this round
        cardshares = calloc(numplayers,sizeof(bits256));
        if ( allshares == 0 )
            allshares = calloc(numplayers,sizeof(bits256) * numplayers * numcards);
		printf("\nplayer:%d",playerid);
        for (i=0; i<numcards; i++)
        {
        	printf("\ncard:%d",i);
            gfshare_calc_shares(cardshares[0].bytes,blindings[i].bytes,sizeof(bits256),sizeof(bits256),M,numplayers,sharenrs,space,sizeof(space));
            // create combined allshares
            for (j=0; j<numplayers; j++) {
                allshares[j*numplayers*numcards + (i*numplayers + playerid)] = cardshares[j];
				printf("\nshare:%d\n",j);
				for(k=0;k<32;k++)
				{
					printf("%d ",allshares[j*numplayers*numcards + (i*numplayers + playerid)].bytes[k]);
				}
			}
			
        }
		// when all players have submitted their finalcards, blinding vendor can send encrypted allshares for each player, see cards777.c
    }
}

bits256 player_decode(int32_t playerid,int32_t cardID,int numplayers,struct pair256 key,bits256 blindingval,bits256 blindedcard,bits256 *cardprods,bits256 *playerprivs,int32_t *permis,int32_t numcards)
{
    bits256 tmp,xoverz,hash,fe,decoded,refval,basepoint,*cardshares; int32_t i,j,k,unpermi; char str[65];uint8_t space[8192];
	struct gfshare_ctx *G;
	uint8_t *recover=NULL;
	basepoint = curve25519_basepoint9();

	
	cardshares = calloc(numplayers,sizeof(bits256));
		printf("\nPlayer:%d:card:%d",playerid,cardID);
			for (j=0; j<numplayers; j++) 
			{
				cardshares[j]=allshares[j*numplayers*numcards + (cardID*numplayers + playerid)];
        		printf("\nshare:%d\n",j);
				for(k=0;k<32;k++)
				{
					printf("%d ",cardshares[j].bytes[k]);
				}
			}
	
		
	 G = gfshare_initdec(sharenrs,numplayers,sizeof(bits256),space,sizeof(space));
    for (i=0; i<numplayers; i++)
            gfshare_dec_giveshare(G,i,cardshares[i].bytes);
    //gfshare_dec_newshares(G,recovernrs);
	printf("\nsharecount:%d,size:%d",G->sharecount,G->size);

	//gfshare_decextract(0,0,G,recover);
    //gfshare_free(G);
	
   
	refval = fmul_donna(blindedcard,crecip_donna(blindingval));
	#if 0  
	for (i=0; i<numcards; i++)
    {
        /*unpermi = -1;
        for (j=0; j<numcards; j++)
            if ( permis[j] == i )
            {
                unpermi = j;
				
                break;
            }
        if ( unpermi < 0 )
        {
            printf("couldnt find unpermi for %d\n",i);
            memset(tmp.bytes,0,sizeof(tmp));
            return(tmp);
        }*/
        //printf("i.%d unpermi.%d vs %d\n",i,unpermi,permis[i]);
      
        for (j=0; j<numcards; j++)
        {
            tmp = fmul_donna(playerprivs[i],cardprods[j]);
            tmp = fmul_donna(tmp,key.priv);
            xoverz = xoverz_donna(tmp);
            vcalc_sha256(0,hash.bytes,xoverz.bytes,sizeof(xoverz));
            fe = crecip_donna(curve25519_fieldelement(hash));
            decoded = fmul_donna(fmul_donna(refval,fe),basepoint);
            if ( bits256_cmp(decoded,cardprods[j]) == 0 )
            {
                printf("player.%d decoded card %s value %d\n",playerid,bits256_str(str,decoded),playerprivs[i].bytes[30]);
                return(playerprivs[i]);
            }
        }
    }
    printf("couldnt decode blindedcard %s\n",bits256_str(str,blindedcard));
    memset(tmp.bytes,0,sizeof(tmp));
	#endif
    return(tmp);
}

int32_t player_init(uint8_t *decoded,bits256 *playerprivs,bits256 *playercards,int32_t *permis,int32_t playerid,int32_t numplayers,int32_t numcards,bits256 deckid)
{
    int32_t i,j,k,errs,unpermi; struct pair256 key; bits256 decoded256,cardprods[256],finalcards[256],blindingvals[256],blindedcards[256];
	key = deckgen_player(playerprivs,playercards,permis,numcards);
	deckgen_vendor(cardprods,finalcards,numcards,playercards,deckid); // over network

	blinding_vendor(blindingvals,blindedcards,finalcards,numcards,numplayers,playerid,deckid); // over network
	
	
	#if 1
	memset(decoded,0xff,numcards);
    for (errs=i=0; i<numcards; i++)
    {
        decoded256 = player_decode(playerid,i,numplayers,key,blindingvals[i],blindedcards[i],cardprods,playerprivs,permis,numcards);
        if ( bits256_nonz(decoded256) == 0 )
            errs++;
        else
       	{
       		unpermi=-1;
       		for(j=0;j<numcards;j++){
				if(permis[j]==decoded256.bytes[30]){
					unpermi=j;
					break;
				}
			}
       		decoded[i] = j;    	
	   	}
			
    }
	#endif
    return(errs);
}

int32_t players_init(int32_t numplayers,int32_t numcards,bits256 deckid)
{
    static int32_t decodebad,decodegood;
    int32_t i,j,k,playerid,errs,playererrs,good,bad,permis[CARDS777_MAXPLAYERS][256]; uint8_t decoded[CARDS777_MAXPLAYERS][256]; bits256 playerprivs[CARDS777_MAXPLAYERS][256],playercards[CARDS777_MAXPLAYERS][256]; char str[65];
	
	dekgen_vendor_perm(numcards);
	blinding_vendor_perm(numcards);
	numplayers=2;numcards=2;
	printf("\nNumber of players:%d, Number of cards:%d",numplayers,numcards);
	allshares = calloc(numplayers,sizeof(bits256) * numplayers * numcards);
	
	for (playererrs=playerid=0; playerid<numplayers; playerid++)
    {
        if ( (errs= player_init(decoded[playerid],playerprivs[playerid],playercards[playerid],permis[playerid],playerid,numplayers,numcards,deckid)) != 0 )
        {
            //printf("playerid.%d got errors %d for deckid.%s\n",playerid,errs,bits256_str(str,deckid));
            playererrs++;
        }
        decodebad += errs;
        decodegood += (numcards - errs);
    }

	
	
	
	#if 0
	for (good=bad=i=0; i<numplayers-1; i++)
    {
        for (j=i+1; j<numplayers; j++)
        {
            if ( memcmp(decoded[i],decoded[j],numcards) != 0 )
            {
                printf("decoded cards mismatch between player %d and %d\n",i,j);
                bad++;
            } good++;
        }
    }
    printf("numplayers.%d numcards.%d deck %s -> playererrs.%d good.%d bad.%d decode.[good %d, bad %d]\n",numplayers,numcards,bits256_str(str,deckid),playererrs,good,bad,decodegood,decodebad);
	#endif
	return(playererrs);
}
