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


void cls_config::parm_set_bool(bool &parm_dest, std::string &parm_in)
{
    if ((parm_in == "1") || (parm_in == "yes") || (parm_in == "on") || (parm_in == "true") ) {
        parm_dest = true;
    } else {
        parm_dest = false;
    }
}

void cls_config::parm_get_bool(std::string &parm_dest, bool &parm_in)
{
    if (parm_in == true) {
        parm_dest = "on";
    } else {
        parm_dest = "off";
    }
}

void cls_config::edit_log_file(std::string &parm, enum PARM_ACT pact)
{
    char    lognm[4096];
    tm      *logtm;
    time_t  logt;

    if (pact == PARM_ACT_DFLT) {
        log_file = "";
    } else if (pact == PARM_ACT_SET) {
        time(&logt);
        logtm = localtime(&logt);
        strftime(lognm, 4096, parm.c_str(), logtm);
        log_file = lognm;
    } else if (pact == PARM_ACT_GET) {
        parm = log_file;
    }
    return;
}

void cls_config::edit_log_level(std::string &parm, enum PARM_ACT pact)
{
    int parm_in;
    if (pact == PARM_ACT_DFLT) {
        log_level = 6;
    } else if (pact == PARM_ACT_SET) {
        parm_in = atoi(parm.c_str());
        if ((parm_in < 1) || (parm_in > 9)) {
            LOG_MSG(NTC,  NO_ERRNO, "Invalid log_level %d",parm_in);
        } else {
            log_level = parm_in;
        }
    } else if (pact == PARM_ACT_GET) {
        parm = std::to_string(log_level);
    } else if (pact == PARM_ACT_LIST) {
        parm = "[";
        parm = parm + "\"1\",\"2\",\"3\",\"4\",\"5\"";
        parm = parm + ",\"6\",\"7\",\"8\",\"9\"";
        parm = parm + "]";
    }

    return;
}

void cls_config::edit_epg_socket(std::string &parm, enum PARM_ACT pact)
{
    if (pact == PARM_ACT_DFLT) {
        epg_socket = "/var/lib/tvheadend/epggrab/xmltv.sock";
    } else if (pact == PARM_ACT_SET) {
        if (parm == "/") {
            LOG_MSG(NTC, NO_ERRNO, "Invalid epg_socket");
        } else if (parm.length() >= 1) {
            if (parm.substr(0, 1) != "/") {
                LOG_MSG(NTC, NO_ERRNO
                    , "Invalid epg_socket:  Must start with a / ");
                epg_socket = "/" + parm;
            } else {
                epg_socket = parm;
            }
        } else {
            epg_socket = parm;
        }
    } else if (pact == PARM_ACT_GET) {
        parm = epg_socket;
    }
    return;
}

void cls_config::edit_language_code(std::string &parm, enum PARM_ACT pact)
{
    if (pact == PARM_ACT_DFLT) {
        language_code = "eng";
    } else if (pact == PARM_ACT_SET) {
        language_code = parm;
    } else if (pact == PARM_ACT_GET) {
        parm = language_code;
    }
    return;
}

void cls_config::edit_log_fflevel(std::string &parm, enum PARM_ACT pact)
{
    int parm_in;
    if (pact == PARM_ACT_DFLT) {
        log_fflevel = 2;
    } else if (pact == PARM_ACT_SET) {
        parm_in = atoi(parm.c_str());
        if ((parm_in < 1) || (parm_in > 9)) {
            LOG_MSG(NTC,  NO_ERRNO, "Invalid log_fflevel %d",parm_in);
        } else {
            log_fflevel = parm_in;
        }
    } else if (pact == PARM_ACT_GET) {
        parm = std::to_string(log_fflevel);
    } else if (pact == PARM_ACT_LIST) {
        parm = "[";
        parm = parm + "\"1\",\"2\",\"3\",\"4\",\"5\"";
        parm = parm + ",\"6\",\"7\",\"8\",\"9\"";
        parm = parm + "]";
    }

    return;
}

