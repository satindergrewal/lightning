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

char *chipsln_command(void *ctx,cJSON *argjson,char *remoteaddr,uint16_t port)
{
    cJSON *array,*item; int32_t i,n,numargs,maxsize = 1000000; char *args[16],*buffer = malloc(maxsize);
    memset(args,0,sizeof(args));
    numargs = 0;
    args[numargs++] = "chipsln";
    args[numargs++] = jstr(argjson,"method");
    if ( (array= jarray(&n,argjson,"params")) != 0 )
    {
        for (i=0; i<n; i++)
        {
            item = jitem(array,i);
            args[numargs++] = jprint(item,0);
        }
    }
    //for (i=0; i<numargs; i++)
    //    printf("(%s) ",args[i]);
    //printf(" <- %s\n",jprint(argjson,0));
    cli_main(buffer,maxsize,numargs,args,0);
    if ( numargs > 2 )
    {
        for (i=2; i<numargs; i++)
            if ( args[i] != 0 )
                free(args[i]);
    }
    n = (int32_t)strlen(buffer);
    if ( buffer[n-1] == '\n' )
        buffer[n-1] = 0;
    buffer = realloc(buffer,n+1);
    return(buffer);
}

cJSON *chipsln_issue(char *buf)
{
    char *retstr; cJSON *retjson,*argjson;
    argjson = cJSON_Parse(buf);
    //printf("parse.(%s) <- %s\n",jprint(argjson,0),buf);
    if ( (retstr= chipsln_command(0,argjson,"127.0.0.1",0)) != 0 )
    {
        retjson = cJSON_Parse(retstr);
        free(retstr);
    }
    free_json(argjson);
    return(retjson);
}

cJSON *chipsln_noargs(char *method)
{
    char buf[1024];
    sprintf(buf,"{ \"method\":\"%s\", \"id\":\"chipsln-%d\", \"params\":[] }",method,getpid());
    return(chipsln_issue(buf));
}

cJSON *chipsln_strarg(char *method,char *str)
{
    char buf[4096];
    sprintf(buf,"{ \"method\":\"%s\", \"id\":\"chipsln-%d\", \"params\":[\"%s\"] }",method,getpid(),str);
    return(chipsln_issue(buf));
}

cJSON *chipsln_strnum(char *method,char *str,uint64_t num)
{
    char buf[4096];
    sprintf(buf,"{ \"method\":\"%s\", \"id\":\"chipsln-%d\", \"params\":[\"%s\", %llu] }",method,getpid(),str,(long long)num);
    return(chipsln_issue(buf));
}

cJSON *chipsln_numstr(char *method,uint64_t num,char *str)
{
    char buf[4096];
    sprintf(buf,"{ \"method\":\"%s\", \"id\":\"chipsln-%d\", \"params\":[%llu, \"%s\"] }",method,getpid(),(long long)num,str);
    return(chipsln_issue(buf));
}

cJSON *chipsln_getinfo() { return(chipsln_noargs("getinfo")); }
cJSON *chipsln_help() { return(chipsln_noargs("help")); }
cJSON *chipsln_stop() { return(chipsln_noargs("stop")); }
cJSON *chipsln_newaddr() { return(chipsln_noargs("newaddr")); }
cJSON *chipsln_getnodes() { return(chipsln_noargs("getnodes")); }
cJSON *chipsln_getpeers() { return(chipsln_noargs("getpeers")); }
cJSON *chipsln_getchannels() { return(chipsln_noargs("getchannels")); }
cJSON *chipsln_devblockheight() { return(chipsln_noargs("dev-blockheight")); }

cJSON *chipsln_listinvoice(char *label) { return(chipsln_strarg("listinvoice",label)); }
cJSON *chipsln_delinvoice(char *label) { return(chipsln_strarg("delinvoice",label)); }
cJSON *chipsln_waitanyinvoice(char *label) { return(chipsln_strarg("waitanyinvoice",label)); }
cJSON *chipsln_waitinvoice(char *label) { return(chipsln_strarg("waitinvoice",label)); }

cJSON *chipsln_getlog(char *level) { return(chipsln_strarg("getlog",level)); }
cJSON *chipsln_close(char *idstr) { return(chipsln_strarg("close",idstr)); }
cJSON *chipsln_devrhash(char *secret) { return(chipsln_strarg("dev-rhash",secret)); }
cJSON *chipsln_addfunds(char *rawtx) { return(chipsln_strarg("addfunds",rawtx)); }

cJSON *chipsln_fundchannel(char *idstr,uint64_t satoshi)
{
    return(chipsln_strnum("fundchannel",idstr,satoshi));
}

cJSON *chipsln_invoice(uint64_t msatoshi,char *label)
{
    return(chipsln_numstr("invoice",msatoshi,label));
}

bits256 chipsln_rhash_create(uint64_t satoshis,char *label)
{
    cJSON *inv,*array; bits256 rhash;
    if ( label == 0 )
        label = "";
    memset(rhash.bytes,0,sizeof(rhash));
    if ( (array= chipsln_listinvoice(label)) != 0 )
    {
        if ( cJSON_GetArraySize(array) == 1 )
        {
            inv = jitem(array,0);
            rhash = jbits256(inv,"rhash");
            //char str[65]; printf("list invoice.(%s) (%s) -> %s\n",label,jprint(inv,0),bits256_str(str,rhash));
            if ( bits256_nonz(rhash) != 0 )
            {
                free_json(array);
                return(rhash);
            }
        }
        free_json(array);
    }
    if ( (inv= chipsln_invoice(satoshis * 1000,label)) != 0 )
    {
        rhash = jbits256(inv,"rhash");
        char str[65]; printf("rhash.(%s) -> %s\n",jprint(inv,0),bits256_str(str,rhash));
        free_json(inv);
    }
    return(rhash);
}

