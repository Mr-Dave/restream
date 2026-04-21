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
#ifndef _INCLUDE_WEBU_ANS_HPP_
#define _INCLUDE_WEBU_ANS_HPP_
    #define WEBUA_LEN_PARM 512          /* Parameters specified */
    #define WEBUA_LEN_URLI 512          /* Maximum URL permitted */
    #define WEBUA_LEN_RESP 2048         /* Initial response size */
    #define WEBUA_POST_BFRSZ  512

    enum WEBUA_METHOD {
        WEBUA_METHOD_GET    = 0,
        WEBUA_METHOD_POST   = 1
    };

    enum WEBUA_CNCT {
        WEBUA_CNCT_CONTROL,
        WEBUA_CNCT_TS_FULL,
        WEBUA_CNCT_M3U8,
        WEBUA_CNCT_XMLTV,
        WEBUA_CNCT_UNKNOWN
    };

    enum WEBUA_RESP {
        WEBUA_RESP_HTML     = 0,
        WEBUA_RESP_JSON     = 1,
        WEBUA_RESP_TEXT     = 2
    };

    class cls_webua {
        public:
            cls_webua(cls_app *p_app, const char *uri);
            ~cls_webua();
            cls_channel             *chitm;
            enum WEBUA_CNCT         cnct_type;      /* Type of connection we are processing */
            struct MHD_Connection   *connection;    /* The MHD connection value from the client */
            std::string             hostfull;       /* Full http name for host with port number */
            std::string             resp_page;      /* The response that will be sent */
            enum WEBUA_RESP         resp_type;      /* indicator for the type of response to provide. */
            int                     channel_indx;   /* Index number of the channel */
            int                     channel_id;     /* channel id number requested */
            
            mhdrslt answer_main(
                struct MHD_Connection *p_connection, const char *method
                , const char *upload_data, size_t *upload_data_size);
            mhdrslt mhd_send();
            
        private:
            cls_app         *c_app;
            cls_config      *c_conf;
            cls_webu        *c_webu;
            cls_webuts      *c_webuts;
            cls_webum       *c_webum;
            cls_webup       *c_webup;

            char        *auth_opaque;   /* Opaque string for digest authentication*/
            char        *auth_realm;    /* Realm string for digest authentication*/
            char        *auth_user;     /* Parsed user from config authentication string*/
            char        *auth_pass;     /* Parsed password from config authentication string*/
            int         mhd_first;      /* Boolean for whether it is the first connection*/
            std::string clientip;       /* IP of the connecting client */
            bool        authenticated;  /* Boolean for whether authentication has been passed */
            std::string url;            /* The URL sent from the client */
            std::string uri_chid;       /* Parsed channel number from the url */
            std::string uri_cmd1;       /* Parsed command1 from the url */
            std::string uri_cmd2;       /* Parsed command2 from the url */
            std::string uri_cmd3;       /* Parsed command3 from the url */

            enum WEBUA_METHOD   cnct_method;    /* Connection method.  Get or Post */

            void    parseurl();
            void    parms_edit(const char *uri);
            void    failauth_log(bool userid_fail);
            void    html_badreq();
            void    get_clientip();
            void    get_hostname();
            mhdrslt answer_get();
            void    client_connect();

            mhdrslt mhd_digest_fail(int signal_stale);
            mhdrslt mhd_digest();
            mhdrslt mhd_basic_fail();
            mhdrslt mhd_basic();
            mhdrslt mhd_auth();
            void    mhd_auth_parse();
            mhdrslt failauth_check();

            void    stream_cnct_cnt();
            int     stream_type();
            int     stream_checks();

    };

#endif /* _INCLUDE_WEBU_ANS_HPP_ */