void cls_config::edit_webcontrol_port(std::string &parm, enum PARM_ACT pact)
{
    int parm_in;
    if (pact == PARM_ACT_DFLT) {
        webcontrol_port = 0;
    } else if (pact == PARM_ACT_SET) {
        parm_in = atoi(parm.c_str());
        if ((parm_in < 0) || (parm_in > 65535)) {
            LOG_MSG(NTC, NO_ERRNO, "Invalid webcontrol_port %d",parm_in);
        } else {
            webcontrol_port = parm_in;
        }
    } else if (pact == PARM_ACT_GET) {
        parm = std::to_string(webcontrol_port);
    }
    return;
}

void cls_config::edit_webcontrol_base_path(std::string &parm, enum PARM_ACT pact)
{
    if (pact == PARM_ACT_DFLT) {
        webcontrol_base_path = "";
    } else if (pact == PARM_ACT_SET) {
        if (parm == "/") {
            LOG_MSG(NTC, NO_ERRNO
                , "Invalid webcontrol_base_path: Use blank instead of single / ");
            webcontrol_base_path = "";
        } else if (parm.length() >= 1) {
            if (parm.substr(0, 1) != "/") {
                LOG_MSG(NTC, NO_ERRNO
                    , "Invalid webcontrol_base_path:  Must start with a / ");
                webcontrol_base_path = "/" + parm;
            } else {
                webcontrol_base_path = parm;
            }
        } else {
            webcontrol_base_path = parm;
        }
    } else if (pact == PARM_ACT_GET) {
        parm = webcontrol_base_path;
    }
    return;
}

void cls_config::edit_webcontrol_ipv6(std::string &parm, enum PARM_ACT pact)
{
    if (pact == PARM_ACT_DFLT) {
        webcontrol_ipv6 = false;
    } else if (pact == PARM_ACT_SET) {
        parm_set_bool(webcontrol_ipv6, parm);
    } else if (pact == PARM_ACT_GET) {
        parm_get_bool(parm, webcontrol_ipv6);
    }
    return;
}

void cls_config::edit_webcontrol_localhost(std::string &parm, enum PARM_ACT pact)
{
    if (pact == PARM_ACT_DFLT) {
        webcontrol_localhost = true;
    } else if (pact == PARM_ACT_SET) {
        parm_set_bool(webcontrol_localhost, parm);
    } else if (pact == PARM_ACT_GET) {
        parm_get_bool(parm, webcontrol_localhost);
    }
    return;
}

void cls_config::edit_webcontrol_parms(std::string &parm, enum PARM_ACT pact)
{
    int parm_in;
    if (pact == PARM_ACT_DFLT) {
        webcontrol_parms = 0;
    } else if (pact == PARM_ACT_SET) {
        parm_in = atoi(parm.c_str());
        if ((parm_in < 0) || (parm_in > 3)) {
            LOG_MSG(NTC, NO_ERRNO, "Invalid webcontrol_parms %d",parm_in);
        } else {
            webcontrol_parms = parm_in;
        }
    } else if (pact == PARM_ACT_GET) {
        parm = std::to_string(webcontrol_parms);
    } else if (pact == PARM_ACT_LIST) {
        parm = "[";
        parm = parm +  "\"0\",\"1\",\"2\",\"3\"";
        parm = parm + "]";
    }
    return;
}

void cls_config::edit_webcontrol_interface(std::string &parm, enum PARM_ACT pact)
{
    if (pact == PARM_ACT_DFLT) {
        webcontrol_interface = "default";
    } else if (pact == PARM_ACT_SET) {
        if ((parm == "default") || (parm == "user"))  {
            webcontrol_interface = parm;
        } else if (parm == "") {
            webcontrol_interface = "default";
        } else {
            LOG_MSG(NTC, NO_ERRNO, "Invalid webcontrol_interface %s", parm.c_str());
        }
    } else if (pact == PARM_ACT_GET) {
        parm = webcontrol_interface;
    } else if (pact == PARM_ACT_LIST) {
        parm = "[";
        parm = parm +  "\"default\",\"user\"";
        parm = parm + "]";
    }

    return;
}

