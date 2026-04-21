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

#ifndef _INCLUDE_WEBU_HPP_
#define _INCLUDE_WEBU_HPP_
    #define WEBU_MHD_OPTS 10           /* Maximum number of options permitted for MHD */
    #define WEBUI_POST_BFRSZ  512
    
    struct ctx_mhdstart {
        std::string             tls_cert;
        std::string             tls_key;
        bool                    tls_use;
        struct MHD_OptionItem   *mhd_ops;
        int                     mhd_opt_nbr;
        unsigned int            mhd_flags;
        int                     ipv6;
        struct sockaddr_in      lpbk_ipv4;
        struct sockaddr_in6     lpbk_ipv6;
    };
    struct ctx_key {
        char                        *key_nm;        /* Name of the key item */
        char                        *key_val;       /* Value of the key item */
        size_t                      key_sz;         /* The size of the value */
    };

    class cls_webu {
        public:
            cls_webu(cls_app *p_app);
            ~cls_webu();

            bool        wb_running;
            bool        wb_finish;
            ctx_params  headers;
            char        digest_rand[12];
            struct MHD_Daemon               *daemon;
            std::list<ctx_webu_clients>      clients;
        private:
            cls_app *c_app;
            void mhd_features_basic();
            void mhd_features_digest();
            void mhd_features_ipv6(ctx_mhdstart *mhdst);
            void mhd_features_tls(ctx_mhdstart *mhdst);
            void mhd_features(ctx_mhdstart *mhdst);
            void mhd_loadfile(std::string fname, std::string &filestr);
            void mhd_checktls(ctx_mhdstart *mhdst);
            void mhd_opts_init(ctx_mhdstart *mhdst);
            void mhd_opts_deinit(ctx_mhdstart *mhdst);
            void mhd_opts_localhost(ctx_mhdstart *mhdst);
            void mhd_opts_digest(ctx_mhdstart *mhdst);
            void mhd_opts_tls(ctx_mhdstart *mhdst);
            void mhd_opts(ctx_mhdstart *mhdst);
            void mhd_flags(ctx_mhdstart *mhdst);
    };

#endif /* _INCLUDE_WEBU_HPP_ */
