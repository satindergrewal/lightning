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

bits256 Host_rhashes[256]; int32_t Num_hostrhashes;

bits256 BET_clientrhash()
{
    return(Clientrhash);
}

void BET_chip_recv(char *label,struct privatebet_info *bet)
{
    cJSON *reqjson;
    if ( label != 0 && label[0] != 0 )
    {
        reqjson = cJSON_CreateObject();
        jaddstr(reqjson,"method","onechip");
        jaddstr(reqjson,"label",label);
        BET_message_send("BET_chip_recv",bet->pubsock>=0?bet->pubsock:bet->pushsock,reqjson,1,bet);
    }
}

void BET_hostrhash_update(bits256 rhash)
{
    int32_t i;
    char str[65]; printf("hostrhash update.(%s)\n",bits256_str(str,rhash));
    if ( Num_hostrhashes < sizeof(Host_rhashes)/sizeof(*Host_rhashes) )
    {
        for (i=0; i<Num_hostrhashes; i++)
            if ( bits256_cmp(rhash,Host_rhashes[i]) == 0 )
                return;
        Host_rhashes[Num_hostrhashes++] = rhash;
    }
}

bits256 BET_hosthash_extract(cJSON *argjson,int32_t chipsize)
{
    cJSON *array; bits256 hash,hostrhashes[CARDS777_MAXPLAYERS+1]; int32_t i,n;
    if ( (array= jarray(&n,argjson,"hostrhash")) != 0 )
    {
        for (i=0; i<n; i++)
            hostrhashes[i] = jbits256i(array,i);
        if ( (array= jarray(&n,argjson,"pubkeys")) != 0 && chipsize == jint(argjson,"chipsize") )
        {
            for (i=0; i<n; i++)
            {
                hash = jbits256i(array,i);
                if ( bits256_cmp(hash,Mypubkey) == 0 && Host_peerid[0] != 0 && Host_channel[0] != 0 )
                {
                    //char str[65]; printf("BET_clientpay.[%d] %s\n",i,bits256_str(str,Hostrhashes[i]));
                    //BET_clientpay(hostrhashes[i],bet->chipsize);
                    BET_hostrhash_update(hostrhashes[i]);
                    return(hostrhashes[i]);
                }
            }
        }
    }
    memset(hash.bytes,0,sizeof(hash));
    return(hash);
}

int32_t BET_clientpay(uint64_t chipsize)
{
    bits256 rhash,preimage; cJSON *routejson,*retjson,*array; int32_t n,retval = -1;
    if ( Host_channel[0] != 0 && (n= Num_hostrhashes) > 0 )
    {
        rhash = Host_rhashes[n-1];
        if ( bits256_nonz(rhash) != 0 )
        {
            array = cJSON_CreateArray();
            routejson = cJSON_CreateObject();
            jaddstr(routejson,"id",Host_peerid);
            jaddstr(routejson,"channel",Host_channel);
            jaddnum(routejson,"msatoshi",chipsize*1000);
            jaddnum(routejson,"delay",10);
            jaddi(array,routejson);
            // route { "id" : "02779b57b66706778aa1c7308a817dc080295f3c2a6af349bb1114b8be328c28dc", "channel" : "27446:1:0", "msatoshi" : 1000000, "delay" : 10 }
            // replace rhash in route
            if ( (retjson= chipsln_sendpay(array,rhash)) != 0 )
            {
                char str[65],str2[65];
                preimage = jbits256(retjson,"preimage");
                printf("sendpay rhash.(%s) %.8f to %s -> %s preimage.%s\n",bits256_str(str,rhash),dstr(chipsize),jprint(array,0),jprint(retjson,0),bits256_str(str2,preimage));
                // if valid, reduce Host_rhashes[]
                if ( Num_hostrhashes > 0 )
                {
                    Num_hostrhashes--;
                    retval = 0;
                }
                free_json(retjson);
            }
            free_json(array);
        }
    }
    return(retval);
}

void BET_channels_parse()
{
    cJSON *channels,*array,*item; int32_t i,n,len; char *source,*dest,*short_id;
    if ( (channels= chipsln_getchannels()) != 0 )
    {
        //printf("got.(%s)\n",jprint(channels,0));
        if ( (array= jarray(&n,channels,"channels")) != 0 )
        {
            for (i=0; i<n; i++)
            {
                item = jitem(array,i);
                source = jstr(item,"source");
                dest = jstr(item,"destination");
                short_id = jstr(item,"short_id");
                //printf("source.%s dest.%s myid.%s Host.%s short.%s\n",source,dest,LN_idstr,Host_peerid,short_id);
                if ( source != 0 && dest != 0 && strcmp(source,LN_idstr) == 0 && strcmp(dest,Host_peerid) == 0 && short_id != 0 )
                {
                    len = strlen(short_id);
                    if ( len > 3 && short_id[len-2] == '/' )
                    {
                        strcpy(Host_channel,short_id);
                        Host_channel[len-2] = 0;
                        printf("Host_channel.(%s)\n",Host_channel);
                    }
                }
            }
        }
        free_json(channels);
    }
}