void cls_config::edit_webcontrol_auth_method(std::string &parm, enum PARM_ACT pact)
{
    if (pact == PARM_ACT_DFLT) {
        webcontrol_auth_method = "none";
    } else if (pact == PARM_ACT_SET) {
        if ((parm == "none") || (parm == "basic") || (parm == "digest"))  {
            webcontrol_auth_method = parm;
        } else if (parm == "") {
            webcontrol_auth_method = "none";
        } else {
            LOG_MSG(NTC, NO_ERRNO, "Invalid webcontrol_auth_method %s", parm.c_str());
        }
    } else if (pact == PARM_ACT_GET) {
        parm = webcontrol_auth_method;
    } else if (pact == PARM_ACT_LIST) {
        parm = "[";
        parm = parm +  "\"none\",\"basic\",\"digest\"";
        parm = parm + "]";
    }
    return;
}

void cls_config::edit_webcontrol_authentication(std::string &parm, enum PARM_ACT pact)
{
    if (pact == PARM_ACT_DFLT) {
        webcontrol_authentication = "";
    } else if (pact == PARM_ACT_SET) {
        webcontrol_authentication = parm;
    } else if (pact == PARM_ACT_GET) {
        parm = webcontrol_authentication;
    }
    return;
}

void cls_config::edit_webcontrol_tls(std::string &parm, enum PARM_ACT pact)
{
    if (pact == PARM_ACT_DFLT) {
        webcontrol_tls = false;
    } else if (pact == PARM_ACT_SET) {
        parm_set_bool(webcontrol_tls, parm);
    } else if (pact == PARM_ACT_GET) {
        parm_get_bool(parm, webcontrol_tls);
    }
    return;
}

void cls_config::edit_webcontrol_cert(std::string &parm, enum PARM_ACT pact)
{
    if (pact == PARM_ACT_DFLT) {
        webcontrol_cert = "";
    } else if (pact == PARM_ACT_SET) {
        webcontrol_cert = parm;
    } else if (pact == PARM_ACT_GET) {
        parm = webcontrol_cert;
    }
    return;
}

void cls_config::edit_webcontrol_key(std::string &parm, enum PARM_ACT pact)
{
    if (pact == PARM_ACT_DFLT) {
        webcontrol_key = "";
    } else if (pact == PARM_ACT_SET) {
        webcontrol_key = parm;
    } else if (pact == PARM_ACT_GET) {
        parm = webcontrol_key;
    }
    return;
}

void cls_config::edit_webcontrol_headers(std::string &parm, enum PARM_ACT pact)
{
    if (pact == PARM_ACT_DFLT) {
        webcontrol_headers = "";
    } else if (pact == PARM_ACT_SET) {
        webcontrol_headers = parm;
    } else if (pact == PARM_ACT_GET) {
        parm = webcontrol_headers;
    }
    return;
}

void cls_config::edit_webcontrol_html(std::string &parm, enum PARM_ACT pact)
{
    if (pact == PARM_ACT_DFLT) {
        webcontrol_html = "";
    } else if (pact == PARM_ACT_SET) {
        webcontrol_html = parm;
    } else if (pact == PARM_ACT_GET) {
        parm = webcontrol_html;
    }
    return;
}

void cls_config::edit_webcontrol_lock_minutes(std::string &parm, enum PARM_ACT pact)
{
    int parm_in;
    if (pact == PARM_ACT_DFLT) {
        webcontrol_lock_minutes = 10;
    } else if (pact == PARM_ACT_SET) {
        parm_in = atoi(parm.c_str());
        if (parm_in < 0) {
            LOG_MSG(NTC, NO_ERRNO, "Invalid webcontrol_lock_minutes %d",parm_in);
        } else {
            webcontrol_lock_minutes = parm_in;
        }
    } else if (pact == PARM_ACT_GET) {
        parm = std::to_string(webcontrol_lock_minutes);
    }
    return;
}

