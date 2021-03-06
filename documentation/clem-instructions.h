#define CLEM_AD		0x00
#define CLEM_ADI		0x00
#define CLEM_SB		0x01
#define CLEM_SBI		0x01
#define CLEM_MU		0x02
#define CLEM_MUI		0x02
#define CLEM_DV		0x03
#define CLEM_DVI		0x03
#define CLEM_MD		0x04
#define CLEM_MDI		0x04
#define CLEM_AN		0x05
#define CLEM_ANI		0x05
#define CLEM_OR		0x06
#define CLEM_ORI		0x06
#define CLEM_XR		0x07
#define CLEM_XRI		0x07
#define CLEM_ADC		0x08
#define CLEM_ADCI		0x08
#define CLEM_SBC		0x09
#define CLEM_SBCI		0x09
#define CLEM_SL		0x0a
#define CLEM_SR		0x0a
#define CLEM_SA		0x0b
#define CLEM_RL		0x0c
#define CLEM_RR		0x0c
#define CLEM_DMT		0x0d
#define CLEM_SLI		0x0e
#define CLEM_SRI		0x0e
#define CLEM_SAI		0x0f
#define CLEM_RLI		0x10
#define CLEM_RRI		0x10
#define CLEM_MH		0x11
#define CLEM_ML		0x12
#define CLEM_MS		0x13
#define CLEM_RE		0x14
#define CLEM_RE_SUB	0x00
#define CLEM_RE_SUB2	0x00
#define CLEM_IR		0x14
#define CLEM_IR_SUB	0x00
#define CLEM_IR_SUB2	0x01
#define CLEM_WT		0x14
#define CLEM_WT_SUB	0x00
#define CLEM_WT_SUB2	0x02
#define CLEM_HT		0x14
#define CLEM_HT_SUB	0x00
#define CLEM_HT_SUB2	0x03
#define CLEM_EI		0x14
#define CLEM_EI_SUB	0x00
#define CLEM_EI_SUB2	0x04
#define CLEM_DI		0x14
#define CLEM_DI_SUB	0x00
#define CLEM_DI_SUB2	0x05
#define CLEM_SES		0x14
#define CLEM_SES_SUB	0x00
#define CLEM_SES_SUB2	0x07
#define CLEM_SEW		0x14
#define CLEM_SEW_SUB	0x00
#define CLEM_SEW_SUB2	0x08
#define CLEM_ZES		0x14
#define CLEM_ZES_SUB	0x00
#define CLEM_ZES_SUB2	0x09
#define CLEM_ZEW		0x14
#define CLEM_ZEW_SUB	0x00
#define CLEM_ZEW_SUB2	0x0a
#define CLEM_SF		0x14
#define CLEM_SF_SUB	0x00
#define CLEM_SF_SUB2	0x0b
#define CLEM_RF		0x14
#define CLEM_RF_SUB	0x00
#define CLEM_RF_SUB2	0x0c
#define CLEM_FTI		0x14
#define CLEM_FTI_SUB	0x01
#define CLEM_FTI_SUB2	0x01
#define CLEM_ITF		0x14
#define CLEM_ITF_SUB	0x01
#define CLEM_ITF_SUB2	0x00
#define CLEM_SMP		0x14
#define CLEM_SMP_SUB	0x02
#define CLEM_SMP_SUB2	0x01
#define CLEM_RMP		0x14
#define CLEM_RMP_SUB	0x02
#define CLEM_RMP_SUB2	0x00
#define CLEM_NG		0x14
#define CLEM_NG_SUB	0x03
#define CLEM_NG_SUB2	0x00
#define CLEM_NT		0x14
#define CLEM_NT_SUB	0x03
#define CLEM_NT_SUB2	0x01
#define CLEM_BF		0x14
#define CLEM_BF_SUB	0x03
#define CLEM_BF_SUB2	0x02
#define CLEM_RND		0x14
#define CLEM_RND_SUB	0x03
#define CLEM_RND_SUB2	0x03
#define CLEM_LDS		0x15
#define CLEM_LDS_SUB		0x00
#define CLEM_LDW		0x15
#define CLEM_LDW_SUB		0x01
#define CLEM_LDT		0x15
#define CLEM_LDT_SUB		0x02
#define CLEM_STS		0x16
#define CLEM_STS_SUB		0x00
#define CLEM_STW		0x16
#define CLEM_STW_SUB		0x01
#define CLEM_STT		0x16
#define CLEM_STT_SUB		0x02
#define CLEM_CM		0x17
#define CLEM_CMI		0x17
#define BRANCH_COND_NOT_EQUAL	0
#define BRANCH_COND_EQUAL	1
#define BRANCH_COND_LESS_THAN	2
#define BRANCH_COND_LESS_THAN_OR_EQUAL	3
#define BRANCH_COND_GREATER_THAN	4
#define BRANCH_COND_GREATER_THAN_OR_EQUAL	5
#define BRANCH_COND_NOT_OVERFLOW	6
#define BRANCH_COND_OVERFLOW	7
#define BRANCH_COND_NOT_SIGNED	8
#define BRANCH_COND_SIGNED	9
#define BRANCH_COND_SIGNED_LESS_THAN	10
#define BRANCH_COND_SIGNED_LESS_THAN_OR_EQUAL	11
#define BRANCH_COND_SIGNED_GREATER_THAN	12
#define BRANCH_COND_SIGNED_GREATER_THAN_OR_EQUAL	13
#define BRANCH_COND_ALWAYS	15
#define CLEM_B		0x18
#define CLEM_BN_COND		0x00
#define CLEM_BE_COND		0x01
#define CLEM_BL_COND		0x02
#define CLEM_BLE_COND		0x03
#define CLEM_BG_COND		0x04
#define CLEM_BGE_COND		0x05
#define CLEM_BNO_COND		0x06
#define CLEM_BO_COND		0x07
#define CLEM_BNS_COND		0x08
#define CLEM_BS_COND		0x09
#define CLEM_BSL_COND		0x0a
#define CLEM_BSLE_COND		0x0b
#define CLEM_BSG_COND		0x0c
#define CLEM_BSGE_COND		0x0d
#define CLEM_B_COND		0x0f
#define BRANCH_COND_NOT_EQUAL	0
#define BRANCH_COND_EQUAL	1
#define BRANCH_COND_LESS_THAN	2
#define BRANCH_COND_LESS_THAN_OR_EQUAL	3
#define BRANCH_COND_GREATER_THAN	4
#define BRANCH_COND_GREATER_THAN_OR_EQUAL	5
#define BRANCH_COND_NOT_OVERFLOW	6
#define BRANCH_COND_OVERFLOW	7
#define BRANCH_COND_NOT_SIGNED	8
#define BRANCH_COND_SIGNED	9
#define BRANCH_COND_SIGNED_LESS_THAN	10
#define BRANCH_COND_SIGNED_LESS_THAN_OR_EQUAL	11
#define BRANCH_COND_SIGNED_GREATER_THAN	12
#define BRANCH_COND_SIGNED_GREATER_THAN_OR_EQUAL	13
#define BRANCH_COND_ALWAYS	15
#define CLEM_BR		0x19
#define CLEM_BRN_COND		0x00
#define CLEM_BRE_COND		0x01
#define CLEM_BRL_COND		0x02
#define CLEM_BRLE_COND		0x03
#define CLEM_BRG_COND		0x04
#define CLEM_BRGE_COND		0x05
#define CLEM_BRNO_COND		0x06
#define CLEM_BRO_COND		0x07
#define CLEM_BRNS_COND		0x08
#define CLEM_BRS_COND		0x09
#define CLEM_BRSL_COND		0x0a
#define CLEM_BRSLE_COND		0x0b
#define CLEM_BRSG_COND		0x0c
#define CLEM_BRSGE_COND		0x0d
#define CLEM_BR_COND		0x0f
#define BRANCH_COND_NOT_EQUAL	0
#define BRANCH_COND_EQUAL	1
#define BRANCH_COND_LESS_THAN	2
#define BRANCH_COND_LESS_THAN_OR_EQUAL	3
#define BRANCH_COND_GREATER_THAN	4
#define BRANCH_COND_GREATER_THAN_OR_EQUAL	5
#define BRANCH_COND_NOT_OVERFLOW	6
#define BRANCH_COND_OVERFLOW	7
#define BRANCH_COND_NOT_SIGNED	8
#define BRANCH_COND_SIGNED	9
#define BRANCH_COND_SIGNED_LESS_THAN	10
#define BRANCH_COND_SIGNED_LESS_THAN_OR_EQUAL	11
#define BRANCH_COND_SIGNED_GREATER_THAN	12
#define BRANCH_COND_SIGNED_GREATER_THAN_OR_EQUAL	13
#define BRANCH_COND_ALWAYS	15
#define CLEM_C		0x1a
#define CLEM_CN_COND		0x00
#define CLEM_CE_COND		0x01
#define CLEM_CL_COND		0x02
#define CLEM_CLE_COND		0x03
#define CLEM_CG_COND		0x04
#define CLEM_CGE_COND		0x05
#define CLEM_CNO_COND		0x06
#define CLEM_CO_COND		0x07
#define CLEM_CNS_COND		0x08
#define CLEM_CS_COND		0x09
#define CLEM_CSL_COND		0x0a
#define CLEM_CSLE_COND		0x0b
#define CLEM_CSG_COND		0x0c
#define CLEM_CSGE_COND		0x0d
#define CLEM_C_COND		0x0f
#define BRANCH_COND_NOT_EQUAL	0
#define BRANCH_COND_EQUAL	1
#define BRANCH_COND_LESS_THAN	2
#define BRANCH_COND_LESS_THAN_OR_EQUAL	3
#define BRANCH_COND_GREATER_THAN	4
#define BRANCH_COND_GREATER_THAN_OR_EQUAL	5
#define BRANCH_COND_NOT_OVERFLOW	6
#define BRANCH_COND_OVERFLOW	7
#define BRANCH_COND_NOT_SIGNED	8
#define BRANCH_COND_SIGNED	9
#define BRANCH_COND_SIGNED_LESS_THAN	10
#define BRANCH_COND_SIGNED_LESS_THAN_OR_EQUAL	11
#define BRANCH_COND_SIGNED_GREATER_THAN	12
#define BRANCH_COND_SIGNED_GREATER_THAN_OR_EQUAL	13
#define BRANCH_COND_ALWAYS	15
#define CLEM_CR		0x1b
#define CLEM_CRN_COND		0x00
#define CLEM_CRE_COND		0x01
#define CLEM_CRL_COND		0x02
#define CLEM_CRLE_COND		0x03
#define CLEM_CRG_COND		0x04
#define CLEM_CRGE_COND		0x05
#define CLEM_CRNO_COND		0x06
#define CLEM_CRO_COND		0x07
#define CLEM_CRNS_COND		0x08
#define CLEM_CRS_COND		0x09
#define CLEM_CRSL_COND		0x0a
#define CLEM_CRSLE_COND		0x0b
#define CLEM_CRSG_COND		0x0c
#define CLEM_CRSGE_COND		0x0d
#define CLEM_CR_COND		0x0f
#define CLEM_BRR		0x1c
#define CLEM_BRR_SUB		0x00
#define CLEM_BRA		0x1c
#define CLEM_BRA_SUB		0x01
#define CLEM_CAR		0x1c
#define CLEM_CAR_SUB		0x02
#define CLEM_CAA		0x1c
#define CLEM_CAA_SUB		0x03
#define CLEM_DBRK		0x1F
