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

bool playlist_cmp(const ctx_playlist_item& a, const ctx_playlist_item& b)
{
    return a.fullnm < b.fullnm;
}

void cls_channel::playlist_load()
{
    DIR           *d;
    struct dirent *dir;
    ctx_playlist_item playitm;

    playlist_count = 0;
    playlist.clear();

    if (ch_finish == true) {
        return;
    }

    LOG_MSG(NTC, NO_ERRNO, "Playlist directory: %s", ch_dir.c_str());

    d = opendir(ch_dir.c_str());
    if (d) {
        while ((dir=readdir(d)) != NULL){
            if ((strstr(dir->d_name,".mkv") != NULL) ||
                (strstr(dir->d_name,".mp4") != NULL)) {
                playlist_count++;
                playitm.fullnm = ch_dir;
                playitm.fullnm += dir->d_name;
                playitm.filenm = dir->d_name;
                playitm.displaynm = playitm.filenm.substr(
                    0, playitm.filenm.find_last_of("."));
                playlist.push_back(playitm);
            }
        }
    }
    closedir(d);

    if (ch_sort == "alpha") {
        std::sort(playlist.begin(), playlist.end(), playlist_cmp);
    } else {
        std::shuffle(playlist.begin(), playlist.end()
            , std::mt19937{std::random_device{}()});
    }
}

void cls_channel::guide_times(
    std::string f1, std::string &st1,std::string &en1,
    std::string f2, std::string &st2,std::string &en2)
{
    AVFormatContext *guidefmt_ctx;
    int64_t         dur_time;
    int             retcd;
    time_t          timenow;
    struct tm       *time_info;
    char            timebuf[1024];

    dur_time = 0;

    time(&timenow);
    time_info = localtime(&timenow);
    strftime(timebuf, sizeof(timebuf), "%Y%m%d%H%M%S %z", time_info);
    st1 = timebuf;

    guidefmt_ctx = nullptr;
    retcd = avformat_open_input(&guidefmt_ctx,f1.c_str(), 0, 0);
    if (retcd < 0) {
        en1 = st1;
        LOG_MSG(NTC, NO_ERRNO, "Could not open file %s",f1.c_str());
        return;
    }
    dur_time = av_rescale(guidefmt_ctx->duration, 1 , AV_TIME_BASE);
    avformat_close_input(&guidefmt_ctx);
    guidefmt_ctx = nullptr;

    timenow = timenow + (int16_t)(dur_time);
    time_info = localtime(&timenow);
    strftime(timebuf, sizeof(timebuf), "%Y%m%d%H%M%S %z", time_info);
    en1 = timebuf;

    timenow++;
    time_info = localtime(&timenow);
    strftime(timebuf, sizeof(timebuf), "%Y%m%d%H%M%S %z", time_info);
    st2 = timebuf;

    retcd = avformat_open_input(&guidefmt_ctx, f2.c_str(), 0, 0);
    if (retcd < 0) {
        en2 = st2;
        LOG_MSG(NTC, NO_ERRNO, "Could not open file %s",f2.c_str());
        return;
    }
    dur_time = av_rescale(guidefmt_ctx->duration, 1 , AV_TIME_BASE);
    avformat_close_input(&guidefmt_ctx);
    guidefmt_ctx = nullptr;

    timenow = timenow + (int16_t)(dur_time);
    time_info = localtime(&timenow);
    strftime(timebuf, sizeof(timebuf), "%Y%m%d%H%M%S %z", time_info);
    en2 = timebuf;

}

void cls_channel::guide_write_xml(std::string &xml)
{
    std::string st1,en1,fl1,dn1;
    std::string st2,en2,fl2,dn2;
    std::string gnm;
    char    buf[4096];

    fl1 = playlist[playlist_index].fullnm;
    dn1 = playlist[playlist_index].displaynm;

    if ((playlist_index+1) >= playlist_count) {
        fl2 = playlist[0].fullnm;
        dn2 = playlist[0].displaynm;
    } else {
        fl2 = playlist[playlist_index+1].fullnm;
        dn2 = playlist[playlist_index+1].displaynm;
    }
    guide_times(fl1, st1, en1, fl2, st2, en2);

    gnm = "channel"+ch_nbr;

    snprintf(buf, 4096,
        "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n"
        "<!DOCTYPE tv SYSTEM \"xmlv.dtd\">\n"
        "<tv>\n"
        "  <channel id=\"%s\">\n"
        "    <display-name>%s</display-name>\n"
        "  </channel>\n"
        "  <programme start=\"%s \" stop=\"%s \" channel=\"%s\">\n"
        "    <title lang=\"en\">%s</title>\n"
        "  </programme>\n"
        "  <channel id=\"%s\">\n"
        "    <display-name>%s</display-name>\n"
        "  </channel>\n"
        "  <programme start=\"%s \" stop=\"%s \" channel=\"%s\">\n"
        "    <title lang=\"en\">%s</title>\n"
        "  </programme>\n"
        "</tv>\n"

        ,gnm.c_str(),gnm.c_str()
        ,st1.c_str(),en1.c_str(),gnm.c_str(),dn1.c_str()

        ,gnm.c_str(),gnm.c_str()
        ,st2.c_str(),en2.c_str(),gnm.c_str(),dn2.c_str()
    );

    xml = buf;
}