void cls_config::edit_webcontrol_lock_attempts(std::string &parm, enum PARM_ACT pact)
{
    int parm_in;
    if (pact == PARM_ACT_DFLT) {
        webcontrol_lock_attempts = 3;
    } else if (pact == PARM_ACT_SET) {
        parm_in = atoi(parm.c_str());
        if (parm_in < 0) {
            LOG_MSG(NTC, NO_ERRNO, "Invalid webcontrol_lock_attempts %d",parm_in);
        } else {
            webcontrol_lock_attempts = parm_in;
        }
    } else if (pact == PARM_ACT_GET) {
        parm = std::to_string(webcontrol_lock_attempts);
    }
    return;
}

void cls_config::edit_webcontrol_lock_script(std::string &parm, enum PARM_ACT pact)
{
    if (pact == PARM_ACT_DFLT) {
        webcontrol_lock_script = "";
    } else if (pact == PARM_ACT_SET) {
        webcontrol_lock_script = parm;
    } else if (pact == PARM_ACT_GET) {
        parm = webcontrol_lock_script;
    }
    return;
}

void cls_config::edit_channels(std::string &parm, enum PARM_ACT pact)
{
    if (pact == PARM_ACT_DFLT) {
        channels.clear();
        if (parm != "") {
            channels.push_back(parm);
        }
    } else if (pact == PARM_ACT_SET) {
        if (parm == "") {
            return;
        }
        channels.push_back(parm);   /* Add to the end of list*/
    } else if (pact == PARM_ACT_GET) {
        if (channels.empty()) {
            parm = "";
        } else {
            parm = channels.back();     /* Give last item*/
        }
    }
    return;
}

void cls_config::edit_channels(std::list<std::string> &parm, enum PARM_ACT pact)
{
    if (pact == PARM_ACT_DFLT) {
        channels.clear();
    } else if (pact == PARM_ACT_SET) {
        channels = parm;
    } else if (pact == PARM_ACT_GET) {
        parm = channels;
    }
    return;
}

void cls_config::edit_cat00(std::string parm_nm
        , std::string &parm_val, enum PARM_ACT pact)
{
    if (parm_nm == "log_file") {            edit_log_file(parm_val, pact);
    } else if (parm_nm == "log_level") {    edit_log_level(parm_val, pact);
    } else if (parm_nm == "log_fflevel") {  edit_log_fflevel(parm_val, pact);
    } else if (parm_nm == "epg_socket") {   edit_epg_socket(parm_val, pact);
    } else if (parm_nm == "language_code"){ edit_language_code(parm_val, pact);
    }
}

void cls_config::edit_cat01(std::string parm_nm
        , std::string &parm_val, enum PARM_ACT pact)
{
    if (parm_nm == "webcontrol_port") {                    edit_webcontrol_port(parm_val, pact);
    } else if (parm_nm == "webcontrol_base_path") {        edit_webcontrol_base_path(parm_val, pact);
    } else if (parm_nm == "webcontrol_ipv6") {             edit_webcontrol_ipv6(parm_val, pact);
    } else if (parm_nm == "webcontrol_localhost") {        edit_webcontrol_localhost(parm_val, pact);
    } else if (parm_nm == "webcontrol_parms") {            edit_webcontrol_parms(parm_val, pact);
    } else if (parm_nm == "webcontrol_interface") {        edit_webcontrol_interface(parm_val, pact);
    } else if (parm_nm == "webcontrol_auth_method") {      edit_webcontrol_auth_method(parm_val, pact);
    } else if (parm_nm == "webcontrol_authentication") {   edit_webcontrol_authentication(parm_val, pact);
    } else if (parm_nm == "webcontrol_tls") {              edit_webcontrol_tls(parm_val, pact);
    } else if (parm_nm == "webcontrol_cert") {             edit_webcontrol_cert(parm_val, pact);
    } else if (parm_nm == "webcontrol_key") {              edit_webcontrol_key(parm_val, pact);
    } else if (parm_nm == "webcontrol_headers") {          edit_webcontrol_headers(parm_val, pact);
    } else if (parm_nm == "webcontrol_html") {             edit_webcontrol_html(parm_val, pact);
    } else if (parm_nm == "webcontrol_lock_minutes") {     edit_webcontrol_lock_minutes(parm_val, pact);
    } else if (parm_nm == "webcontrol_lock_attempts") {    edit_webcontrol_lock_attempts(parm_val, pact);
    } else if (parm_nm == "webcontrol_lock_script") {      edit_webcontrol_lock_script(parm_val, pact);
    }
}

