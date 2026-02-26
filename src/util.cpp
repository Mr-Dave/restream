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


/** Non case sensitive equality check for strings*/
int mystrceq(const char* var1, const char* var2)
{
    if ((var1 == NULL) || (var2 == NULL)) {
        return 0;
    }
    return (strcasecmp(var1,var2) ? 0 : 1);
}

/** Non case sensitive inequality check for strings*/
int mystrcne(const char* var1, const char* var2)
{
    if ((var1 == NULL) || (var2 == NULL)) {
        return 0;
    }
    return (strcasecmp(var1,var2) ? 1: 0);
}

/** Case sensitive equality check for strings*/
int mystreq(const char* var1, const char* var2)
{
    if ((var1 == NULL) || (var2 == NULL)) {
        return 0;
    }
    return (strcmp(var1,var2) ? 0 : 1);
}

/** Case sensitive inequality check for strings*/
int mystrne(const char* var1, const char* var2)
{
    if ((var1 == NULL) ||(var2 == NULL)) {
        return 0;
    }
    return (strcmp(var1,var2) ? 1: 0);
}

/* Trim whitespace from left side */
void myltrim(std::string &parm)
{
    if (parm.length() == 0 ) {
        return;
    }

    while (std::isspace(parm.at(0))) {
        if (parm.length() == 1) {
            parm="";
            return;
        } else {
            parm = parm.substr(1);
        }
    }
}

/* Trim whitespace from right side */
void myrtrim(std::string &parm)
{
    if (parm.length() == 0 ) {
        return;
    }

    while (std::isspace(parm.at(parm.length()-1))) {
        if (parm.length() == 1) {
            parm="";
            return;
        } else {
            parm = parm.substr(0,parm.length()-1);
        }
    }
}

/* Trim left and right whitespace */
void mytrim(std::string &parm)
{
    myrtrim(parm);
    myltrim(parm);
}

/* Remove surrounding quotes */
void myunquote(std::string &parm)
{
    size_t plen;

    mytrim(parm);

    plen = parm.length();
    while ((plen >= 2) &&
        (((parm.substr(0,1)== "\"") && (parm.substr(plen,1)== "\"")) ||
         ((parm.substr(0,1)== "'") && (parm.substr(plen,1)== "'")))) {

        parm = parm.substr(1, plen-2);
        plen = parm.length();
    }

}

void myfree(void *ptr_addr)
{
    void **ptr = (void **)ptr_addr;

    if (*ptr != NULL) {
        free(*ptr);
        *ptr = NULL;
    }
}

void *mymalloc(size_t nbytes)
{
    void *dummy = calloc(nbytes, 1);

    if (!dummy) {
        LOG_MSG(EMG, SHOW_ERRNO, "Could not allocate %llu bytes of memory!"
            ,(unsigned long long)nbytes);
        exit(1);
    }

    return dummy;
}

void *myrealloc(void *ptr, size_t size, const char *desc)
{
    void *dummy = NULL;

    if (size == 0) {
        free(ptr);
        LOG_MSG(WRN, NO_ERRNO
            ,"Warning! Function %s tries to resize 0 bytes!",desc);
    } else {
        dummy = realloc(ptr, size);
        if (!dummy) {
            LOG_MSG(EMG, NO_ERRNO
                ,"Could not resize memory-block at offset %p to %llu bytes (function %s)!"
                ,ptr, (unsigned long long)size, desc);
            exit(1);
        }
    }

    return dummy;
}

int mycreate_path(const char *path)
{
    std::string tmp;
    mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    size_t indx_pos;
    int retcd;
    struct stat statbuf;

    tmp = std::string(path);

    if (tmp.substr(0, 1) == "/") {
        indx_pos = tmp.find("/", 1);
    } else {
        indx_pos = tmp.find("/", 0);
    }

    while (indx_pos != std::string::npos) {
        if (stat(tmp.substr(0, indx_pos + 1).c_str(), &statbuf) != 0) {
            LOG_MSG(NTC, NO_ERRNO
                ,"Creating %s", tmp.substr(0, indx_pos + 1).c_str());
            retcd = mkdir(tmp.substr(0, indx_pos + 1).c_str(), mode);
            if (retcd == -1 && errno != EEXIST) {
                LOG_MSG(ERR, SHOW_ERRNO
                    ,"Problem creating directory %s"
                    , tmp.substr(0, indx_pos + 1).c_str());
                return -1;
            }
        }
        indx_pos++;
        if (indx_pos >= tmp.length()) {
            break;
        }
        indx_pos = tmp.find("/", indx_pos);
    }

    return 0;
}

