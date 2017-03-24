
typedef enum known_sequences {
    SEQ_COFFEE1           ,
    SEQ_COFFEE2           ,
    SEQ_COFFEE3           ,
    SEQ_COFFEE4           ,
    SEQ_COFFEE5           ,
    SEQ_COFFEE6           ,

    SEQ_COFFEE1_1SIP      ,
    SEQ_COFFEE2_1SIP      ,
    SEQ_COFFEE3_1SIP      ,
    SEQ_COFFEE4_1SIP      ,
    SEQ_COFFEE5_1SIP      ,
    SEQ_COFFEE6_1SIP      ,

    SEQ_ERROR_COFFEE1     ,
    SEQ_ERROR_COFFEE2     ,
    SEQ_ERROR_COFFEE3     ,
    SEQ_ERROR_COFFEE4     ,
    SEQ_ERROR_COFFEE5     ,
    SEQ_ERROR_COFFEE6     ,

    SEQ_ERROR_COFFEE_S1   ,
    SEQ_ERROR_COFFEE_S2   ,
    SEQ_ERROR_COFFEE_C    ,
    SEQ_ERROR_COFFEE_D    ,
    SEQ_ERROR_COFFEE      ,

    SEQ_TEA1              ,
    SEQ_TEA2              ,
    SEQ_TEA3              ,

    SEQ_TEA1_CREAM        ,
    SEQ_TEA2_CREAM        ,

    SEQ_TEA1_1SIP         ,
    SEQ_TEA2_1SIP         ,
    SEQ_TEA3_1SIP         ,
    SEQ_ERROR_TEA1        ,
    SEQ_ERROR_TEA2        ,
    SEQ_ERROR_TEA         ,

    SEQ_PICKUP_SIP        ,
    SEQ_SUGAR_PACK        ,
    SEQ_SUGAR_BOWL        ,
    SEQ_ERROR             ,

    NUM_VARIANTS} KnownSequences;

extern char *task_name[3];
extern char *variant_name[NUM_VARIANTS];

extern ActionType sequence_coffee1[37];
extern ActionType sequence_coffee2[37];
extern ActionType sequence_coffee3[37];
extern ActionType sequence_coffee4[37];
extern ActionType sequence_coffee5[31];
extern ActionType sequence_coffee6[31];
extern ActionType sequence_coffee1_1sip[36];
extern ActionType sequence_coffee2_1sip[36];
extern ActionType sequence_coffee3_1sip[36];
extern ActionType sequence_coffee4_1sip[36];
extern ActionType sequence_coffee5_1sip[30];
extern ActionType sequence_coffee6_1sip[30];
extern ActionType sequence_coffee1_error[33];
extern ActionType sequence_coffee2_error[33];
extern ActionType sequence_coffee3_error[33];
extern ActionType sequence_coffee4_error[33];
extern ActionType sequence_coffee5_error[27];
extern ActionType sequence_coffee6_error[27];
extern ActionType sequence_coffee_s1_drink[26];
extern ActionType sequence_coffee_s2_drink[26];
extern ActionType sequence_coffee_c_drink[26];
extern ActionType sequence_coffee_drink[15];
extern ActionType sequence_coffee_start[11];
extern ActionType sequence_tea1[20];
extern ActionType sequence_tea2[20];
extern ActionType sequence_tea3[18];
extern ActionType sequence_tea1_1sip[19];
extern ActionType sequence_tea2_1sip[19];
extern ActionType sequence_tea3_1sip[17];
extern ActionType sequence_tea_start[5];
extern ActionType sequence_tea1_error[16];
extern ActionType sequence_tea2_error[16];
extern ActionType sequence_tea1_with_cream[31];
extern ActionType sequence_tea2_with_cream[31];
extern ActionType sequence_pickup_sip[3];
extern ActionType sequence_sugar_bowl[12];
extern ActionType sequence_sugar_pack[12];