void cls_config::edit_cat02(std::string parm_nm
        ,std::string &parm_val, enum PARM_ACT pact)
{
    if (parm_nm == "channel") {  edit_channels(parm_val, pact);
    }
}

void cls_config::edit_cat02(std::string parm_nm
        ,std::list<std::string> &parm_val, enum PARM_ACT pact)
{
    if (parm_nm == "channel") {  edit_channels(parm_val, pact);
    }
}

void cls_config::edit_cat(std::string parm_nm
        , std::string &parm_val, enum PARM_ACT pact, enum PARM_CAT pcat)
{
    if (pcat == PARM_CAT_00) {          edit_cat00(parm_nm, parm_val, pact);
    } else if (pcat == PARM_CAT_01) {   edit_cat01(parm_nm, parm_val, pact);
    } else if (pcat == PARM_CAT_02) {   edit_cat02(parm_nm, parm_val, pact);
    }
}

void cls_config::edit_cat(std::string parm_nm
        ,std::list<std::string> &parm_val, enum PARM_ACT pact, enum PARM_CAT pcat)
{
    if (pcat == PARM_CAT_02) {
        edit_cat02(parm_nm, parm_val, pact);
    }
}

void cls_config::parm_get(std::string parm_nm
    , std::string &parm_val, enum PARM_CAT parm_cat)
{
    edit_cat(parm_nm, parm_val, PARM_ACT_GET, parm_cat);
}

void cls_config::parm_get(std::string parm_nm
    , std::list<std::string> &parm_val, enum PARM_CAT parm_cat)
{
    edit_cat(parm_nm, parm_val, PARM_ACT_GET, parm_cat);
}

/* Assign the parameter value */
void cls_config::parm_set(std::string parm_nm
        , std::string parm_val)
{
    std::vector<ctx_config_item>::iterator cfg_it;

    for (cfg_it = config_parms.begin();
        cfg_it != config_parms.end(); cfg_it++) {
        if (parm_nm == cfg_it->parm_name) {
            edit_cat(parm_nm, parm_val, PARM_ACT_SET, cfg_it->parm_cat);
            return;
        }
    }
    LOG_MSG(ALR, NO_ERRNO, "Unknown config option \"%s\"", parm_nm.c_str());
}

/* Get list of valid values for items only permitting a set*/
void cls_config::parm_list(std::string parm_nm, std::string &parm_val
        , enum PARM_CAT parm_cat)
{
    edit_cat(parm_nm, parm_val, PARM_ACT_LIST, parm_cat);
}

std::string cls_config::conf_type_desc(enum PARM_TYP ptype)
{
    if (ptype == PARM_TYP_BOOL) {           return "bool";
    } else if (ptype == PARM_TYP_INT) {     return "int";
    } else if (ptype == PARM_TYP_LIST) {    return "list";
    } else if (ptype == PARM_TYP_STRING) {  return "string";
    } else if (ptype == PARM_TYP_ARRAY) {   return "array";
    } else {                                return "error";
    }
}

/* Return a string describing the parameter category */
std::string cls_config::conf_cat_desc(enum PARM_CAT pcat, bool shrt)
{
    if (shrt) {
        if (pcat == PARM_CAT_00)        { return "app";
        } else if (pcat == PARM_CAT_01) { return "webcontrol";
        } else if (pcat == PARM_CAT_02) { return "channels";
        } else { return "unk";
        }
    } else {
        if (pcat == PARM_CAT_00)        { return "Application";
        } else if (pcat == PARM_CAT_01) { return "Webcontrol";
        } else if (pcat == PARM_CAT_02) { return "Channels";
        } else { return "Other";
        }
    }
}

/** Prints usage and options allowed from Command-line. */
void cls_config::usage()
{
    printf("Restream version %s, Copyright 2026\n","0.1.1");
    printf("\nusage:\trestream [options]\n");
    printf("\n\n");
    printf("Possible options:\n\n");
    printf("-c config\t\tFull path and filename of config file.\n");
    printf("-d level\t\tLog level (1-9) (EMG, ALR, CRT, ERR, WRN, NTC, INF, DBG, ALL). default: 6 / NTC.\n");
    printf("-l log file \t\tFull path and filename of log file.\n");
    printf("-h\t\t\tShow this screen.\n");
    printf("\n");
}

