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

int32_t BET_client_onechip(cJSON *argjson,struct privatebet_info *bet,struct privatebet_vars *vars,int32_t senderid)
{
    printf("client onechop.(%s)\n",jprint(argjson,0));
    return(0);
}

int32_t BET_client_gameeval(cJSON *argjson,struct privatebet_info *bet,struct privatebet_vars *vars,int32_t senderid)
{
    int32_t i,M,consensus = 0; uint32_t crc32 = 0;
    crc32 = juint(argjson,"crc32");
    vars->evalcrcs[senderid] = crc32;
    //printf("EVAL.(%u).p%d\n",crc32,senderid);
    M = (bet->numplayers >> 1) + 1;
    for (i=0; i<bet->numplayers; i++)
    {
        if ( vars->evalcrcs[i] != 0 && vars->evalcrcs[i] == crc32 )
            consensus++;
    }
    if ( consensus > vars->numconsensus )
        vars->numconsensus = consensus;
    if ( vars->consensus == 0 && consensus >= M )
    {
        vars->consensus = crc32;
        for (i=0; i<bet->numplayers; i++)
            printf("%u ",vars->evalcrcs[i]);
        printf("CONSENSUS.%d\n",consensus);
        BET_statemachine_consensus(bet,vars);
    }
    return(0);
}

int32_t BET_client_join(cJSON *argjson,struct privatebet_info *bet,struct privatebet_vars *vars,int32_t senderid)
{
    cJSON *array,*pubkeys,*retjson,*channels,*item; int32_t i,n,flag,already_connected=0,len,err=0; bits256 hash; char *idstr,*source,*dest,*short_id,*rstr;
    printf("BET_client_join\n");
    if ( jstr(argjson,"hostid") != 0 )
    {
        safecopy(Host_peerid,jstr(argjson,"hostid"),sizeof(Host_peerid));
        printf("BET_client_join %s\n",Host_peerid);
        if ( BET_peer_state(Host_peerid,"CHANNELD_NORMAL") == 0 )
        {
            already_connected = 1;
            printf("already connected\n");
        }
        else if ( (retjson= chipsln_connect(Host_ipaddr,LN_port,Host_peerid)) != 0 )
        {
            printf("(%s:%u %s) CONNECTLN.(%s)\n",Host_ipaddr,LN_port,Host_peerid,jprint(retjson,0));
            if ( (idstr= jstr(retjson,"id")) != 0 && strcmp(idstr,Host_peerid) == 0 )
                already_connected = 1;
            free_json(retjson);
        } else printf("null return from chipsln_connect\n");
        if ( already_connected != 0 )
        {
            BET_channels_parse();
            printf("Host_channel.(%s)\n",Host_channel);
            if ( Host_channel[0] == 0 || (int32_t)BET_peer_chipsavail(Host_peerid,bet->chipsize) < 2 )
            {
                if ( (retjson= chipsln_fundchannel(Host_peerid,CARDS777_MAXCHIPS*bet->chipsize)) != 0 )
                {
                    rstr = jprint(retjson,0);
                    if ( strcmp(LN_FUNDINGERROR,rstr) == 0 )
                    {
                        err = 1;
                        system("./fund");
                    }
                    printf("fundchannel -> (%s) err.%d\n",rstr,err);
                    free(rstr);
                    free_json(retjson);
                    if ( err == 0 && BET_peer_state(Host_peerid,"GOSSIPD") != 0 )
                    {
                        for (i=flag=0; i<10; i++)
                        {
                            if ( BET_peer_state(Host_peerid,"CHANNELD_AWAITING_LOCKIN") == 0 )
                            {
                                printf("waiting for CHANNELD_AWAITING_LOCKIN\n");
                                sleep(10);
                            } else break;
                        }
                        for (i=flag=0; i<10; i++)
                        {
                            if ( BET_peer_state(Host_peerid,"CHANNELD_NORMAL") != 0 )
                                sleep(10);
                            else
                            {
                                printf("channel is normal\n");
                                sleep(10);
                                break;
                            }
                        }
                        BET_channels_parse();
                        printf("new Host_channel.(%s)\n",Host_channel);
                    }
                }
            }
        }
    } else printf("no hostid in (%s)\n",jprint(argjson,0));
    BET_hosthash_extract(argjson,bet->chipsize);
    BET_clientpay(bet->chipsize);
    BET_statemachine_joined_table(bet,vars);
    printf("JOIN broadcast.(%s)\n",jprint(argjson,0));
    return(0);
}

