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
#include "webu_meta.hpp"

void cls_webum::get_m3u8(std::string &p_resp, enum WEBUA_RESP &p_type)
{
    if (c_webu->wb_finish == true) {
        p_resp = "";
        return;
    }

    LOG_MSG(NTC, NO_ERRNO, "Getting m3u8 data");

    /* These seem to work but may (probably are) not correct */
    p_resp =
        "#EXTM3U\n"
        "#EXT-X-TARGETDURATION:10\n"
        "#EXTINF:10.0 tvg-id=\"channel";
    p_resp += chitm->ch_nbr + "\", Ch ";
    p_resp += chitm->ch_nbr + "\n";
    p_resp += "http:" + c_webua->hostfull + "/";
    p_resp += chitm->ch_nbr + "/mpegts.ts\n";

    p_type = WEBUA_RESP_TEXT;
    //"#EXTINF:10.0,\n";
}

void cls_webum::get_xmltv(std::string &p_resp
                , enum WEBUA_RESP &p_type, cls_channel *p_chitm)
{
    int indx, indx2;
    std::string chid, dispnm, st,en;
    char    buf[4096];
    time_t          timechk;
    struct tm       *time_info;
    char            timebuf[1024];

    if (c_webu->wb_finish == true) {
        p_resp = "";
        return;
    }

    chid = "channel"+p_chitm->ch_nbr;

    p_resp += "<tv>\n";
    memset(buf,0,4096);
    snprintf(buf, 4096,
        "  <channel id=\"%s\">\n"
        "    <display-name>Ch %s</display-name>\n"
        "  </channel>\n"
        , chid.c_str(), p_chitm->ch_nbr.c_str());
    p_resp += buf;
    timechk = p_chitm->start_tm;
    for (indx=0;indx<p_chitm->playlist_count;indx++) {
        indx2 = p_chitm->playlist_index + indx;
        if (indx2 >= p_chitm->playlist_count) {
            indx2 -= p_chitm->playlist_count;
        }
        time_info = localtime(&timechk);
        strftime(timebuf, sizeof(timebuf), "%Y%m%d%H%M%S %z", time_info);
        st = timebuf;
        timechk += p_chitm->playlist[p_chitm->playlist_index].tm_dur;
        time_info = localtime(&timechk);
        strftime(timebuf, sizeof(timebuf), "%Y%m%d%H%M%S %z", time_info);
        en = timebuf;
        timechk++;
        memset(buf,0,4096);
        snprintf(buf, 4096,
        "  <programme start=\"%s\" stop=\"%s\" channel=\"%s\">\n"
        "    <title lang=\"en\">%s</title>\n"
        "  </programme>\n"
        , st.c_str(),en.c_str(),chid.c_str()
        , p_chitm->playlist[indx2].displaynm.c_str());
        p_resp += buf;
    }            
    p_resp += "</tv>\n";

    p_type = WEBUA_RESP_TEXT;

}

void cls_webum::main(std::string &p_resp, enum WEBUA_RESP &p_type)
{
    int indx;
    if (c_webu->wb_finish == true) {
        p_resp = "";
        return;
    }
    if (c_webua->cnct_type == WEBUA_CNCT_M3U8){
        get_m3u8(p_resp, p_type);
    } else if (c_webua->cnct_type == WEBUA_CNCT_XMLTV) {
        LOG_MSG(NTC, NO_ERRNO, "Getting epg data for channel %s"
            , chitm->ch_nbr.c_str());
        p_resp = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<!DOCTYPE tv SYSTEM \"xmlv.dtd\">\n";
        get_xmltv(p_resp, p_type, chitm);
    } else if (c_webua->cnct_type == WEBUA_CNCT_XMLTV_ALL) {
        LOG_MSG(NTC, NO_ERRNO, "Getting epg data for all channels");        
        p_resp = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<!DOCTYPE tv SYSTEM \"xmlv.dtd\">\n";
        for (indx=0;indx<c_app->ch_count;indx++) {
            get_xmltv(p_resp, p_type, c_app->channels[indx]);
        }
    }
}

cls_webum::cls_webum(cls_app *p_app, cls_webua *p_webua)
{
    c_app   = p_app;
    c_conf  = p_app->conf;
    c_webu  = p_app->webu;
    c_webua = p_webua;

    chitm       = c_webua->chitm;

}

cls_webum::~cls_webum()
{

}