void cls_config::process_cmdline()
{
    int c;

    while ((c = getopt(c_app->argc, c_app->argv, "cdlh:")) != -1)
        switch (c) {
        case 'c':
            c_app->conf_file.assign(optarg);
            break;
        case 'd':
            cmd_log_level.assign(optarg);
            break;
        case 'l':
            cmd_log_file.assign(optarg);
            break;
        case 'h':
        case '?':
        default:
             usage();
             exit(1);
        }

    optind = 1;
}

/** Process each line from the config file. */
void cls_config::process_file()
{
    size_t stpos;
    std::string line, parm_nm, parm_vl;
    std::ifstream ifs;

    ifs.open(c_app->conf_file);
        if (ifs.is_open() == false) {
            LOG_MSG(ERR, NO_ERRNO
                , "config file not found: %s"
                , c_app->conf_file.c_str());
            return;
        }

        LOG_MSG(NTC, NO_ERRNO
            , "Processing config file %s"
            , c_app->conf_file.c_str());

        while (std::getline(ifs, line)) {
            mytrim(line);
            stpos = line.find(" ");
            if (line.find('\t') != std::string::npos) {
                if (line.find('\t') < stpos) {
                    stpos =line.find('\t');
                }
            }
            if (stpos > line.find("=")) {
                stpos = line.find("=");
            }
            if ((stpos != std::string::npos) &&
                (stpos != line.length()-1) &&
                (stpos != 0) &&
                (line.substr(0, 1) != ";") &&
                (line.substr(0, 1) != "#")) {
                parm_nm = line.substr(0, stpos);
                parm_vl = line.substr(stpos+1, line.length()-stpos);
                myunquote(parm_nm);
                myunquote(parm_vl);
                parm_set(parm_nm, parm_vl);
            } else if ((line != "") &&
                (line.substr(0, 1) != ";") &&
                (line.substr(0, 1) != "#") &&
                (stpos != std::string::npos) ) {
                LOG_MSG(ERR, NO_ERRNO
                , "Unable to parse line: %s", line.c_str());
            }
        }
    ifs.close();

}

/**  Write the configuration(s) to the log */
void cls_config::parms_log()
{
    std::string parm_vl;
    std::list<std::string> parm_array;
    std::list<std::string>::iterator it;
    std::vector<ctx_config_item>::iterator cfg_it;

    SHT_MSG(INF, NO_ERRNO
        ,"Logging parameters from config file: %s"
        , c_app->conf_file.c_str());

    for (cfg_it = config_parms.begin();
        cfg_it != config_parms.end(); cfg_it++) {
        if (cfg_it->parm_type == PARM_TYP_ARRAY) {
            parm_get(cfg_it->parm_name, parm_array, cfg_it->parm_cat);
            for (it = parm_array.begin(); it != parm_array.end(); it++) {
                SHT_MSG(INF, NO_ERRNO, "%-25s %s"
                    , cfg_it->parm_name.c_str(), it->c_str());
            }
        } else {
            parm_get(cfg_it->parm_name, parm_vl, cfg_it->parm_cat);
            SHT_MSG(INF, NO_ERRNO,"%-25s %s"
                , cfg_it->parm_name.c_str(), parm_vl.c_str());
        }
    }
}

void cls_config::parms_write_parms(
    FILE *conffile, std::string parm_nm
    , std::string parm_vl, enum PARM_CAT parm_ct, bool reset)
{
    static enum PARM_CAT prev_ct;

    if (reset) {
        prev_ct = PARM_CAT_00;
        return;
    }

    if (parm_ct != prev_ct) {
        fprintf(conffile,"\n%s",";*************************************************\n");
        fprintf(conffile,"%s%s\n", ";*****   ", conf_cat_desc(parm_ct,false).c_str());
        fprintf(conffile,"%s",";*************************************************\n");
        prev_ct = parm_ct;
    }

    if (parm_vl.compare(0, 1, " ") == 0) {
        fprintf(conffile, "%s \"%s\"\n", parm_nm.c_str(), parm_vl.c_str());
    } else {
        fprintf(conffile, "%s %s\n", parm_nm.c_str(), parm_vl.c_str());
    }
}