int32_t BET_client_tablestatus(cJSON *msgjson,struct privatebet_info *bet,struct privatebet_vars *vars)
{
    BET_betinfo_parse(bet,vars,msgjson);
    /*if ( vars->turni == bet->myplayerid )
        BET_client_turnisend(bet,vars);*/
    return(0);
}

int32_t BET_client_gamestart(cJSON *argjson,struct privatebet_info *bet,struct privatebet_vars *vars)
{
    cJSON *cmdjson;
    cmdjson = cJSON_CreateObject();
    jaddstr(cmdjson,"method","started");
    jaddstr(cmdjson,"game",bet->game);
    jaddbits256(cmdjson,"tableid",bet->tableid);
    jaddnum(cmdjson,"numrounds",bet->numrounds);
    jaddnum(cmdjson,"range",bet->range);
    jaddbits256(cmdjson,"pubkey",Mypubkey);
    jadd(cmdjson,"actions",BET_statemachine_gamestart_actions(bet,vars));
    if ( bits256_nonz(vars->myhash) == 0 )
    {
        BET_permutation(vars->mypermi,bet->range);
        vcalc_sha256(0,vars->myhash.bytes,(uint8_t *)vars->mypermi,sizeof(*vars->mypermi) * bet->range);
    }
    jaddbits256(cmdjson,"hash",vars->myhash);
    BET_message_send("BET_gamestarted",bet->pushsock,cmdjson,1,bet);
    BET_statemachine_roundstart(bet,vars);
    return(0);
}

int32_t BET_client_gamestarted(cJSON *argjson,struct privatebet_info *bet,struct privatebet_vars *vars,int32_t senderid)
{
    int32_t i; cJSON *array,*reqjson; char str[65];
    if ( senderid >= 0 && senderid <= bet->numplayers )
    {
        vars->hashes[senderid][0] = jbits256(argjson,"hash");
        for (i=0; i<bet->numplayers; i++)
            if ( bits256_nonz(vars->hashes[i][0]) == 0 )
                break;
        if ( i == bet->numplayers )
        {
            array = cJSON_CreateArray();
            for (i=0; i<bet->range; i++)
                jaddinum(array,vars->mypermi[i]);
            reqjson = cJSON_CreateObject();
            jaddstr(reqjson,"method","perm");
            jadd(reqjson,"perm",array);
            jaddbits256(reqjson,"pubkey",Mypubkey);
            BET_message_send("BET_perm",bet->pubsock>=0?bet->pubsock:bet->pushsock,reqjson,1,bet);
        } //else printf("i.%d != num.%d senderid.%d process gamestarted.(%s) [sender.%d] <- %s\n",i,bet->numplayers,senderid,jprint(argjson,0),senderid,bits256_str(str,vars->hashes[senderid][0]));
    }
    return(0);
}

int32_t BET_client_perm(cJSON *argjson,struct privatebet_info *bet,struct privatebet_vars *vars,int32_t senderid)
{
    int32_t i,n,j; cJSON *array;
    //printf("got perm.(%s) sender.%d\n",jprint(argjson,0),senderid);
    if ( senderid >= 0 && senderid < bet->numplayers )
    {
        if ( (array= jarray(&n,argjson,"perm")) != 0 && n == bet->range )
        {
            for (i=0; i<bet->range; i++)
                vars->permis[senderid][i] = jinti(array,i);
            vcalc_sha256(0,vars->hashes[senderid][1].bytes,(uint8_t *)vars->permis[senderid],sizeof(*vars->permis[senderid]) * bet->range);
        }
        for (i=0; i<bet->numplayers; i++)
        {
            if ( bits256_cmp(vars->hashes[i][0],vars->hashes[i][1]) != 0 )
            {
                //char str[65],str2[65]; printf("%d: %s != %s\n",i,bits256_str(str,vars->hashes[i][0]),bits256_str(str2,vars->hashes[i][1]));
                break;
            }
        }
        if ( i == bet->numplayers )
        {
            j = BET_permutation_sort(vars->permi,vars->permis,bet->numplayers,bet->range);
            for (i=0; i<bet->range; i++)
                printf("%d ",vars->permi[i]);
            printf("validated perms best.%d\n",j);
            vars->roundready = 0;
            vars->turni = 0;
            vars->validperms = 1;
            /*if ( vars->turni == bet->myplayerid && vars->round == 0 )
                BET_client_turnisend(bet,vars);*/
        }
    }
    return(0);
}

