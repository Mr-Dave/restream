/*
 *    This file is part of Restream.
 *
 *    Restream is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    Restream is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Restream.  If not, see <https://www.gnu.org/licenses/>.
 *
*/

#include "restream.hpp"
#include "conf.hpp"
#include "util.hpp"
#include "logger.hpp"
#include "channel.hpp"
#include "infile.hpp"
#include "pktarray.hpp"
#include "webu.hpp"
#include "webu_ans.hpp"
#include "webu_mpegts.hpp"
#include "webu_meta.hpp"


/* Initialize the MHD answer */
static void *mhd_init(void *cls, const char *uri, struct MHD_Connection *connection)
{
    (void)connection;
    cls_app     *p_app =(cls_app *)cls;
    cls_webua   *webua;

    mythreadname_set("wc", 0, NULL);

    webua = new cls_webua(p_app, uri);

    return webua;
}

/* Clean up our variables when the MHD connection closes */
static void mhd_deinit(void *cls, struct MHD_Connection *connection
        , void **con_cls, enum MHD_RequestTerminationCode toe)
{
    (void)connection;
    (void)cls;
    (void)toe;
    cls_webua *webua =(cls_webua *) *con_cls;

    if (webua != nullptr) {
        if (webua->cnct_type == WEBUA_CNCT_TS_FULL) {
            if (webua->chitm->cnct_cnt > 0) {
                webua->chitm->cnct_cnt--;
            }
            LOG_MSG(INF, NO_ERRNO ,"Ch%s: Closing connection"
                , webua->chitm->ch_nbr.c_str());
        }
        delete webua;
    }
}

/* Answer the connection request for the webcontrol*/
static mhdrslt mhd_answer(void *cls
        , struct MHD_Connection *connection
        , const char *url, const char *method, const char *version
        , const char *upload_data, size_t *upload_data_size
        , void **ptr)
{
    (void)cls;
    (void)url;
    (void)version;
    (void)upload_data;
    (void)upload_data_size;
    (void)method;

    cls_webua *webua =(cls_webua *) *ptr;

    return webua->answer(connection);

}


/* Validate that the MHD version installed can process basic authentication */
void cls_webu::mhd_features_basic()
{
    #if MHD_VERSION < 0x00094400
        LOG_MSG(NTC, NO_ERRNO ,"Basic authentication: disabled");
        c_app->conf->webcontrol_auth_method = "none";
    #else
        mhdrslt retcd;
        retcd = MHD_is_feature_supported (MHD_FEATURE_BASIC_AUTH);
        if (retcd == MHD_YES) {
            LOG_MSG(DBG, NO_ERRNO ,"Basic authentication: available");
        } else {
            if (c_app->conf->webcontrol_auth_method == "basic") {
                LOG_MSG(NTC, NO_ERRNO ,"Basic authentication: disabled");
                c_app->conf->webcontrol_auth_method = "none";
            } else {
                LOG_MSG(INF, NO_ERRNO ,"Basic authentication: disabled");
            }
        }
    #endif
}

/* Validate that the MHD version installed can process digest authentication */
void cls_webu::mhd_features_digest()
{
    #if MHD_VERSION < 0x00094400
        LOG_MSG(NTC, NO_ERRNO ,"Digest authentication: disabled");
        c_app->conf->webcontrol_auth_method = "none";
    #else
        mhdrslt retcd;
        retcd = MHD_is_feature_supported (MHD_FEATURE_DIGEST_AUTH);
        if (retcd == MHD_YES) {
            LOG_MSG(DBG, NO_ERRNO ,"Digest authentication: available");
        } else {
            if (c_app->conf->webcontrol_auth_method == "digest") {
                LOG_MSG(NTC, NO_ERRNO ,"Digest authentication: disabled");
                c_app->conf->webcontrol_auth_method = "none";
            } else {
                LOG_MSG(INF, NO_ERRNO ,"Digest authentication: disabled");
            }
        }
    #endif
}

