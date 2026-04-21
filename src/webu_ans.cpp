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
#include "webu_post.hpp"

void cls_webua::html_badreq()
{
    resp_page =
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<body>\n"
        "<p>Bad Request</p>\n"
        "<p>The server did not understand your request.</p>\n"
        "</body>\n"
        "</html>\n";

}

/* Extract the camid and cmds from the url */
void cls_webua::parseurl()
{
    char *tmpurl;
    size_t  pos_slash1, pos_slash2, baselen;

    /* Example:  /chid/cmd1/cmd2/cmd3   */
    uri_chid = "";
    uri_cmd1 = "";
    uri_cmd2 = "";
    uri_cmd3 = "";

    LOG_MSG(DBG, NO_ERRNO, "Sent url: %s", url.c_str());

    tmpurl = (char*)mymalloc(url.length()+1);
    memcpy(tmpurl, url.c_str(), url.length());

    MHD_http_unescape(tmpurl);

    url.assign(tmpurl);
    free(tmpurl);

    LOG_MSG(DBG, NO_ERRNO, "Decoded url: %s",url.c_str());

    baselen = c_conf->webcontrol_base_path.length();

    if (url.length() < baselen) {
        url = "";
        return;
    }

    if (url.substr(baselen) ==  "/favicon.ico") {
        url = "";
        return;
    }

    if (url.substr(0, baselen) !=
        c_conf->webcontrol_base_path) {
        url = "";
        return;
    }

    if (url == "/") {
        url = "";
        return;
    }

    /* Remove any trailing slash to keep parms clean */
    if (url.substr(url.length()-1,1) == "/") {
        url = url.substr(0, url.length()-1);
    }

    if (url.length() == baselen) {
        return;
    }

    pos_slash1 = url.find("/", baselen+1);
    if (pos_slash1 != std::string::npos) {
        uri_chid = url.substr(baselen+1, pos_slash1-baselen- 1);
    } else {
        uri_chid = url.substr(baselen+1);
        return;
    }

    pos_slash1++;
    if (pos_slash1 >= url.length()) {
        return;
    }

    pos_slash2 = url.find("/", pos_slash1);
    if (pos_slash2 != std::string::npos) {
        uri_cmd1 = url.substr(pos_slash1, pos_slash2 - pos_slash1);
    } else {
        uri_cmd1 = url.substr(pos_slash1);
        return;
    }

    pos_slash1 = ++pos_slash2;
    if (pos_slash1 >= url.length()) {
        return;
    }

    pos_slash2 = url.find("/", pos_slash1);
    if (pos_slash2 != std::string::npos) {
        uri_cmd2 = url.substr(pos_slash1, pos_slash2 - pos_slash1);
    } else {
        uri_cmd2 = url.substr(pos_slash1);
        return;
    }

    pos_slash1 = ++pos_slash2;
    if (pos_slash1 >= url.length()) {
        return;
    }
    uri_cmd3 = url.substr(pos_slash1);

}

/* Edit the parameters specified in the url sent */
void cls_webua::parms_edit(const char *uri)
{
    int indx, is_nbr;

    url.assign(uri);

    parseurl();

    channel_id = -1;
    if (uri_chid.length() > 0) {
        is_nbr = true;
        for (indx=0; indx < (int)uri_chid.length(); indx++) {
            if ((uri_chid[indx] > '9') || (uri_chid[indx] < '0')) {
                is_nbr = false;
            }
        }
        if (is_nbr) {
            channel_id = atoi(uri_chid.c_str());
        }
    }

    for (indx=0; indx< app->ch_count; indx++) {
        if (atoi(app->channels[indx]->ch_nbr.c_str()) == channel_id) {
            channel_indx = indx;
            chitm = app->channels[indx];
        }
    }

    LOG_MSG(DBG, NO_ERRNO
        , "channel: >%s< index: >%d< cmd1: >%s< cmd2: >%s< cmd3: >%s<"
        , uri_chid.c_str(), channel_indx
        , uri_cmd1.c_str(), uri_cmd2.c_str()
        , uri_cmd3.c_str());

}