int32_t BET_client_endround(cJSON *argjson,struct privatebet_info *bet,struct privatebet_vars *vars,int32_t senderid)
{
    int32_t i;
    if ( senderid >= 0 && senderid < bet->numplayers )
    {
        if ( vars->endround[senderid] == 0 )
            vars->endround[senderid] = (uint32_t)time(NULL);
        for (i=0; i<bet->numplayers; i++)
            if ( vars->endround[i] == 0 || vars->actions[vars->roundready][i] == 0 )
                break;
        if ( i == bet->numplayers )
        {
            vars->roundready++;
            BET_statemachine_roundstart(bet,vars);
            /*if ( vars->turni == bet->myplayerid && vars->round == vars->roundready && vars->round < bet->numrounds )
                BET_client_turnisend(bet,vars);*/
        }
    }
    else
    {
        printf(">>>>>>>> BET_client_endround.%d senderid.%d\n",vars->round,senderid);
        BET_statemachine_roundend(bet,vars);
    }
    return(0);
}

int32_t BET_client_MofN(cJSON *argjson,struct privatebet_info *bet,struct privatebet_vars *vars,int32_t senderid)
{
    int32_t cardi,playerj,recvlen; bits256 shard,*ptr; char *cipherstr; uint8_t decoded[sizeof(bits256) + 1024],encoded[sizeof(bits256) + 1024];
    cardi = jint(argjson,"cardi");
    playerj = jint(argjson,"playerj");
    memset(shard.bytes,0,sizeof(shard));
    if ( cardi >= 0 && cardi < bet->range && playerj >= 0 && playerj < bet->numplayers && senderid >= 0 && senderid < bet->numplayers && playerj == bet->myplayerid )
    {
        if ( (cipherstr= jstr(argjson,"cipher")) != 0 )
        {
            char str[65];
            recvlen = (int32_t)strlen(cipherstr) >> 1;
            decode_hex(encoded,recvlen,cipherstr);
            if ( (ptr= (bits256 *)BET_decrypt(decoded,sizeof(decoded),bet->playerpubs[senderid],Myprivkey,encoded,&recvlen)) == 0 )
                printf("decrypt error cardi.%d playerj.%d %s\n",cardi,playerj,bits256_str(str,Myprivkey));
            else shard = *ptr;
            //printf("cipherstr.(%s) -> %s\n",cipherstr,bits256_str(str,shard));
        } else shard = jbits256(argjson,"shard");
        //char str[65],str2[65]; printf("client MofN: cardi.%d playerj.%d %s shardi.%d bet->deckid.%s\n",cardi,playerj,bits256_str(str,shard),senderid,bits256_str(str2,bet->deckid));
        BET_MofN_item(bet->deckid,cardi,bet->cardpubs,bet->range,playerj,bet->numplayers,shard,senderid);
    }
    return(0);
}

int32_t BET_senderid(cJSON *argjson,struct privatebet_info *bet)
{
    int32_t i; char *peerid = jstr(argjson,"peerid");
    for (i=0; i<bet->numplayers; i++)
        if ( bits256_cmp(jbits256(argjson,"pubkey"),bet->playerpubs[i]) == 0 )
            return(i);
    if ( peerid != 0 && strcmp(Host_peerid,peerid) == 0 )
        return(bet->maxplayers);
    return(-1);
}