FILE *myfopen(const char *path, const char *mode)
{
    FILE *fp;

    fp = fopen(path, mode);
    if (fp) {
        return fp;
    }

    /* If path did not exist, create and try again*/
    if (errno == ENOENT) {
        if (mycreate_path(path) == -1) {
            return NULL;
        }
        fp = fopen(path, mode);
    }
    if (!fp) {
        LOG_MSG(ERR, SHOW_ERRNO
            ,"Error opening file %s with mode %s", path, mode);
        return NULL;
    }

    return fp;
}

int myfclose(FILE* fh)
{
    int rval = fclose(fh);

    if (rval != 0) {
        LOG_MSG(ERR, SHOW_ERRNO, "Error closing file");
    }

    return rval;
}

void mythreadname_set(const char *abbr, int threadnbr, const char *threadname)
{
    char tname[32];
    if (abbr != NULL) {
        snprintf(tname, sizeof(tname), "%s%02d%s%s",abbr,threadnbr,
             threadname ? ":" : "",
             threadname ? threadname : "");
    } else {
        snprintf(tname, sizeof(tname), "%s",threadname);
    }

    #ifdef __APPLE__
        pthread_setname_np(tname);
    #elif defined(BSD)
        pthread_set_name_np(pthread_self(), tname);
    #elif HAVE_PTHREAD_SETNAME_NP
        pthread_setname_np(pthread_self(), tname);
    #else
        if (app != nullptr) {
            LOG_MSG(INF, NO_ERRNO, "Unable to set thread name %s", tname);
        }
    #endif

}

void mythreadname_get(char *threadname)
{
    #if ((!defined(BSD) && HAVE_PTHREAD_GETNAME_NP) || defined(__APPLE__))
        char currname[16];
        pthread_getname_np(pthread_self(), currname, sizeof(currname));
        snprintf(threadname, sizeof(currname), "%s",currname);
    #else
        snprintf(threadname, 8, "%s","Unknown");
    #endif
}

/****************************************************************************
 *  These are designed to be extremely simple version specific
 *  variants of the libav functions.
 ****************************************************************************/

/*********************************************/
AVFrame *myframe_alloc(void)
{
    AVFrame *pic;
    #if (MYFFVER >= 55000)
        pic = av_frame_alloc();
    #else
        pic = avcodec_alloc_frame();
    #endif
    return pic;
}
/*********************************************/
void myframe_free(AVFrame *frame)
{
    #if (MYFFVER >= 55000)
        av_frame_free(&frame);
    #else
        av_freep(&frame);
    #endif
}
/*********************************************/
int myimage_get_buffer_size(enum MyPixelFormat pix_fmt, int width, int height)
{
    int retcd = 0;
    #if (MYFFVER >= 57000)
        int align = 1;
        retcd = av_image_get_buffer_size(pix_fmt, width, height, align);
    #else
        retcd = avpicture_get_size(pix_fmt, width, height);
    #endif
    return retcd;
}
/*********************************************/
int myimage_copy_to_buffer(AVFrame *frame, uint8_t *buffer_ptr, enum MyPixelFormat pix_fmt
        , int width, int height, int dest_size)
{
    int retcd = 0;
    #if (MYFFVER >= 57000)
        int align = 1;
        retcd = av_image_copy_to_buffer((uint8_t *)buffer_ptr,dest_size
            ,(const uint8_t * const*)frame,frame->linesize,pix_fmt,width,height,align);
    #else
        retcd = avpicture_layout((const AVPicture*)frame,pix_fmt,width,height
            ,(unsigned char *)buffer_ptr,dest_size);
    #endif
    return retcd;
}
/*********************************************/
int myimage_fill_arrays(AVFrame *frame,uint8_t *buffer_ptr,enum MyPixelFormat pix_fmt
        , int width,int height)
{
    int retcd = 0;
    #if (MYFFVER >= 57000)
        int align = 1;
        retcd = av_image_fill_arrays(
            frame->data
            ,frame->linesize
            ,buffer_ptr
            ,pix_fmt
            ,width
            ,height
            ,align
        );
    #else
        retcd = avpicture_fill(
            (AVPicture *)frame
            ,buffer_ptr
            ,pix_fmt
            ,width
            ,height);
    #endif
    return retcd;
}
/*********************************************/
void mypacket_free(AVPacket *pkt)
{
    #if (MYFFVER >= 57041)
        av_packet_free(&pkt);
    #else
        av_free_packet(pkt);
    #endif

}
/*********************************************/
void myavcodec_close(AVCodecContext *codec_context)
{
    #if (MYFFVER >= 57041)
        avcodec_free_context(&codec_context);
    #else
        avcodec_close(codec_context);
    #endif
}
/*********************************************/
int mycopy_packet(AVPacket *dest_pkt, AVPacket *src_pkt)
{
    #if (MYFFVER >= 55000)
        return av_packet_ref(dest_pkt, src_pkt);
    #else
        /* Old versions of libav do not support copying packet
        * We therefore disable the pass through recording and
        * for this function, simply do not do anything
        */
        if (dest_pkt == src_pkt ) {
            return 0;
        } else {
            return 0;
        }
    #endif
}
/*********************************************/
AVPacket *mypacket_alloc(AVPacket *pkt)
{
    if (pkt != NULL) {
        mypacket_free(pkt);
    };
    pkt = av_packet_alloc();
    #if (MYFFVER < 58076)
        av_init_packet(pkt);
        pkt->data = NULL;
        pkt->size = 0;
    #endif

    return pkt;

}