/* Log the ip of the client connecting*/
void cls_webua::get_clientip()
{
    const union MHD_ConnectionInfo *con_info;
    char client[WEBUA_LEN_URLI];
    const char *ip_dst;
    struct sockaddr_in6 *con_socket6;
    struct sockaddr_in *con_socket4;
    int is_ipv6;

    is_ipv6 = false;
    if (c_conf->webcontrol_ipv6) {
        is_ipv6 = true;
    }

    con_info = MHD_get_connection_info(connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);
    if (is_ipv6) {
        con_socket6 = (struct sockaddr_in6 *)con_info->client_addr;
        ip_dst = inet_ntop(AF_INET6, &con_socket6->sin6_addr, client, WEBUA_LEN_URLI);
        if (ip_dst == NULL) {
            clientip = "Unknown";
        } else {
            clientip.assign(client);
            if (clientip.substr(0, 7) == "::ffff:") {
                clientip = clientip.substr(7);
            }
        }
    } else {
        con_socket4 = (struct sockaddr_in *)con_info->client_addr;
        ip_dst = inet_ntop(AF_INET, &con_socket4->sin_addr, client, WEBUA_LEN_URLI);
        if (ip_dst == NULL) {
            clientip = "Unknown";
        } else {
            clientip.assign(client);
        }
    }

}

/* Get the hostname */
void cls_webua::get_hostname()
{
    const char *hdr;

    hdr = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_HOST);
    if (hdr == NULL) {
        hostfull = "//localhost:" +
            std::to_string(c_conf->webcontrol_port) +
            c_conf->webcontrol_base_path;
    } else {
        hostfull = "//" +  std::string(hdr) +
            c_conf->webcontrol_base_path;
    }

    LOG_MSG(DBG, NO_ERRNO, "Full Host:  %s", hostfull.c_str());

    return;
}

/* Log the failed authentication check */
void cls_webua::failauth_log(bool userid_fail)
{
    timespec            tm_cnct;
    ctx_webu_clients    p_clients;
    std::list<ctx_webu_clients>::iterator   it;

    LOG_MSG(ALR, NO_ERRNO
            ,"Failed authentication from %s", clientip.c_str());

    clock_gettime(CLOCK_MONOTONIC, &tm_cnct);

    it = c_webu->clients.begin();
    while (it != c_webu->clients.end()) {
        if (it->clientip == clientip) {
            it->conn_nbr++;
            it->conn_time.tv_sec =tm_cnct.tv_sec;
            it->authenticated = false;
            if (userid_fail) {
                it->userid_fail_nbr++;
            }
            return;
        }
        it++;
    }

    p_clients.clientip = clientip;
    p_clients.conn_nbr = 1;
    p_clients.conn_time = tm_cnct;
    p_clients.authenticated = false;
    if (userid_fail) {
        p_clients.userid_fail_nbr = 1;
    } else {
        p_clients.userid_fail_nbr = 0;
    }

    c_webu->clients.push_back(p_clients);
}

void cls_webua::client_connect()
{
    timespec  tm_cnct;
    ctx_webu_clients  p_clients;
    std::list<ctx_webu_clients>::iterator   it;

    clock_gettime(CLOCK_MONOTONIC, &tm_cnct);

    /* First we need to clean out any old IPs from the list*/
    it = c_webu->clients.begin();
    while (it !=c_webu->clients.end()) {
        if ((tm_cnct.tv_sec - it->conn_time.tv_sec) >=
            (c_conf->webcontrol_lock_minutes*60)) {
            it = c_webu->clients.erase(it);
        }
        it++;
    }

    /* When this function is called, we know that we are authenticated
     * so we reset the info and as needed print a message that the
     * ip is connected.
     */
    it = c_webu->clients.begin();
    while (it != c_webu->clients.end()) {
        if (it->clientip == clientip) {
            if (it->authenticated == false) {
                LOG_MSG(INF, NO_ERRNO, "Connection from: %s",clientip.c_str());
            }
            it->authenticated = true;
            it->conn_nbr = 1;
            it->userid_fail_nbr = 0;
            it->conn_time.tv_sec = tm_cnct.tv_sec;
            return;
        }
        it++;
    }

    /* The ip was not already in our list. */
    p_clients.clientip = clientip;
    p_clients.conn_nbr = 1;
    p_clients.userid_fail_nbr = 0;
    p_clients.conn_time = tm_cnct;
    p_clients.authenticated = true;
    c_webu->clients.push_back(p_clients);

    LOG_MSG(INF, NO_ERRNO, "Connection from: %s",clientip.c_str());

    return;

}

