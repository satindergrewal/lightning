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


////////////////////////// Game statemachine
cJSON *BET_statemachine_gamestart_actions(struct privatebet_info *bet,struct privatebet_vars *vars)
{
    printf("BET_statemachine_gamestart\n");
    return(cJSON_CreateArray());
}

cJSON *BET_statemachine_turni_actions(struct privatebet_info *bet,struct privatebet_vars *vars)
{
    uint32_t r; cJSON *array = cJSON_CreateArray();
    OS_randombytes((void *)&r,sizeof(r));
    if ( bet->range < 2 )
        r = 0;
    else r %= bet->range;
    jaddinum(array,r);
    printf("BET_statemachine_turni -> r%d turni.%d r.%d / range.%d\n",vars->round,vars->turni,r,bet->range);
    return(array);
}

void BET_statemachine_endround(struct privatebet_info *bet,struct privatebet_vars *vars)
{
    printf("BET_statemachine_endround -> %d\n",vars->round);
}

void BET_statemachine_deali(struct privatebet_info *bet,struct privatebet_vars *vars,int32_t deali,int32_t playerj)
{
    cJSON *reqjson;
    //printf("BET_statemachine_deali cardi.%d -> r%d, t%d, d%d playerj.%d\n",vars->permi[deali],vars->roundready,vars->turni,deali,playerj);
    reqjson = cJSON_CreateObject();
    jaddstr(reqjson,"method","deali");
    jaddnum(reqjson,"playerj",playerj);
    jaddnum(reqjson,"deali",deali);
    jaddnum(reqjson,"cardi",vars->permi[deali]);
    BET_message_send("BET_deali",bet->pubsock>=0?bet->pubsock:bet->pushsock,reqjson,1,bet);
}

void BET_statemachine_roundstart(struct privatebet_info *bet,struct privatebet_vars *vars)
{
    if ( vars->roundready < bet->numrounds )
    {
        printf("BET_statemachine_roundstart -> %d\n",vars->roundready);
        /*if ( bet->myplayerid == 0 )
            BET_statemachine_deali(bet,vars,vars->roundready % bet->range,(rand() % (bet->numplayers+1)) - 1);*/
    }
}

////////////////////////// end Game statemachine