int32_t BET_clientupdate(cJSON *argjson,uint8_t *ptr,int32_t recvlen,struct privatebet_info *bet,struct privatebet_vars *vars) // update game state based on host broadcast
{
    static uint8_t *decoded; static int32_t decodedlen;
    char *method; int32_t senderid; bits256 *MofN;
    if ( (method= jstr(argjson,"method")) != 0 )
    {
        senderid = BET_senderid(argjson,bet);
        if ( IAMHOST == 0 )
            BET_hosthash_extract(argjson,bet->chipsize);
 //printf("BET_clientupdate: pushsock.%d subsock.%d method.%s sender.%d\n",bet->pushsock,bet->subsock,method,senderid);
        if ( strcmp(method,"tablestatus") == 0 )
            return(BET_client_tablestatus(argjson,bet,vars));
        else if ( strcmp(method,"turni") == 0 )
            return(BET_client_turni(argjson,bet,vars,senderid));
        else if ( strcmp(method,"onechip") == 0 )
            return(BET_client_onechip(argjson,bet,vars,senderid));
        else if ( strcmp(method,"roundend") == 0 )
            return(BET_client_endround(argjson,bet,vars,senderid));
        else if ( strcmp(method,"start0") == 0 )
        {
            bet->numrounds = jint(argjson,"numrounds");
            bet->numplayers = jint(argjson,"numplayers");
            bet->range = jint(argjson,"range");
            return(0);
        }
        else if ( strcmp(method,"start") == 0 )
            return(BET_client_gamestart(argjson,bet,vars));
        else if ( strcmp(method,"gameeval") == 0 )
            return(BET_client_gameeval(argjson,bet,vars,senderid));
        else if ( strcmp(method,"started") == 0 )
            return(BET_client_gamestarted(argjson,bet,vars,senderid));
        else if ( strcmp(method,"perm") == 0 )
            return(BET_client_perm(argjson,bet,vars,senderid));
        else if ( strcmp(method,"deali") == 0 )
            return(BET_client_deali(argjson,bet,vars,senderid));
        else if ( strcmp(method,"join") == 0 )
            return(BET_client_join(argjson,bet,vars,senderid));
        else if ( strcmp(method,"MofN") == 0 )
            return(BET_client_MofN(argjson,bet,vars,senderid));
        else if ( strcmp(method,"deckpacket") == 0 )
        {
            if ( decodedlen < recvlen )
            {
                decoded = realloc(decoded,recvlen);
                decodedlen = recvlen;
                printf("alloc decoded[%d]\n",recvlen);
            }
            if ( (MofN= BET_process_packet(bet->cardpubs,&bet->deckid,GENESIS_PUBKEY,Myprivkey,decoded,decodedlen,Mypubkey,ptr,recvlen,bet->numplayers,bet->range)) == 0 )
            {
                //printf("error processing packet, most likely not encrypted to us\n");
                return(0);
            }
            memcpy(bet->MofN,MofN,sizeof(*MofN) * bet->numplayers * bet->range);
            //int32_t i; char str[65]; for (i=0; i<bet->numplayers * bet->range; i++)
            //    printf("%s ",bits256_str(str,bet->MofN[i]));
            //printf("MofN.%s\n",bits256_str(str,bet->deckid));
            return(0);
        }
    } else printf("clientupdate unexpected.(%s)\n",jprint(argjson,0));
    return(-1);
}
void BET_clientloop(void *_ptr)
{
    uint32_t lasttime = 0; int32_t nonz,recvlen,lastChips_paid; uint16_t port=7798; char connectaddr[64],hostip[64]; void *ptr; cJSON *msgjson,*reqjson; struct privatebet_vars *VARS; struct privatebet_info *bet = _ptr;
    VARS = calloc(1,sizeof(*VARS));
    VARS->lastround = lastChips_paid = -1;
    strcpy(hostip,"5.9.253.195"); // jl777: get from BET blockchain
    printf("client loop: pushsock.%d subsock.%d\n",bet->pushsock,bet->subsock);
    sleep(5);
    while ( 1 )
    {
        if ( bet->subsock >= 0 && bet->pushsock >= 0 )
        {
            nonz = 0;
            if ( (recvlen= nn_recv(bet->subsock,&ptr,NN_MSG,0)) > 0 )
            {
                nonz++;
                if ( (msgjson= cJSON_Parse(ptr)) != 0 )
                {
                    if ( BET_clientupdate(msgjson,ptr,recvlen,bet,VARS) < 0 )
                        printf("unknown clientupdate msg.(%s)\n",jprint(msgjson,0));
                    if ( Num_hostrhashes > 0 && Chips_paid > lastChips_paid )
                    {
                        lastChips_paid = Chips_paid;
                        BET_clientpay(bet->chipsize);
                    }
                    free_json(msgjson);
                }
                nn_freemsg(ptr);
            }
            if ( nonz == 0 )
            {
                if ( time(NULL) > lasttime+60 )
                {
                    printf("%s round.%d turni.%d myid.%d | valid.%d roundready.%d lastround.%d -> myturn? %d\n",bet->game,VARS->round,VARS->turni,bet->myplayerid,VARS->validperms,VARS->roundready,VARS->lastround,VARS->validperms != 0 && VARS->turni == bet->myplayerid && VARS->roundready == VARS->round && VARS->lastround != VARS->round);
                    lasttime = (uint32_t)time(NULL);
                }
                usleep(10000);
                BET_statemachine(bet,VARS);
            }
        }
        else if ( hostip[0] != 0 && port > 0 )
        {
            BET_transportname(0,connectaddr,hostip,port);
            safecopy(Host_ipaddr,hostip,sizeof(Host_ipaddr));
            printf("connect %s\n",connectaddr);
            bet->subsock = BET_nanosock(0,connectaddr,NN_SUB);
            BET_transportname(0,connectaddr,hostip,port+1);
            bet->pushsock = BET_nanosock(0,connectaddr,NN_PUSH);
            reqjson = cJSON_CreateObject();
            jaddbits256(reqjson,"pubkey",Mypubkey);
            jaddstr(reqjson,"method","join");
            jaddstr(reqjson,"peerid",LN_idstr);
            Clientrhash = chipsln_rhash_create(bet->chipsize,"0");
            BET_message_send("BET_havetable",bet->pushsock,reqjson,1,bet);
        }
        else
        {
            // update list of tables
        }
    }
}