/* Validate that the MHD version installed can process IPV6 */
void cls_webu::mhd_features_ipv6(ctx_mhdstart *mhdst)
{
    #if MHD_VERSION < 0x00094400
        if (mhdst->ipv6) {
            LOG_MSG(INF, NO_ERRNO ,"libmicrohttpd libary too old ipv6 disabled");
            if (mhdst->ipv6) {
                mhdst->ipv6 = 0;
            }
        }
    #else
        mhdrslt retcd;
        retcd = MHD_is_feature_supported (MHD_FEATURE_IPv6);
        if (retcd == MHD_YES) {
            LOG_MSG(DBG, NO_ERRNO ,"IPV6: available");
        } else {
            LOG_MSG(NTC, NO_ERRNO ,"IPV6: disabled");
            if (mhdst->ipv6) {
                mhdst->ipv6 = 0;
            }
        }
    #endif
}

/* Validate that the MHD version installed can process tls */
void cls_webu::mhd_features_tls(ctx_mhdstart *mhdst)
{
    #if MHD_VERSION < 0x00094400
        if (mhdst->tls_use) {
            LOG_MSG(INF, NO_ERRNO ,"libmicrohttpd libary too old SSL/TLS disabled");
            mhdst->tls_use = false;
        }
    #else
        mhdrslt retcd;
        retcd = MHD_is_feature_supported (MHD_FEATURE_SSL);
        if (retcd == MHD_YES) {
            LOG_MSG(DBG, NO_ERRNO ,"SSL/TLS: available");
        } else {
            if (mhdst->tls_use) {
                LOG_MSG(NTC, NO_ERRNO ,"SSL/TLS: disabled");
                mhdst->tls_use = false;
            } else {
                LOG_MSG(INF, NO_ERRNO ,"SSL/TLS: disabled");
            }
        }
    #endif
}

/* Validate the features that MHD can support */
void cls_webu::mhd_features(ctx_mhdstart *mhdst)
{
    mhd_features_basic();

    mhd_features_digest();

    mhd_features_ipv6(mhdst);

    mhd_features_tls(mhdst);

}

/* Load a either the key or cert file for MHD*/
void cls_webu::mhd_loadfile(std::string fname, std::string &filestr)
{
    /* This needs conversion to c++ stream */
    FILE        *infile;
    size_t      file_size, read_size;
    char        *file_char;

    filestr = "";
    if (fname != "") {
        infile = myfopen(fname.c_str() , "rbe");
        if (infile != NULL) {
            fseek(infile, 0, SEEK_END);
            file_size = ftell(infile);
            if (file_size > 0 ) {
                file_char = (char*)mymalloc(file_size +1);
                fseek(infile, 0, SEEK_SET);
                read_size = fread(file_char, file_size, 1, infile);
                if (read_size > 0 ) {
                    file_char[file_size] = 0;
                    filestr.assign(file_char, file_size);
                } else {
                    LOG_MSG(ERR, NO_ERRNO
                        ,"Error reading file for SSL/TLS support.");
                }
                free(file_char);
            }
            myfclose(infile);
        }
    }

}

/* Validate that we have the files needed for tls*/
void cls_webu::mhd_checktls(ctx_mhdstart *mhdst)
{

    if (mhdst->tls_use) {
        if ((c_app->conf->webcontrol_cert == "") || (mhdst->tls_cert == "")) {
            LOG_MSG(NTC, NO_ERRNO
                ,"SSL/TLS requested but no cert file provided.  SSL/TLS disabled");
            mhdst->tls_use = false;
        }
        if ((c_app->conf->webcontrol_key == "") || (mhdst->tls_key == "")) {
            LOG_MSG(NTC, NO_ERRNO
                ,"SSL/TLS requested but no key file provided.  SSL/TLS disabled");
            mhdst->tls_use = false;
        }
    }

}

/* Set the initialization function for MHD to call upon getting a connection */
void cls_webu::mhd_opts_init(ctx_mhdstart *mhdst)
{
    mhdst->mhd_ops[mhdst->mhd_opt_nbr].option = MHD_OPTION_URI_LOG_CALLBACK;
    mhdst->mhd_ops[mhdst->mhd_opt_nbr].value = (intptr_t)mhd_init;
    mhdst->mhd_ops[mhdst->mhd_opt_nbr].ptr_value = c_app;
    mhdst->mhd_opt_nbr++;
}

