/*
 * ModSecurity for Apache 2.x, http://www.modsecurity.org/
 * Copyright (c) 2004-2010 Trustwave Holdings, Inc. (http://www.trustwave.com/)
 *
 * This product is released under the terms of the General Public Licence,
 * version 2 (GPLv2). Please refer to the file LICENSE (included with this
 * distribution) which contains the complete text of the licence.
 *
 * There are special exceptions to the terms and conditions of the GPL
 * as it is applied to this software. View the full text of the exception in
 * file MODSECURITY_LICENSING_EXCEPTION in the directory of this software
 * distribution.
 *
 * If any of the files related to licensing are missing or if you have any
 * other questions related to licensing please contact Trustwave Holdings, Inc.
 * directly using the email address support@trustwave.com.
 *
 */

#include "msc_gsb.h"

static int gsb_mal_create(directory_config *dcfg, char **error_msg)
{
    char errstr[1024];
    apr_pool_t *mp = dcfg->mp;
    gsb_db *gsb = dcfg->gsb;
    apr_int32_t wanted = APR_FINFO_SIZE;
    apr_finfo_t finfo;
    apr_status_t rc;
    apr_size_t nbytes;
    char *buf = NULL, *p = NULL, *savedptr = NULL;
    char *op = NULL;

    if ((rc = apr_file_open(&gsb->db, gsb->dbfn, APR_READ, APR_OS_DEFAULT, mp)) != APR_SUCCESS) {
        *error_msg = apr_psprintf(mp, "Could not open gsb database \"%s\": %s", gsb->dbfn, apr_strerror(rc, errstr, 1024));
        return 0;
    }

    if ((rc = apr_file_info_get(&finfo, wanted, gsb->db)) != APR_SUCCESS)  {
        *error_msg = apr_psprintf(mp, "Could not cannot get gsb malware file information \"%s\": %s", gsb->dbfn, apr_strerror(rc, errstr, 1024));
        return 0;
    }

    buf = apr_palloc(dcfg->mp, finfo.size+1);

    if (buf == NULL)   {
        *error_msg = apr_psprintf(mp, "Could not alloc memory for gsb data");
        return 0;
    }

    rc = apr_file_read_full(gsb->db, buf, finfo.size, &nbytes);

    gsb->gsb_table = apr_table_make(dcfg->mp, 16);

    if (gsb->gsb_table == NULL)   {
        *error_msg = apr_psprintf(mp, "Could not alloc memory for gsb table");
        return 0;
    }

    p = apr_strtok(buf,"\t",&savedptr);

    while (p != NULL)   {

        op = strchr(p,'+');

        if(op != NULL)   {
            char *hash = ++op;
            if(strlen(hash) == 32)
            apr_table_setn(gsb->gsb_table,hash,"malware");
        }

        op = strchr(p,'-');

        if(op != NULL)   {
            char *hash = ++op;
            if(strlen(hash) == 32)
            apr_table_unset(gsb->gsb_table,hash);
        }

        p = apr_strtok(NULL,"\t",&savedptr);
    }

    apr_file_close(gsb->db);

    return 1;
}


/**
 * Initialise Gsb malware data structure
 */
int gsb_mal_init(directory_config *dcfg, const char *dbfn, char **error_msg)
{

    *error_msg = NULL;

    if ((dcfg->gsb == NULL) || (dcfg->gsb == NOT_SET_P)) {
        dcfg->gsb = apr_pcalloc(dcfg->mp, sizeof(gsb_db));
        if (dcfg->gsb == NULL)  {
            ap_log_error(APLOG_MARK, APLOG_WARNING, 0, NULL, "GSB 0 : %p",dcfg->gsb);
        }
    }

    dcfg->gsb->db = NULL;
    dcfg->gsb->dbfn = apr_pstrdup(dcfg->mp, dbfn);

    return gsb_mal_create(dcfg, error_msg);
}