char *enc_share_str(char hexstr[177],struct enc_share x)
{
    int bytes=init_hexbytes_noT(hexstr,x.bytes,sizeof(x));
    return(hexstr);
}

bits256 BET_request()
{
	printf("\n%s:%d",__FUNCTION__,__LINE__);
}

bits256 BET_response()
{
	printf("\n%s:%d",__FUNCTION__,__LINE__);
}

bits256 BET_receive()
{
	printf("\n%s:%d",__FUNCTION__,__LINE__);

}


bits256 BET_request_share(int32_t ofCardID,int32_t ofPlayerID,struct privatebet_info *bet,bits256 bvv_public_key,struct pair256 player_key)
{
	cJSON *shareInfo=NULL;
	bits256 share;
	char *buf=NULL;
	char str[65];
	int bytes;
	int32_t forPlayerID;
	
	shareInfo=cJSON_CreateObject();
	cJSON_AddStringToObject(shareInfo,"messageid","request_share");
	cJSON_AddNumberToObject(shareInfo,"ofCardID",ofCardID);
	cJSON_AddNumberToObject(shareInfo,"ofPlayerID",ofPlayerID);
	cJSON_AddNumberToObject(shareInfo,"forPlayerID",bet->myplayerid);
	buf=cJSON_Print(shareInfo);
	bytes=nn_send(bet->pushsock,buf,strlen(buf),0);
	cJSON_Delete(shareInfo);
	shareInfo=cJSON_CreateObject();
	bytes=0;
	buf=NULL;
	while(1)
	{
		bytes=nn_recv(bet->subsock,&buf,NN_MSG,0);
		if(bytes>0)
		{
			shareInfo=cJSON_Parse(buf);
			if(0==strcmp(cJSON_str(cJSON_GetObjectItem(shareInfo,"messageid")),"response_share"))
			{
				share=jbits256(shareInfo,"share");
				break;
        	}
			else if(0==strcmp(cJSON_str(cJSON_GetObjectItem(shareInfo,"messageid")),"request_share"))
			{
				forPlayerID=jint(shareInfo,"forPlayerID");
				if(forPlayerID!=bet->myplayerid)
				{
					BET_give_share(shareInfo,bet,bvv_public_key,player_key);
				}
			}
		}
		sleep(5);
	}
	return share;
}