/*********************************************/

static void util_parms_file(ctx_params &params, std::string params_file)
{
    int chk;
    size_t stpos;
    std::list<ctx_params_item>::iterator  it;
    std::string line, parm_nm, parm_vl;
    std::ifstream ifs;

    LOG_MSG(ERR, NO_ERRNO, "parse file: %s", params_file.c_str());

    chk = 0;
    for (it  = params.params_array.begin();
         it != params.params_array.end(); it++) {
        if (it->param_name == "params_file" ) {
            chk++;
        }
    }
    if (chk > 1){
        LOG_MSG(ERR, NO_ERRNO
            ,"Only one params_file specification is permitted.");
        return;
    }

    ifs.open(params_file.c_str());
        if (ifs.is_open() == false) {
            LOG_MSG(ERR, NO_ERRNO
                ,"params_file not found: %s", params_file.c_str());
            return;
        }
        while (std::getline(ifs, line)) {
            mytrim(line);
            stpos = line.find(" ");
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
                mytrim(parm_nm);
                mytrim(parm_vl);
                util_parms_add(params, parm_nm.c_str(), parm_vl.c_str());
            } else if ((line != "") &&
                (line.substr(0, 1) != ";") &&
                (line.substr(0, 1) != "#")) {
                LOG_MSG(ERR, NO_ERRNO
                ,"Unable to parse line: %s", line.c_str());
            }
        }
    ifs.close();

}

void util_parms_add(ctx_params &params, std::string parm_nm, std::string parm_val)
{
    std::list<ctx_params_item>::iterator  it;
    ctx_params_item parm_itm;

    for (it  = params.params_array.begin();
         it != params.params_array.end(); it++) {
        if (it->param_name == parm_nm) {
            it->param_value.assign(parm_val);
            return;
        }
    }

    /* This is a new parameter*/
    params.params_count++;
    parm_itm.param_name.assign(parm_nm);
    parm_itm.param_value.assign(parm_val);
    params.params_array.push_back(parm_itm);

    LOG_MSG(INF, NO_ERRNO,"%s Parsed: >%s< >%s<"
        ,params.params_desc.c_str(), parm_nm.c_str(),parm_val.c_str());

    if ((parm_nm == "params_file") && (parm_val != "")) {
        util_parms_file(params, parm_val);
    }
}

static void util_parms_strip_qte(std::string &parm)
{
    if (parm.length() == 0) {
        return;
    }

    if (parm.substr(0, 1)=="\"") {
        if (parm.length() == 1) {
            parm = "";
            return;
        } else {
            parm = parm.substr(1);
        }
    }

    if (parm.substr(parm.length() -1, 1)== "\"") {
        if (parm.length() == 1) {
            parm = "";
            return;
        } else {
            parm = parm.substr(0, parm.length() - 1);
        }
    }

}