/* Set the MHD option on the function to call when the connection closes */
void cls_webu::mhd_opts_deinit(ctx_mhdstart *mhdst)
{
    mhdst->mhd_ops[mhdst->mhd_opt_nbr].option = MHD_OPTION_NOTIFY_COMPLETED;
    mhdst->mhd_ops[mhdst->mhd_opt_nbr].value = (intptr_t)mhd_deinit;
    mhdst->mhd_ops[mhdst->mhd_opt_nbr].ptr_value = NULL;
    mhdst->mhd_opt_nbr++;

}

/* Set the MHD option on acceptable connections */
void cls_webu::mhd_opts_localhost(ctx_mhdstart *mhdst)
{
    if (c_app->conf->webcontrol_localhost) {
        if (mhdst->ipv6) {
            memset(&mhdst->lpbk_ipv6, 0, sizeof(struct sockaddr_in6));
            mhdst->lpbk_ipv6.sin6_family = AF_INET6;
            mhdst->lpbk_ipv6.sin6_port = htons((uint16_t)c_app->conf->webcontrol_port);
            mhdst->lpbk_ipv6.sin6_addr = in6addr_loopback;

            mhdst->mhd_ops[mhdst->mhd_opt_nbr].option = MHD_OPTION_SOCK_ADDR;
            mhdst->mhd_ops[mhdst->mhd_opt_nbr].value = 0;
            mhdst->mhd_ops[mhdst->mhd_opt_nbr].ptr_value = (struct sosockaddr *)(&mhdst->lpbk_ipv6);
            mhdst->mhd_opt_nbr++;

        } else {
            memset(&mhdst->lpbk_ipv4, 0, sizeof(struct sockaddr_in));
            mhdst->lpbk_ipv4.sin_family = AF_INET;
            mhdst->lpbk_ipv4.sin_port = htons((uint16_t)c_app->conf->webcontrol_port);
            mhdst->lpbk_ipv4.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

            mhdst->mhd_ops[mhdst->mhd_opt_nbr].option = MHD_OPTION_SOCK_ADDR;
            mhdst->mhd_ops[mhdst->mhd_opt_nbr].value = 0;
            mhdst->mhd_ops[mhdst->mhd_opt_nbr].ptr_value = (struct sockaddr *)(&mhdst->lpbk_ipv4);
            mhdst->mhd_opt_nbr++;
        }
    }

}

/* Set the mhd digest options */
void cls_webu::mhd_opts_digest(ctx_mhdstart *mhdst)
{
    if (c_app->conf->webcontrol_auth_method == "digest") {

        mhdst->mhd_ops[mhdst->mhd_opt_nbr].option = MHD_OPTION_DIGEST_AUTH_RANDOM;
        mhdst->mhd_ops[mhdst->mhd_opt_nbr].value = sizeof(digest_rand);
        mhdst->mhd_ops[mhdst->mhd_opt_nbr].ptr_value = digest_rand;
        mhdst->mhd_opt_nbr++;

        mhdst->mhd_ops[mhdst->mhd_opt_nbr].option = MHD_OPTION_NONCE_NC_SIZE;
        mhdst->mhd_ops[mhdst->mhd_opt_nbr].value = 300;
        mhdst->mhd_ops[mhdst->mhd_opt_nbr].ptr_value = NULL;
        mhdst->mhd_opt_nbr++;

        mhdst->mhd_ops[mhdst->mhd_opt_nbr].option = MHD_OPTION_CONNECTION_TIMEOUT;
        mhdst->mhd_ops[mhdst->mhd_opt_nbr].value = (unsigned int) 120;
        mhdst->mhd_ops[mhdst->mhd_opt_nbr].ptr_value = NULL;
        mhdst->mhd_opt_nbr++;
    }

}

