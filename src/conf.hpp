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

#ifndef _INCLUDE_CONF_HPP_
#define _INCLUDE_CONF_HPP_
    class cls_config {
        public:
            cls_config(cls_app *p_app);
            ~cls_config();
            std::string     log_file;
            int             log_level;
            int             log_fflevel;
            std::string     epg_socket;
            std::string     epg_dir;
            std::string     language_code;
            int             webcontrol_port;
            int             webcontrol_port2;
            std::string     webcontrol_base_path;
            bool            webcontrol_ipv6;
            bool            webcontrol_localhost;
            int             webcontrol_parms;
            std::string     webcontrol_interface;
            std::string     webcontrol_auth_method;
            std::string     webcontrol_authentication;
            bool            webcontrol_tls;
            std::string     webcontrol_cert;
            std::string     webcontrol_key;
            std::string     webcontrol_headers;
            std::string     webcontrol_html;
            int             webcontrol_lock_minutes;
            int             webcontrol_lock_attempts;
            std::string     webcontrol_lock_script;
            std::list<std::string>          channels;
            std::vector<ctx_config_item>    config_parms;
            void parm_set_bool(bool &parm_dest, std::string &parm_in);
            void parms_log();

        private:
            cls_app         *c_app;
            std::string     cmd_log_file;
            std::string     cmd_log_level;

            void parm_get_bool(std::string &parm_dest, bool &parm_in);
            void edit_log_file(std::string &parm, enum PARM_ACT pact);
            void edit_log_level(std::string &parm, enum PARM_ACT pact);
            void edit_log_fflevel(std::string &parm, enum PARM_ACT pact);
            void edit_epg_socket(std::string &parm, enum PARM_ACT pact);
            void edit_epg_dir(std::string &parm, enum PARM_ACT pact);
            void edit_language_code(std::string &parm, enum PARM_ACT pact);
            void edit_webcontrol_port(std::string &parm, enum PARM_ACT pact);
            void edit_webcontrol_base_path(std::string &parm, enum PARM_ACT pact);
            void edit_webcontrol_ipv6(std::string &parm, enum PARM_ACT pact);
            void edit_webcontrol_localhost(std::string &parm, enum PARM_ACT pact);
            void edit_webcontrol_parms(std::string &parm, enum PARM_ACT pact);
            void edit_webcontrol_interface(std::string &parm, enum PARM_ACT pact);
            void edit_webcontrol_auth_method(std::string &parm, enum PARM_ACT pact);
            void edit_webcontrol_authentication(std::string &parm, enum PARM_ACT pact);
            void edit_webcontrol_tls(std::string &parm, enum PARM_ACT pact);
            void edit_webcontrol_cert(std::string &parm, enum PARM_ACT pact);
            void edit_webcontrol_key(std::string &parm, enum PARM_ACT pact);
            void edit_webcontrol_headers(std::string &parm, enum PARM_ACT pact);
            void edit_webcontrol_html(std::string &parm, enum PARM_ACT pact);
            void edit_webcontrol_lock_minutes(std::string &parm, enum PARM_ACT pact);
            void edit_webcontrol_lock_attempts(std::string &parm, enum PARM_ACT pact);
            void edit_webcontrol_lock_script(std::string &parm, enum PARM_ACT pact);
            void edit_channels(std::string &parm, enum PARM_ACT pact);
            void edit_channels(std::list<std::string> &parm, enum PARM_ACT pact);
            void edit_cat00(std::string parm_nm, std::string &parm_val, enum PARM_ACT pact);
            void edit_cat01(std::string parm_nm, std::string &parm_val, enum PARM_ACT pact);
            void edit_cat02(std::string parm_nm,std::string &parm_val, enum PARM_ACT pact);
            void edit_cat02(std::string parm_nm,std::list<std::string> &parm_val, enum PARM_ACT pact);
            void edit_cat(std::string parm_nm, std::string &parm_val, enum PARM_ACT pact, enum PARM_CAT pcat);
            void edit_cat(std::string parm_nm,std::list<std::string> &parm_val, enum PARM_ACT pact, enum PARM_CAT pcat);
            void parm_get(std::string parm_nm, std::string &parm_val, enum PARM_CAT parm_cat);
            void parm_get(std::string parm_nm, std::list<std::string> &parm_val, enum PARM_CAT parm_cat);
            void parm_set(std::string parm_nm, std::string parm_val);
            void parm_list(std::string parm_nm, std::string &parm_val, enum PARM_CAT parm_cat);
            std::string conf_type_desc(enum PARM_TYP ptype);
            std::string conf_cat_desc(enum PARM_CAT pcat, bool shrt);
            void process_cmdline();
            void process_file();
            void parms_dflt();
            void parms_write_parms(FILE *conffile, std::string parm_nm, std::string parm_vl, enum PARM_CAT parm_ct, bool reset);
            void parms_write();
            void parms_add(std::string pname, enum PARM_TYP ptyp, enum PARM_CAT pcat, enum WEBUI_LEVEL plvl);
            void parms_init();
            void usage();
    };

#endif /* _INCLUDE_CONF_HPP_ */