void BET_give_share(cJSON *shareInfo,struct privatebet_info *bet,bits256 bvv_public_key,struct pair256 player_key)
{
	int32_t ofCardID,ofPlayerID,forPlayerID;
	struct enc_share temp;
	char str[65],enc_str[177];
	bits256 share;
	uint8_t decipher[sizeof(bits256) + 1024],*ptr; int32_t recvlen;
	ofCardID=jint(shareInfo,"ofCardID");
	ofPlayerID=jint(shareInfo,"ofPlayerID");
	forPlayerID=jint(shareInfo,"forPlayerID");
	cJSON_Print(shareInfo);

	if((ofPlayerID==bet->myplayerid)&&(forPlayerID!=bet->myplayerid))
	{
        temp=g_shares[ofPlayerID*bet->numplayers*bet->range + (ofCardID*bet->numplayers + forPlayerID)];
	    recvlen = sizeof(temp);
		if ( (ptr= BET_decrypt(decipher,sizeof(decipher),bvv_public_key,player_key.priv,temp.bytes,&recvlen)) == 0 )
            printf("decrypt error ");
        else
        {
        	memcpy(share.bytes,ptr,recvlen);
			cJSON_Delete(shareInfo);
			shareInfo=cJSON_CreateObject();
			cJSON_AddStringToObject(shareInfo,"messageid","response_share");
			cJSON_AddNumberToObject(shareInfo,"ofCardID",ofCardID);
			cJSON_AddNumberToObject(shareInfo,"ofPlayerID",ofPlayerID);
			cJSON_AddNumberToObject(shareInfo,"forPlayerID",forPlayerID);
			jaddbits256(shareInfo,"share",share);
	   }
		if(bet->pushsock>=0){
			char *buf=NULL;
			buf=cJSON_Print(shareInfo);
			int bytes=nn_send(bet->pushsock,buf,strlen(buf),0);
		}	
	}
}


struct enc_share get_API_enc_share(cJSON *obj)
{
    struct enc_share hash; char *str;
	char hexstr[177];
    memset(hash.bytes,0,sizeof(hash));
   if ( obj != 0 )
    {
        if ( is_cJSON_String(obj) != 0 && (str= obj->valuestring) != 0 && strlen(str) == 176 ){
			
			decode_hex(hash.bytes,sizeof(hash),str);

        }
    }   

    return(hash);
}