/* Set the MHD options needed when we want TLS connections */
void cls_webu::mhd_opts_tls(ctx_mhdstart *mhdst)
{
    if (mhdst->tls_use) {

        mhdst->mhd_ops[mhdst->mhd_opt_nbr].option = MHD_OPTION_HTTPS_MEM_CERT;
        mhdst->mhd_ops[mhdst->mhd_opt_nbr].value = 0;
        mhdst->mhd_ops[mhdst->mhd_opt_nbr].ptr_value = (void *)mhdst->tls_cert.c_str();
        mhdst->mhd_opt_nbr++;

        mhdst->mhd_ops[mhdst->mhd_opt_nbr].option = MHD_OPTION_HTTPS_MEM_KEY;
        mhdst->mhd_ops[mhdst->mhd_opt_nbr].value = 0;
        mhdst->mhd_ops[mhdst->mhd_opt_nbr].ptr_value = (void *)mhdst->tls_key.c_str();
        mhdst->mhd_opt_nbr++;
    }

}

/* Set all the MHD options based upon the configuration parameters*/
void cls_webu::mhd_opts(ctx_mhdstart *mhdst)
{
    mhdst->mhd_opt_nbr = 0;

    mhd_checktls(mhdst);

    mhd_opts_deinit(mhdst);

    mhd_opts_init(mhdst);

    mhd_opts_localhost(mhdst);

    mhd_opts_digest(mhdst);

    mhd_opts_tls(mhdst);

    mhdst->mhd_ops[mhdst->mhd_opt_nbr].option = MHD_OPTION_END;
    mhdst->mhd_ops[mhdst->mhd_opt_nbr].value = 0;
    mhdst->mhd_ops[mhdst->mhd_opt_nbr].ptr_value = NULL;
    mhdst->mhd_opt_nbr++;

}

/* Set the mhd start up flags */
void cls_webu::mhd_flags(ctx_mhdstart *mhdst)
{
    mhdst->mhd_flags = MHD_USE_THREAD_PER_CONNECTION;

    if (mhdst->ipv6) {
        mhdst->mhd_flags = mhdst->mhd_flags | MHD_USE_DUAL_STACK;
    }

    if (mhdst->tls_use) {
        mhdst->mhd_flags = mhdst->mhd_flags | MHD_USE_SSL;
    }

}

cls_webu::cls_webu(cls_app *p_app)
{
    daemon = NULL;
    wb_finish = false;

    c_app = p_app;

    if (c_app->conf->webcontrol_port == 0) {
        return;
    }
    ctx_mhdstart mhdst;
    unsigned int randnbr;

    LOG_MSG(NTC, NO_ERRNO
        , "Starting webcontrol on port %d"
        , c_app->conf->webcontrol_port);

    headers.update_params = true;
    util_parms_parse(headers,"webu"
        , c_app->conf->webcontrol_headers);

    mhd_loadfile(c_app->conf->webcontrol_cert, mhdst.tls_cert);
    mhd_loadfile(c_app->conf->webcontrol_key, mhdst.tls_key);
    mhdst.ipv6 = c_app->conf->webcontrol_ipv6;
    mhdst.tls_use = c_app->conf->webcontrol_tls;

    /* Set the rand number for webcontrol digest if needed */
    srand((unsigned int)time(NULL));
    randnbr = (unsigned int)(42000000.0 * rand() / (RAND_MAX + 1.0));
    snprintf(digest_rand,sizeof(digest_rand),"%d",randnbr);

    mhdst.mhd_ops =(struct MHD_OptionItem*)mymalloc(
        sizeof(struct MHD_OptionItem)*WEBU_MHD_OPTS);
    mhd_features(&mhdst);
    mhd_opts(&mhdst);
    mhd_flags(&mhdst);

    daemon = MHD_start_daemon (
        mhdst.mhd_flags
        , (uint16_t)c_app->conf->webcontrol_port
        , NULL, NULL
        , &mhd_answer, c_app
        , MHD_OPTION_ARRAY, mhdst.mhd_ops
        , MHD_OPTION_END);

    free(mhdst.mhd_ops);
    if (daemon == NULL) {
        LOG_MSG(NTC, NO_ERRNO ,"Unable to start MHD");
    } else {
        LOG_MSG(NTC, NO_ERRNO
            ,"Started webcontrol on port %d"
            ,c_app->conf->webcontrol_port);
    }
}

cls_webu::~cls_webu()
{
    if (daemon != NULL) {
        wb_finish = true;
        MHD_stop_daemon(daemon);
    }
}

