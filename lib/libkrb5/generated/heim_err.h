/* Generated from /home/biorn/src/lib/libkrb5/../../kerberosV/src/lib/krb5/heim_err.et */
/* $KTH: heim_err.et,v 1.13 2004/02/13 16:23:40 lha Exp $ */

#ifndef __heim_err_h__
#define __heim_err_h__

struct et_list;

void initialize_heim_error_table_r(struct et_list **);

void initialize_heim_error_table(void);
#define init_heim_err_tbl initialize_heim_error_table

typedef enum heim_error_number{
	HEIM_ERR_LOG_PARSE = -1980176640,
	HEIM_ERR_V4_PRINC_NO_CONV = -1980176639,
	HEIM_ERR_SALTTYPE_NOSUPP = -1980176638,
	HEIM_ERR_NOHOST = -1980176637,
	HEIM_ERR_OPNOTSUPP = -1980176636,
	HEIM_ERR_EOF = -1980176635,
	HEIM_ERR_BAD_MKEY = -1980176634,
	HEIM_ERR_SERVICE_NOMATCH = -1980176633,
	HEIM_PKINIT_NO_CERTIFICATE = -1980176576,
	HEIM_PKINIT_NO_PRIVATE_KEY = -1980176575,
	HEIM_PKINIT_NO_VALID_CA = -1980176574,
	HEIM_PKINIT_CERTIFICATE_INVALID = -1980176573,
	HEIM_PKINIT_PRIVATE_KEY_INVALID = -1980176572,
	HEIM_EAI_UNKNOWN = -1980176512,
	HEIM_EAI_ADDRFAMILY = -1980176511,
	HEIM_EAI_AGAIN = -1980176510,
	HEIM_EAI_BADFLAGS = -1980176509,
	HEIM_EAI_FAIL = -1980176508,
	HEIM_EAI_FAMILY = -1980176507,
	HEIM_EAI_MEMORY = -1980176506,
	HEIM_EAI_NODATA = -1980176505,
	HEIM_EAI_NONAME = -1980176504,
	HEIM_EAI_SERVICE = -1980176503,
	HEIM_EAI_SOCKTYPE = -1980176502,
	HEIM_EAI_SYSTEM = -1980176501
} heim_error_number;

#define ERROR_TABLE_BASE_heim -1980176640

#endif /* __heim_err_h__ */