void* BET_clientplayer(void * _ptr)
{
		static int32_t decodebad,decodegood,good,bad,errs;
    	int32_t unpermi,playererrs=0,decoded[CARDS777_MAXCARDS];
    	bits256 public_key_b;
		bits256 decoded256,temp,playerprivs[CARDS777_MAXCARDS],playercards[CARDS777_MAXCARDS],blindedcards[CARDS777_MAXPLAYERS][CARDS777_MAXCARDS],cardprods[CARDS777_MAXPLAYERS][CARDS777_MAXCARDS];
		int32_t permis[CARDS777_MAXCARDS],numcards,numplayers;
		struct pair256 key;struct privatebet_info *bet = _ptr;
		char str[65],share_str[177];
		cJSON *playerInfo,*gameInfo,*cjsonplayercards,*cjsonblindedcards,*cjsonshamirshards,*cjsoncardprods,*item;

		numplayers=bet->numplayers;
		numcards=bet->range;
		
		if ( bet->subsock >= 0 && bet->pushsock >= 0 )
		{
			key = deckgen_player(playerprivs,playercards,permis,numcards);
			playerInfo=cJSON_CreateObject();
			cJSON_AddStringToObject(playerInfo,"messageid","init");
			cJSON_AddNumberToObject(playerInfo,"playerid",bet->myplayerid);
			cJSON_AddNumberToObject(playerInfo,"range",bet->range);
			jaddbits256(playerInfo,"publickey",key.prod);
			cJSON_AddItemToObject(playerInfo,"playercards",cjsonplayercards=cJSON_CreateArray());
			for(int i=0;i<numcards;i++) 
			{
				cJSON_AddItemToArray(cjsonplayercards,cJSON_CreateString(bits256_str(str,playercards[i])));
			}
			
			char *rendered=cJSON_Print(playerInfo);
			int bytes=nn_send(bet->pushsock,rendered,strlen(rendered),0);
			printf("\n%s:%d:bytes:%d,buf:%s",__FUNCTION__,__LINE__,bytes,rendered);
			while (1) 
			{
				char *buf = NULL;
				int bytes = nn_recv (bet->subsock, &buf, NN_MSG, 0);
				if(bytes>0)
				{
					printf("\n%s:%d:%s",__FUNCTION__,__LINE__,buf);
					gameInfo=cJSON_Parse(buf);
					if(0==strcmp(cJSON_str(cJSON_GetObjectItem(gameInfo,"messageid")),"decode"))
					{
						public_key_b=jbits256(gameInfo,"public_key_b");
						g_shares=(struct enc_share*)malloc(CARDS777_MAXPLAYERS*CARDS777_MAXPLAYERS*CARDS777_MAXCARDS*sizeof(struct enc_share));
						cjsonblindedcards=cJSON_GetObjectItem(gameInfo,"blindedcards");
						for(int i=0;i<numplayers;i++)
						{
							for(int j=0;j<numcards;j++)
							{
								blindedcards[i][j]=jbits256i(cjsonblindedcards,i*numcards+j);
							}
						}
						cjsonshamirshards=cJSON_GetObjectItem(gameInfo,"shamirshards");
						cJSON_Print(cjsonshamirshards);	
						
						int k=0;
						for(int playerid=0;playerid<numplayers;playerid++)
						{
							for (int i=0; i<numcards; i++)
					        {
					            for (int j=0; j<numplayers; j++) 
								{
									g_shares[k]=get_API_enc_share(cJSON_GetArrayItem(cjsonshamirshards,k));
									k++;
					            }
					        }
						}
						for(int i=0;i<numcards;i++)
						{
							for(int j=0;j<numcards;j++)
							{
								temp=xoverz_donna(curve25519(key.priv,curve25519(playerprivs[i],cardprods[bet->myplayerid][j])));
								vcalc_sha256(0,v_hash[i][j].bytes,temp.bytes,sizeof(temp));
							}
						}
					   for(int i=0;i<numcards;i++)
					   {
        				    decoded256 = t_sg777_player_decode(bet,i,numplayers,key,public_key_b,blindedcards[bet->myplayerid][i],cardprods[bet->myplayerid],playerprivs,numcards);
            	            if ( bits256_nonz(decoded256) == 0 )
                				errs++;
            				else
            				{
            					int k;
				                unpermi=-1;
				                for(k=0;k<numcards;k++)
								{
				                    if(permis[k]==decoded256.bytes[30])
									{
				                        unpermi=k;
				                        break;
				                    }
				                }
				                decoded[i] = k;    	
            				}
        			 }
    				decodebad += errs;
    				decodegood+= (numcards - errs);
     				}
					else if(0==strcmp(cJSON_str(cJSON_GetObjectItem(gameInfo,"messageid")),"init_d"))
					{
						cjsoncardprods=cJSON_GetObjectItem(gameInfo,"cardprods");
						for(int i=0;i<numplayers;i++)
						{
							for(int j=0;j<numcards;j++)
							{
								cardprods[i][j]=jbits256i(cjsoncardprods,i*numcards+j);
							}
						}
					}
					else if(0==strcmp(cJSON_str(cJSON_GetObjectItem(gameInfo,"messageid")),"request_share"))
					{
						BET_give_share(gameInfo,bet,public_key_b,key);
					}
				}
			}
			nn_shutdown(bet->pushsock,0);
			nn_shutdown(bet->subsock,0);
		}
		return NULL;
}