void cls_channel::guide_process()
{
    struct stat sfile;
    struct  sockaddr_un addr;
    ssize_t retcd;
    int fd, rc;
    std::string xml;

    if ((ch_finish == true) || (ch_tvhguide == false)) {
        return;
    }

    /* Determine if we are on the test machine */
    rc = stat(app->conf->epg_socket.c_str(), &sfile);
    if(rc < 0) {
        LOG_MSG(NTC, NO_ERRNO
            , "Requested tvh guide but the required directory does not exist:> %s <"
            , app->conf->epg_socket.c_str());
        LOG_MSG(NTC, NO_ERRNO
            , "Printing tvh guide xml to log.");
        guide_write_xml(xml);
        LOG_MSG(NTC, NO_ERRNO, "%s",xml.c_str());
    }

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        LOG_MSG(NTC, NO_ERRNO, "Error creating socket for the guide");
        return;
    }

    memset(&addr,'\0', sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path,108,"%s", app->conf->epg_socket.c_str());
    rc = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (rc == -1) {
        LOG_MSG(NTC, NO_ERRNO, "Error connecting socket for the guide: %s"
            , app->conf->epg_socket.c_str());
        close(fd);
        return;
    }

    guide_write_xml(xml);

    retcd = write(fd, xml.c_str(), xml.length());
    if (retcd != (ssize_t)xml.length()) {
        LOG_MSG(NTC, NO_ERRNO
            , "Error writing socket tried %d wrote %ld"
            , xml.length(), retcd);
    }
    close(fd);

}

void cls_channel::process()
{
    int indx;

    LOG_MSG(NTC, NO_ERRNO, "Starting ch%s",ch_nbr.c_str());

    while (ch_finish == false) {
        playlist_load();
        for (indx=0; indx < playlist_count; indx++) {
            playlist_index = indx;
            LOG_MSG(NTC, NO_ERRNO, "Ch%s: Playing: %s"
                , ch_nbr.c_str(), playlist[indx].filenm.c_str());
            guide_process();
            infile->start(playlist[indx].fullnm);
            infile->read();
            infile->stop();
            if (ch_finish == true) {
                break;
            }
        }
    }

    infile->stop();

    ch_running = false;
    LOG_MSG(NTC, NO_ERRNO, "Ch%s: Finished",ch_nbr.c_str());
}

cls_channel::cls_channel(int p_index, std::string p_conf)
{
    std::list<ctx_params_item>::iterator    it;

    ch_finish = false;
    ch_dir = "";
    ch_nbr = "";
    ch_sort = "";
    ch_running = true;
    ch_tvhguide = true;
    ch_encode = "";
    ch_index = p_index;
    ch_conf = p_conf;
    cnct_cnt = 0;
    file_cnt = 0;

    util_parms_parse(
        ch_params
        , "Ch"+std::to_string(ch_index)
        , ch_conf);

    for (it  = ch_params.params_array.begin();
         it != ch_params.params_array.end(); it++) {
        if (it->param_name == "dir") {
            ch_dir = it->param_value;
        }
        if (it->param_name == "ch") {
            ch_nbr = it->param_value;
        }
        if (it->param_name == "sort") {
            ch_sort = it->param_value;
        }
        if (it->param_name == "tvhguide") {
            app->conf->parm_set_bool(ch_tvhguide, it->param_value);
        }
        if (it->param_name == "enc") {
            ch_encode = it->param_value;
        }
    }

    infile = new cls_infile(this);
    pktarray = new cls_pktarray(this);

}

cls_channel::~cls_channel()
{

    delete pktarray;
    delete infile;

}