/**  Write the configuration(s) to file */
void cls_config::parms_write()
{
    std::string parm_vl;
    std::list<std::string> parm_array;
    std::list<std::string>::iterator it;
    std::vector<ctx_config_item>::iterator cfg_it;
    char timestamp[32];
    FILE *conffile;

    time_t now = time(0);
    strftime(timestamp, 32, "%Y-%m-%dT%H:%M:%S", localtime(&now));

    conffile = myfopen(c_app->conf_file.c_str(), "we");
    if (conffile == NULL) {
        LOG_MSG(NTC,  NO_ERRNO
            , "Failed to write configuration to %s"
            , c_app->conf_file.c_str());
        return;
    }

    fprintf(conffile, "; %s\n", c_app->conf_file.c_str());
    fprintf(conffile, "; at %s\n", timestamp);
    fprintf(conffile, "\n\n");

    parms_write_parms(conffile, "", "", PARM_CAT_00, true);

    for (cfg_it = config_parms.begin();
        cfg_it != config_parms.end(); cfg_it++) {
        if (cfg_it->parm_type == PARM_TYP_ARRAY) {
            parm_get(cfg_it->parm_name, parm_array,cfg_it->parm_cat);
            for (it = parm_array.begin(); it != parm_array.end(); it++) {
                parms_write_parms(conffile, cfg_it->parm_name
                    , it->c_str(), cfg_it->parm_cat, false);
            }
        } else {
            parm_get(cfg_it->parm_name, parm_vl, cfg_it->parm_cat);
            parms_write_parms(conffile, cfg_it->parm_name
                , parm_vl, cfg_it->parm_cat, false);
        }
    }

    fprintf(conffile, "\n");
    myfclose(conffile);

    LOG_MSG(NTC,  NO_ERRNO
        , "Configuration written to %s"
        , c_app->conf_file.c_str());
}

void cls_config::parms_dflt()
{
    std::vector<ctx_config_item>::iterator cfg_it;
    std::string dflt = "";

    for (cfg_it = config_parms.begin();
        cfg_it != config_parms.end(); cfg_it++) {
        edit_cat(cfg_it->parm_name, dflt, PARM_ACT_DFLT, cfg_it->parm_cat);
    }
}

void cls_config::parms_add(std::string pname, enum PARM_TYP ptyp
    , enum PARM_CAT pcat, enum WEBUI_LEVEL plvl)
{
    ctx_config_item  config_itm;

    config_itm.parm_name = pname;
    config_itm.parm_type = ptyp;
    config_itm.parm_cat = pcat;
    config_itm.webui_level = plvl;

    config_parms.push_back(config_itm);
}