/* Check for ips with excessive failed authentication attempts */
mhdrslt cls_webua::failauth_check()
{
    timespec                                tm_cnct;
    std::list<ctx_webu_clients>::iterator   it;
    std::string                             tmp;

    if (c_webu->clients.size() == 0) {
        return MHD_YES;
    }

    clock_gettime(CLOCK_MONOTONIC, &tm_cnct);
    it = c_webu->clients.begin();
    while (it != c_webu->clients.end()) {
        if ((it->clientip == clientip) &&
            ((tm_cnct.tv_sec - it->conn_time.tv_sec) <
             (c_conf->webcontrol_lock_minutes*60)) &&
            (it->authenticated == false) &&
            (it->conn_nbr > c_conf->webcontrol_lock_attempts)) {
            LOG_MSG(EMG, NO_ERRNO
                , "Ignoring connection from: %s"
                , clientip.c_str());
            it->conn_time = tm_cnct;
            return MHD_NO;
        } else if ((tm_cnct.tv_sec - it->conn_time.tv_sec) >=
            (c_conf->webcontrol_lock_minutes*60)) {
            it = c_webu->clients.erase(it);
        } else {
            it++;
        }
    }

    return MHD_YES;

}

/* Create a authorization denied response to user*/
mhdrslt cls_webua::mhd_digest_fail(int signal_stale)
{
    struct MHD_Response *response;
    mhdrslt retcd;

    authenticated = false;

    resp_page = "<html><head><title>Access denied</title>"
        "</head><body>Access denied</body></html>";

    response = MHD_create_response_from_buffer(resp_page.length()
        ,(void *)resp_page.c_str(), MHD_RESPMEM_PERSISTENT);

    if (response == NULL) {
        return MHD_NO;
    }

    retcd = MHD_queue_auth_fail_response(connection, auth_realm
        ,auth_opaque, response
        ,(signal_stale == MHD_INVALID_NONCE) ? MHD_YES : MHD_NO);

    MHD_destroy_response(response);

    return retcd;
}

/* Perform digest authentication */
mhdrslt cls_webua::mhd_digest()
{
    /* This function gets called a couple of
     * times by MHD during the authentication process.
     */
    int retcd;
    char *user;

    /*Get username or prompt for a user/pass */
    user = MHD_digest_auth_get_username(connection);
    if (user == NULL) {
        return mhd_digest_fail(MHD_NO);
    }

    /* Check for valid user name */
    if (mystrne(user, auth_user)) {
        failauth_log(true);
        myfree(user);
        return mhd_digest_fail(MHD_NO);
    }
    myfree(user);

    /* Check the password as well*/
    retcd = MHD_digest_auth_check(connection, auth_realm
        , auth_user, auth_pass, 300);

    if (retcd == MHD_NO) {
        failauth_log(false);
    }

    if ( (retcd == MHD_INVALID_NONCE) || (retcd == MHD_NO) )  {
        return mhd_digest_fail(retcd);
    }

    authenticated = true;
    return MHD_YES;

}