static void util_parms_extract(ctx_params &params, std::string &parmline
        ,size_t indxnm_st,size_t indxnm_en,size_t indxvl_st,size_t indxvl_en)
{
    std::string parm_nm, parm_vl;

    if ((indxnm_st != std::string::npos) &&
        (indxnm_en != std::string::npos) &&
        (indxvl_st != std::string::npos) &&
        (indxvl_en != std::string::npos)) {

        parm_nm = parmline.substr(indxnm_st, indxnm_en - indxnm_st + 1);
        parm_vl = parmline.substr(indxvl_st, indxvl_en - indxvl_st + 1);

        mytrim(parm_nm);
        mytrim(parm_vl);

        util_parms_strip_qte(parm_nm);
        util_parms_strip_qte(parm_vl);

        util_parms_add(params, parm_nm.c_str(), parm_vl.c_str());
    }
}

/* Cut out the parameter that was just extracted from the parmline string */
static void util_parms_next(std::string &parmline, size_t indxnm_st, size_t indxvl_en)
{
    size_t indxcm;

    indxcm = parmline.find(",", indxvl_en);

    if (indxnm_st == 0) {
        if (indxcm == std::string::npos) {
            parmline = "";
        } else {
            parmline = parmline.substr(indxcm + 1);
        }
    } else {
        if (indxcm == std::string::npos) {
            parmline = parmline.substr(0, indxnm_st - 1);
        } else {
            parmline = parmline.substr(0, indxnm_st - 1) + parmline.substr(indxcm + 1);
        }
    }
    mytrim(parmline);

}

void util_parms_parse_qte(ctx_params &params, std::string &parmline)
{
    size_t indxnm_st, indxnm_en;
    size_t indxvl_st, indxvl_en;
    size_t indxcm, indxeq;

    /* Parse out all the items within quotes first */
    while (parmline.find("\"", 0) != std::string::npos) {

        indxnm_st = parmline.find("\"", 0);

        indxnm_en = parmline.find("\"", indxnm_st + 1);
        if (indxnm_en == std::string::npos) {
            indxnm_en = parmline.length() - 1;
        }

        indxcm = parmline.find(",", indxnm_en + 1);
        if (indxcm == std::string::npos) {
            indxcm = parmline.length() - 1;
        }

        indxeq = parmline.find("=", indxnm_en + 1);
        if (indxeq == std::string::npos) {
            indxeq = parmline.length() - 1;
        }

        if (indxcm <= indxeq) {
            /* The quoted part of the parm was the value not the name */
            indxvl_st = indxnm_st;
            indxvl_en = indxnm_en;

            indxnm_st = parmline.find_last_of(",", indxvl_st);
            if (indxnm_st == std::string::npos) {
                indxnm_st = 0;
            } else {
                indxnm_st++; /* Move past the comma */
            }

            indxnm_en = parmline.find("=", indxnm_st);
            if ((indxnm_en == std::string::npos) ||
                (indxnm_en > indxvl_st)) {
                indxnm_en = indxvl_st + 1;
            }
            indxnm_en--; /* do not include the = */

        } else {
            /* The quoted part of the parm was the name */
            indxvl_st = parmline.find("\"",indxeq + 1);
            indxcm = parmline.find(",", indxeq + 1);
            if (indxcm == std::string::npos) {
                if (indxvl_st == std::string::npos) {
                    indxvl_st = indxeq + 1;
                    if (indxvl_st >= parmline.length()) {
                        indxvl_st = parmline.length() - 1;
                    }
                    indxvl_en = parmline.length() - 1;
                } else {
                    /* The value is also enclosed in quotes */
                    indxvl_en=parmline.find("\"", indxvl_st + 1);
                    if (indxvl_en == std::string::npos) {
                        indxvl_en = parmline.length() - 1;
                    }
                }
            } else if (indxvl_st == std::string::npos) {
                /* There are no more quotes in the line */
                indxvl_st = indxeq + 1;
                indxvl_en = parmline.find(",",indxvl_st) - 1;
            } else if (indxcm < indxvl_st) {
                /* The quotes belong to next item */
                indxvl_st = indxeq + 1;
                indxvl_en = parmline.find(",",indxvl_st) - 1;
            } else {
                /* The value is also enclosed in quotes */
                indxvl_en=parmline.find("\"", indxvl_st + 1);
                if (indxvl_en == std::string::npos) {
                    indxvl_en = parmline.length() - 1;
                }
            }
        }

        //LOG_MSG(DBG, NO_ERRNO,"Parsing: >%s< >%ld %ld %ld %ld<"
        //    ,parmline.c_str(), indxnm_st, indxnm_en, indxvl_st, indxvl_en);

        util_parms_extract(params, parmline, indxnm_st, indxnm_en, indxvl_st, indxvl_en);
        util_parms_next(parmline, indxnm_st, indxvl_en);
    }
}