void* BET_clientbvv(void * _ptr)
{
	  
	  	bits256 deckid,publickeys[CARDS777_MAXPLAYERS],playercards[CARDS777_MAXPLAYERS][CARDS777_MAXCARDS],cardprods[CARDS777_MAXPLAYERS][CARDS777_MAXCARDS],finalcards[CARDS777_MAXPLAYERS][CARDS777_MAXCARDS],blindedcards[CARDS777_MAXPLAYERS][CARDS777_MAXCARDS],blindingvals[CARDS777_MAXPLAYERS][CARDS777_MAXCARDS];
		int32_t numcards,numplayers,playerID,range,flag=1;
		struct pair256 b_key,keys[CARDS777_MAXPLAYERS];struct privatebet_info *bet = _ptr;
		char str[65],share_str[177];

		cJSON *playerInfo,*gameInfo,*cjsonplayercards,*temp,*item,*cjsonblindedcards,*cjsonfinalcards,*cjsonshamirshards;
		numplayers=0;
		numcards=bet->range;
		blinding_vendor_perm(bet->range);
		b_key.priv=curve25519_keypair(&b_key.prod);
		g_shares=(struct enc_share*)malloc(CARDS777_MAXPLAYERS*CARDS777_MAXPLAYERS*CARDS777_MAXCARDS*sizeof(struct enc_share));
   		if ( bet->subsock >= 0 && bet->pushsock >= 0 ) 
		{
			while(numplayers!=bet->numplayers) {
			  	char *buf=NULL;
				int bytes=nn_recv(bet->subsock,&buf,NN_MSG,0);
				if(bytes>0)	{
					gameInfo=cJSON_Parse(buf);
					if(0==strcmp(cJSON_str(cJSON_GetObjectItem(gameInfo,"messageid")),"init"))
					{
						numplayers++;
						playerID=jint(gameInfo,"playerid");
						range=jint(gameInfo,"range");
						keys[playerID].prod=jbits256(gameInfo,"publickey");
						playerInfo=cJSON_GetObjectItem(gameInfo,"playercards");
						for(int i=0;i<cJSON_GetArraySize(playerInfo);i++) 
						{
								playercards[playerID][i]=jbits256i(playerInfo,i);
						}
					}
				 }
				}
			printf("\n%s:%d",__FUNCTION__,__LINE__);
			while (flag) 
			{
				char *buf = NULL;
				int bytes = nn_recv (bet->subsock, &buf, NN_MSG, 0);
				if(bytes>0)
				{
					printf("\n%s:%d:%s",__FUNCTION__,__LINE__,buf);
					gameInfo=cJSON_Parse(buf);
					if(0==strcmp(cJSON_str(cJSON_GetObjectItem(gameInfo,"messageid")),"init_d")) 
					{
						deckid=jbits256(gameInfo,"deckid");
						cjsonfinalcards=cJSON_GetObjectItem(gameInfo,"finalcards");
						for(int playerID=0;playerID<numplayers;playerID++) 
						{
								for(int i=0;i<numcards;i++) 
								{
									finalcards[playerID][i]=jbits256i(cjsonfinalcards,playerID*numcards+i);
								}
						}
	         		    g_shares=(struct enc_share*)malloc(CARDS777_MAXPLAYERS*CARDS777_MAXPLAYERS*CARDS777_MAXCARDS*sizeof(struct enc_share));
					    for (int playerid=0; playerid<numplayers; playerid++)
						{
					        sg777_blinding_vendor(keys,b_key,blindingvals[playerid],blindedcards[playerid],finalcards[playerid],numcards,numplayers,playerid,deckid); // over network
    					}
						cJSON_Delete(gameInfo);
						gameInfo=cJSON_CreateObject();
						cJSON_AddStringToObject(gameInfo,"messageid","decode");
						jaddbits256(gameInfo,"public_key_b",b_key.prod);
						cJSON_AddItemToObject(gameInfo,"blindedcards",cjsonblindedcards=cJSON_CreateArray());
						for(int i=0;i<numplayers;i++) 
						{
							for(int j=0;j<numcards;j++) 
							{
							cJSON_AddItemToArray(cjsonblindedcards,cJSON_CreateString(bits256_str(str,blindedcards[i][j])));
							}
						}
						cJSON_AddItemToObject(gameInfo,"shamirshards",cjsonshamirshards=cJSON_CreateArray());
						int k=0;
						for(int playerid=0;playerid<numplayers;playerid++) 
						{
							for (int i=0; i<numcards; i++)
							{
					            for (int j=0; j<numplayers; j++) 
								{
									cJSON_AddItemToArray(cjsonshamirshards,cJSON_CreateString(enc_share_str(share_str,g_shares[k++])));
					            }
					        }
						}
						char *rendered=cJSON_Print(gameInfo);
						nn_send(bet->pushsock,rendered,strlen(rendered),0);
						flag=0;
					}
				}
			}
			nn_shutdown(bet->pushsock,0);
			nn_shutdown(bet->subsock,0);
		
		}
	  return NULL;
}