void cls_config::parms_init()
{
    cmd_log_file = "";
    cmd_log_level = "";

    parms_add("log_file",                  PARM_TYP_STRING, PARM_CAT_00, WEBUI_LEVEL_ADVANCED);
    parms_add("log_level",                 PARM_TYP_LIST,   PARM_CAT_00, WEBUI_LEVEL_LIMITED);
    parms_add("log_fflevel",               PARM_TYP_INT,    PARM_CAT_00, WEBUI_LEVEL_LIMITED);
    parms_add("epg_socket",                PARM_TYP_STRING, PARM_CAT_00, WEBUI_LEVEL_LIMITED);
    parms_add("language_code",             PARM_TYP_STRING, PARM_CAT_00, WEBUI_LEVEL_LIMITED);
    parms_add("webcontrol_port",           PARM_TYP_INT,    PARM_CAT_01, WEBUI_LEVEL_ADVANCED);
    parms_add("webcontrol_port2",          PARM_TYP_INT,    PARM_CAT_01, WEBUI_LEVEL_ADVANCED);
    parms_add("webcontrol_base_path",      PARM_TYP_STRING, PARM_CAT_01, WEBUI_LEVEL_ADVANCED);
    parms_add("webcontrol_ipv6",           PARM_TYP_BOOL,   PARM_CAT_01, WEBUI_LEVEL_ADVANCED);
    parms_add("webcontrol_localhost",      PARM_TYP_BOOL,   PARM_CAT_01, WEBUI_LEVEL_ADVANCED);
    parms_add("webcontrol_parms",          PARM_TYP_LIST,   PARM_CAT_01, WEBUI_LEVEL_NEVER);
    parms_add("webcontrol_interface",      PARM_TYP_LIST,   PARM_CAT_01, WEBUI_LEVEL_ADVANCED);
    parms_add("webcontrol_auth_method",    PARM_TYP_LIST,   PARM_CAT_01, WEBUI_LEVEL_RESTRICTED);
    parms_add("webcontrol_authentication", PARM_TYP_STRING, PARM_CAT_01, WEBUI_LEVEL_RESTRICTED);
    parms_add("webcontrol_tls",            PARM_TYP_BOOL,   PARM_CAT_01, WEBUI_LEVEL_RESTRICTED);
    parms_add("webcontrol_cert",           PARM_TYP_STRING, PARM_CAT_01, WEBUI_LEVEL_RESTRICTED);
    parms_add("webcontrol_key",            PARM_TYP_STRING, PARM_CAT_01, WEBUI_LEVEL_RESTRICTED);
    parms_add("webcontrol_headers",        PARM_TYP_STRING, PARM_CAT_01, WEBUI_LEVEL_ADVANCED);
    parms_add("webcontrol_html",           PARM_TYP_STRING, PARM_CAT_01, WEBUI_LEVEL_ADVANCED);
    parms_add("webcontrol_lock_minutes",   PARM_TYP_INT,    PARM_CAT_01, WEBUI_LEVEL_ADVANCED);
    parms_add("webcontrol_lock_attempts",  PARM_TYP_INT,    PARM_CAT_01, WEBUI_LEVEL_ADVANCED);
    parms_add("webcontrol_lock_script",    PARM_TYP_STRING, PARM_CAT_01, WEBUI_LEVEL_RESTRICTED);
    parms_add("channel",                   PARM_TYP_ARRAY,  PARM_CAT_02, WEBUI_LEVEL_ADVANCED);
}

cls_config::cls_config(cls_app *p_app)
{
    c_app = p_app;

    std::string filename;
    char path[PATH_MAX];
    struct stat statbuf;

    parms_init();
    parms_dflt();
    process_cmdline();

    filename = "";
    if (c_app->conf_file != "") {
        filename = c_app->conf_file;
        if (stat(filename.c_str(), &statbuf) != 0) {
            filename="";
        }
    }

    if (filename == "") {
        if (getcwd(path, sizeof(path)) == NULL) {
            LOG_MSG(ERR,  SHOW_ERRNO, "Error getcwd");
            exit(-1);
        }
        filename = path + std::string("/restream.conf");
        if (stat(filename.c_str(), &statbuf) != 0) {
            filename = "";
        }
    }

    if (filename == "") {
        filename = std::string(getenv("HOME")) + std::string("/.restream/restream.conf");
        if (stat(filename.c_str(), &statbuf) != 0) {
            filename = "";
        }
    }

    if (filename == "") {
        filename = std::string( sysconfdir ) + std::string("/restream.conf");
        if (stat(filename.c_str(), &statbuf) != 0) {
            filename = "";
        }
    }

    if (filename == "") {
        LOG_MSG(ALR,  SHOW_ERRNO
            ,"Could not open configuration file");
        exit(-1);
    }

    c_app->conf_file = filename;

    process_file();

    if (cmd_log_file != "") {
        edit_log_file(cmd_log_file, PARM_ACT_SET);
    }

    if (cmd_log_level != "") {
        edit_log_level(cmd_log_level, PARM_ACT_SET);
    }
    c_app->log->log_level = log_level;
    c_app->log->log_fflevel = log_fflevel;
    c_app->log->set_log_file(log_file);

    parms_log();

}

cls_config::~cls_config()
{

}