/* Create a authorization denied response to user*/
mhdrslt cls_webua::mhd_basic_fail()
{
    struct MHD_Response *response;
    int retcd;

    authenticated = false;

    resp_page = "<html><head><title>Access denied</title>"
        "</head><body>Access denied</body></html>";

    response = MHD_create_response_from_buffer(
        resp_page.length()
        ,(void *)resp_page.c_str()
        , MHD_RESPMEM_PERSISTENT);

    if (response == NULL) {
        return MHD_NO;
    }

    retcd = MHD_queue_basic_auth_fail_response (
        connection, auth_realm, response);

    MHD_destroy_response(response);

    if (retcd == MHD_YES) {
        return MHD_YES;
    } else {
        return MHD_NO;
    }

}

/* Perform Basic Authentication.  */
mhdrslt cls_webua::mhd_basic()
{
    char *user, *pass;

    pass = NULL;
    user = NULL;

    user = MHD_basic_auth_get_username_password (connection, &pass);
    if ((user == NULL) || (pass == NULL)) {
        myfree(user);
        myfree(pass);
        return mhd_basic_fail();
    }

    if ((mystrne(user, auth_user)) ||
        (mystrne(pass, auth_pass))) {
        failauth_log(mystrne(user, auth_user));
        myfree(user);
        myfree(pass);
        return mhd_basic_fail();
    }

    myfree(user);
    myfree(pass);

    authenticated = true;

    return MHD_YES;

}

/* Parse apart the user:pass provided*/
void cls_webua::mhd_auth_parse()
{
    int auth_len;
    char *col_pos;

    myfree(auth_user);
    myfree(auth_pass);

    auth_len = (int)c_conf->webcontrol_authentication.length();
    col_pos =(char*) strstr(c_conf->webcontrol_authentication.c_str() ,":");
    if (col_pos == NULL) {
        auth_user = (char*)mymalloc(auth_len+1);
        auth_pass = (char*)mymalloc(2);
        snprintf(auth_user, auth_len + 1, "%s"
            ,c_conf->webcontrol_authentication.c_str());
        snprintf(auth_pass, 2, "%s","");
    } else {
        auth_user = (char*)mymalloc(auth_len - strlen(col_pos) + 1);
        auth_pass =(char*)mymalloc(strlen(col_pos));
        snprintf(auth_user, auth_len - strlen(col_pos) + 1, "%s"
            ,c_conf->webcontrol_authentication.c_str());
        snprintf(auth_pass, strlen(col_pos), "%s", col_pos + 1);
    }

}

/* Initialize for authorization */
mhdrslt cls_webua::mhd_auth()
{
    unsigned int rand1,rand2;

    srand((unsigned int)time(NULL));
    rand1 = (unsigned int)(42000000.0 * rand() / (RAND_MAX + 1.0));
    rand2 = (unsigned int)(42000000.0 * rand() / (RAND_MAX + 1.0));
    snprintf(auth_opaque, WEBUA_LEN_PARM, "%08x%08x", rand1, rand2);

    snprintf(auth_realm, WEBUA_LEN_PARM, "%s","Restream");

    if (c_conf->webcontrol_authentication == "") {
        authenticated = true;
        if (c_conf->webcontrol_auth_method != "none") {
            LOG_MSG(NTC, NO_ERRNO ,"No webcontrol user:pass provided");
        }
        return MHD_YES;
    }

    if (auth_user == NULL) {
        mhd_auth_parse();
    }

    if (c_conf->webcontrol_auth_method == "basic") {
        return mhd_basic();
    } else if (c_conf->webcontrol_auth_method == "digest") {
        return mhd_digest();
    }


    authenticated = true;
    return MHD_YES;

}