cJSON *chipsln_withdraw(uint64_t satoshi,char *address)
{
    return(chipsln_numstr("withdraw",satoshi,address));
}

cJSON *chipsln_getroute(char *idstr,uint64_t msatoshi)
{
    char buf[4096];
    sprintf(buf,"{ \"method\":\"getroute\", \"id\":\"chipsln-%d\", \"params\":[\"%s\", %llu, 1] }",getpid(),idstr,(long long)msatoshi);
    return(chipsln_issue(buf));
}

cJSON *chipsln_connect(char *ipaddr,uint16_t port,char *destid)
{
    char buf[4096];
    sprintf(buf,"{ \"method\":\"connect\", \"id\":\"chipsln-%d\", \"params\":[\"%s\", %u, \"%s\"] }",getpid(),ipaddr,port,destid);
    return(chipsln_issue(buf));
}

cJSON *chipsln_sendpay(cJSON *routejson,bits256 rhash)
{
    char buf[16384],str[65];
    sprintf(buf,"{ \"method\":\"sendpay\", \"id\":\"chipsln-%d\", \"params\":[%s, \"%s\"] }",getpid(),jprint(routejson,0),bits256_str(str,rhash));
    return(chipsln_issue(buf));
}

char *privatebet_command(void *ctx,cJSON *argjson,char *remoteaddr,uint16_t port)
{
    char *method;
    if ( (method= jstr(argjson,"method")) != 0 )
    {
        if ( IAMORACLE != 0 || (IAMHOST != 0 && strcmp(BET_ORACLEURL,"127.0.0.1:7797") == 0) )
            return(BET_oracle_command(ctx,method,argjson));
        else return(clonestr("{\"error\":\"not an ORACLE\"}"));
    } else return(clonestr("{\"error\":\"missing method\"}"));
}

char *pangea_command(void *ctx,cJSON *argjson,char *remoteaddr,uint16_t port)
{
    char *method;
    if ( (method= jstr(argjson,"method")) != 0 )
    {
        return(clonestr("{\"result\":\"success\", \"agent\":\"pangea\"}"));
    } else return(clonestr("{\"error\":\"missing method\"}"));
}

char *stats_JSON(void *ctx,char *myipaddr,int32_t mypubsock,cJSON *argjson,char *remoteaddr,uint16_t port)
{
    char *agent;
    //printf("stats_JSON(%s)\n",jprint(argjson,0));
    agent = jstr(argjson,"agent");
    if ( (agent= jstr(argjson,"agent")) != 0 )
    {
        if ( strcmp(agent,"bet") == 0 )
            return(privatebet_command(ctx,argjson,remoteaddr,port));
        else if ( strcmp(agent,"chipsln") == 0 )
            return(chipsln_command(ctx,argjson,remoteaddr,port));
        else if ( strcmp(agent,"pangea") == 0 )
            return(pangea_command(ctx,argjson,remoteaddr,port));
        else return(clonestr("{\"error\":\"invalid agent\"}"));
    }
    printf("stats_JSON.(%s)\n",jprint(argjson,0));
    return(clonestr("{\"result\":\"success\"}"));
}

int32_t BET_peer_state(char *peerid,char *statestr)
{
    cJSON *retjson,*array,*item; int32_t i,n,retval = -1;
    if ( (retjson= chipsln_getpeers()) != 0 )
    {
        if ( (array= jarray(&n,retjson,"peers")) != 0 && n > 0 )
        {
            for (i=0; i<n; i++)
            {
                item = jitem(array,i);
                if ( jstr(item,"peerid") == 0 || strcmp(jstr(item,"peerid"),Host_peerid) != 0 )
                    continue;
                if ( jstr(item,"state") != 0 && strcmp(jstr(item,"state"),statestr) == 0 )
                    retval = 0;
                break;
            }
        }
        free_json(retjson);
    }
    return(retval);
}

/*void BET_status_disp(struct privatebet_info *bet,struct privatebet_vars *vars)
{
    char str[65];
    printf("%s: mypubkey.(%s) playerid.%d numplayers.%d max.%d numrounds.%d round.%d\n",bet->game,bits256_str(str,Mypubkey),bet->myplayerid,bet->numplayers,bet->maxplayers,bet->numrounds,vars->round);
}

int32_t BET_havetable(bits256 pubkey,uint8_t *pubkey33,struct privatebet_info *bet)
{
    cJSON *reqjson;
    if ( bits256_nonz(bet->tableid) != 0 )
        return(1);
    else
    {
        reqjson = cJSON_CreateObject();
        jaddbits256(reqjson,"pubkey",pubkey);
        jaddstr(reqjson,"method","join");
        jaddstr(reqjson,"peerid",LN_idstr);
        BET_message_send("BET_havetable",bet->pushsock,reqjson,1,bet);
        return(-1);
    }
}

void BET_cmdloop(bits256 privkey,char *smartaddr,uint8_t *pubkey33,bits256 pubkey,struct privatebet_info *bet)
{
    while ( BET_havetable(pubkey,pubkey33,bet) < 0 )
        sleep(10);
    while ( 1 )
    {
        //BET_status_disp();
        sleep(10);
    }
}*/