void util_parms_parse_comma(ctx_params &params, std::string &parmline)
{
    size_t indxnm_st, indxnm_en;
    size_t indxvl_st, indxvl_en;

    while (parmline.find(",", 0) != std::string::npos) {
        indxnm_st = 0;
        indxnm_en = parmline.find("=", 1);
        if (indxnm_en == std::string::npos) {
            indxnm_en = 0;
            indxvl_st = 0;
        } else {
            indxvl_st = indxnm_en + 1;  /* Position past = */
            indxnm_en--;                /* Position before = */
        }

        if (parmline.find(",", indxvl_st) == std::string::npos) {
            indxvl_en = parmline.length() - 1;
        } else {
            indxvl_en = parmline.find(",",indxvl_st) - 1;
        }

        util_parms_extract(params, parmline, indxnm_st, indxnm_en, indxvl_st, indxvl_en);
        util_parms_next(parmline, indxnm_st, indxvl_en);
    }

    /* Take care of last parameter */
    if (parmline != "") {
        indxnm_st = 0;
        indxnm_en = parmline.find("=", 1);
        if (indxnm_en == std::string::npos) {
            /* If no value then we are done */
            return;
        } else {
            indxvl_st = indxnm_en + 1;  /* Position past = */
            indxnm_en--;                /* Position before = */
        }
        indxvl_en = parmline.length() - 1;

        //LOG_MSG(DBG, NO_ERRNO,"Parsing: >%s< >%ld %ld %ld %ld<"
        //    ,parmline.c_str(), indxnm_st, indxnm_en, indxvl_st, indxvl_en);

        util_parms_extract(params, parmline, indxnm_st, indxnm_en, indxvl_st, indxvl_en);
        util_parms_next(parmline, indxnm_st, indxvl_en);
    }

}

/* Parse through the config line and put into the array */
void util_parms_parse(ctx_params &params, std::string parm_desc, std::string confline)
{
    std::string parmline;

    /* We make a copy because the parsing destroys the value passed */
    parmline = confline;

    params.params_array.clear();
    params.params_count = 0;
    params.params_desc = parm_desc;

    util_parms_parse_qte(params, parmline);

    util_parms_parse_comma(params, parmline);

    params.update_params = false;

    return;

}

/* Add the requested int value as a default if the parm_nm does have anything yet */
void util_parms_add_default(ctx_params &params, std::string parm_nm, int parm_vl)
{
    bool dflt;
    std::list<ctx_params_item>::iterator  it;

    dflt = true;
    for (it  = params.params_array.begin();
         it != params.params_array.end(); it++) {
        if (it->param_name == parm_nm) {
            dflt = false;
        }
    }
    if (dflt == true) {
        util_parms_add(params, parm_nm, std::to_string(parm_vl));
    }
}

/* Add the requested string value as a default if the parm_nm does have anything yet */
void util_parms_add_default(ctx_params &params, std::string parm_nm, std::string parm_vl)
{
    bool dflt;
    std::list<ctx_params_item>::iterator  it;

    dflt = true;
    for (it  = params.params_array.begin();
         it != params.params_array.end(); it++) {
        if (it->param_name == parm_nm) {
            dflt = false;
        }
    }
    if (dflt == true) {
        util_parms_add(params, parm_nm, parm_vl);
    }
}

/* Update config line with the values from the params array */
void util_parms_update(ctx_params &params, std::string &confline)
{
    std::string parmline;
    std::string comma;
    std::list<ctx_params_item>::iterator  it;

    comma = "";
    parmline = "";
    for (it  = params.params_array.begin();
         it != params.params_array.end(); it++) {
        parmline += comma;
        comma = ",";
        if (it->param_name.find(" ") == std::string::npos) {
            parmline += it->param_name;
        } else {
            parmline += "\"";
            parmline += it->param_name;
            parmline += "\"";
        }

        parmline += "=";
        if (it->param_value.find(" ") == std::string::npos) {
            parmline += it->param_value;
        } else {
            parmline += "\"";
            parmline += it->param_value;
            parmline += "\"";
        }
    }
    parmline += " ";

    confline = parmline;

    LOG_MSG(INF, NO_ERRNO,"New config: %s", confline.c_str());

    return;
}