/* Send the response that we created back to the user.  */
mhdrslt cls_webua::mhd_send()
{
    mhdrslt retcd;
    struct MHD_Response *response;
    std::list<ctx_params_item>::iterator    it;

    response = MHD_create_response_from_buffer(resp_page.length()
        ,(void *)resp_page.c_str(), MHD_RESPMEM_PERSISTENT);
    if (!response) {
        LOG_MSG(ERR, NO_ERRNO, "Invalid response");
        return MHD_NO;
    }

    for (it  = c_webu->headers.params_array.begin();
         it != c_webu->headers.params_array.end(); it++) {
        MHD_add_response_header (response
            , it->param_name.c_str(), it->param_value.c_str());
    }

    if (resp_type == WEBUA_RESP_TEXT) {
        MHD_add_response_header (response, MHD_HTTP_HEADER_CONTENT_TYPE, "text/plain;");
    } else if (resp_type == WEBUA_RESP_JSON) {
        MHD_add_response_header (response, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json;");
    } else {
        MHD_add_response_header (response, MHD_HTTP_HEADER_CONTENT_TYPE, "text/html");
    }

    retcd = MHD_queue_response (connection, MHD_HTTP_OK, response);
    MHD_destroy_response (response);

    return retcd;
}

void cls_webua::stream_cnct_cnt()
{
    int indx, chk;

    if (chitm->cnct_cnt == 0) {
        for (indx=0; indx < (int)chitm->pktarray->array.size(); indx++) {
            if (chitm->pktarray->array[indx].packet != nullptr) {
                mypacket_free(chitm->pktarray->array[indx].packet);
                chitm->pktarray->array[indx].packet = nullptr;
            }
        }
        chitm->cnct_cnt++;
        chitm->pktarray->start = chitm->pktarray->count;
        chk = 0;
        while ((chitm->pktarray->start > 0) && (chk <100000)) {
            SLEEP(0,10000L);
            chk++;
        }
    } else {
        chitm->cnct_cnt++;
    }
}

int cls_webua::stream_type()
{
    if ((uri_cmd1 == "mpegts") ||
        (uri_cmd1 == "mpegts.ts") ||
        (uri_cmd1 == "mpeg.ts")) {
        if (uri_cmd2 == "stream") {
            cnct_type = WEBUA_CNCT_TS_FULL;
        } else if (uri_cmd2 == "") {
            cnct_type = WEBUA_CNCT_TS_FULL;
        } else {
            cnct_type = WEBUA_CNCT_UNKNOWN;
            return -1;
        }
    } else if ((uri_cmd1 == "m3u8") ||
        (uri_cmd1 == "mpegts.m3u8") ||
        (uri_cmd1 == "ts.m3u8")) {
        cnct_type = WEBUA_CNCT_M3U8;
    } else if ((uri_cmd1 == "xmltv") ||
        (uri_cmd1 == "xmltv.epg") ||
        (uri_cmd1 == "epg")) {
        cnct_type = WEBUA_CNCT_XMLTV;
    } else if ((uri_cmd1 == "xmltvall") ||
        (uri_cmd1 == "all.epg") ||
        (uri_cmd1 == "allepg")) {
        cnct_type = WEBUA_CNCT_XMLTV_ALL;
    } else {
        cnct_type = WEBUA_CNCT_UNKNOWN;
        return -1;
    }
    return 0;
}

int cls_webua::stream_checks()
{
    if (channel_indx == -1) {
        LOG_MSG(ERR, NO_ERRNO
            , "Invalid channel specified: %s",url.c_str());
        return -1;
    }
    if (channel_indx < 0) {
        LOG_MSG(ERR, NO_ERRNO
            , "Invalid channel specified: %s",url.c_str());
            return -1;
    }

    if (c_webu->wb_finish == true) {
        return -1;
    }

    return 0;
}

/* Answer the get request from the user */
mhdrslt cls_webua::answer_get()
{
    LOG_MSG(DBG, NO_ERRNO ,"processing get");

    if (chitm == nullptr) {        
        html_badreq();
        return mhd_send();
    }

    if (stream_type() == -1) {
        html_badreq();
        return mhd_send();
    }

    if (stream_checks() == -1) {
        html_badreq();
        return mhd_send();
    }

    if ((cnct_type == WEBUA_CNCT_TS_FULL)) {
        LOG_MSG(INF, NO_ERRNO, "Starting Stream");
        stream_cnct_cnt();
        if (c_webuts == nullptr) {
           c_webuts = new cls_webuts(app, this);
        }
        return c_webuts->main();
    } else if ((cnct_type == WEBUA_CNCT_M3U8) ||
        (cnct_type == WEBUA_CNCT_XMLTV) ||
        (cnct_type == WEBUA_CNCT_XMLTV_ALL)) {
        LOG_MSG(INF, NO_ERRNO, "Getting metadata");
        if (c_webum == nullptr) {
           c_webum = new cls_webum(app, this);
        }        
        c_webum->main(resp_page, resp_type);
        return mhd_send();
    } else {
        resp_page = "<html><head><title>Sample Page</title>"
            "</head><body>Sample Page</body></html>";
        return mhd_send();
    }
    /* Should not get here*/
    return MHD_NO;
}

/* Answer the connection request for the webcontrol*/
mhdrslt cls_webua::answer_main(struct MHD_Connection *p_connection
    , const char *method, const char *upload_data, size_t *upload_data_size)
{
    mhdrslt retcd;

    cnct_type = WEBUA_CNCT_CONTROL;
    connection = p_connection;

    if (c_webu->wb_finish) {
        LOG_MSG(NTC, NO_ERRNO ,"Shutting down channels");
        return MHD_NO;
    }

    if (chitm != NULL) {
        if (chitm->ch_finish) {
           LOG_MSG(NTC, NO_ERRNO ,"Shutting down channels");
           return MHD_NO;
        }
    }

    if (clientip.length() == 0) {
        get_clientip();
    }

    if (failauth_check() == MHD_NO) {
        return MHD_NO;
    }

    if (authenticated == false) {
        retcd = mhd_auth();
        if (authenticated == false) {
            return retcd;
        }
    }

    client_connect();

    if (mhd_first) {
        mhd_first = false;
        if (mystreq(method,"POST")) {
            if (c_webup == nullptr) {
                c_webup = new cls_webup(c_app, this);
            }
            cnct_method = WEBUA_METHOD_POST;
            return c_webup->processor_init();
        } else {
            cnct_method = WEBUA_METHOD_GET;
            return MHD_YES;
        }
    }
    
    if (hostfull == "") {
        get_hostname();
    }

    if (mystreq(method,"POST")) {
        retcd = c_webup->processor_start(upload_data, upload_data_size);
    } else {
        if (url.length() == 0) {
            LOG_MSG(NTC, NO_ERRNO ,"Url length is zero");
            html_badreq();
            retcd = mhd_send();            
        } else {
            answer_get();
            retcd = MHD_YES;
        }
    }
    return retcd;

}

cls_webua::cls_webua(cls_app *p_app, const char *uri)
{
    c_app = p_app;
    c_webu = p_app->webu;
    c_conf = p_app->conf;
    c_webuts = nullptr;
    c_webum = nullptr;
    c_webup = nullptr;

    url           = "";
    uri_chid      = "";
    uri_cmd1      = "";
    uri_cmd2      = "";
    uri_cmd3      = "";
    clientip      = "";
    hostfull        = "";

    auth_opaque   = (char*)mymalloc(WEBUA_LEN_PARM);
    auth_realm    = (char*)mymalloc(WEBUA_LEN_PARM);
    auth_user     = NULL;                        /* Buffer to hold the user name*/
    auth_pass     = NULL;                        /* Buffer to hold the password */
    authenticated = false;                       /* boolean for whether we are authenticated*/

    resp_page     = "";                          /* The response being constructed */
    cnct_type     = WEBUA_CNCT_UNKNOWN;
    resp_type     = WEBUA_RESP_HTML;             /* Default to html response */
    cnct_method   = WEBUA_METHOD_GET;
    channel_indx  = -1;
    chitm  = nullptr;

    mhd_first = true;

    parms_edit(uri);

}

cls_webua::~cls_webua()
{
    myfree(auth_user);
    myfree(auth_pass);
    myfree(auth_opaque);
    myfree(auth_realm);
    mydelete(c_webuts);
    mydelete(c_webum);
    mydelete(c_webup);
}



