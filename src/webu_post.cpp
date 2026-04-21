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
#include "webu_post.hpp"

/**************Callback functions for MHD **********************/

mhdrslt webup_iterate_post (void *ptr, enum MHD_ValueKind kind
        , const char *key, const char *filename, const char *content_type
        , const char *transfer_encoding, const char *data, uint64_t off, size_t datasz)
{
    (void) kind;
    (void) filename;
    (void) content_type;
    (void) transfer_encoding;
    (void) off;
    cls_webup *webu_post;

    webu_post = (cls_webup *)ptr;
    return webu_post->iterate_post(key, data, datasz);
}

/**************Class methods**********************/

/* Get the command, device_id and camera index from the post data */
void cls_webup::parse_cmd()
{
    int indx, ch_num;

    post_cmd = "";
    ch_num = -1;
    c_webua->chitm = nullptr;

    for (indx = 0; indx < post_sz; indx++) {
        if (mystreq(post_info[indx].key_nm, "command")) {
            post_cmd = post_info[indx].key_val;
        }
        if (mystreq(post_info[indx].key_nm, "channel")) {
            ch_num = atoi(post_info[indx].key_val);
        }

       LOG_MSG(DBG, NO_ERRNO, "key: %s  value: %s "
            , post_info[indx].key_nm
            , post_info[indx].key_val
        );
    }

    if (post_cmd == "") {
        LOG_MSG(ERR, NO_ERRNO, "Invalid post request.  No command");
        return;
    }
    if (ch_num == -1) {
        LOG_MSG(ERR, NO_ERRNO
            , "Invalid post request.  No channel number provided");
        return;
    }

    for (indx=0; indx<c_app->ch_count; indx++) {
        if (mtoi(c_app->channels[indx]->ch_nbr) == ch_num) {
            c_webua->chitm = c_app->channels[indx];
            break;
        }
    }
    
    if (c_webua->chitm == nullptr) {
       LOG_MSG(ERR, NO_ERRNO
            , "Invalid channel. %d",ch_num);
    }

}

/* Process the actions from the webcontrol that the user requested */
void cls_webup::process_actions()
{
    int indx;
    parse_cmd();

    if ((post_cmd == "") || (c_webua->chitm == nullptr)) {
        return;
    }
    
    LOG_MSG(INF, NO_ERRNO, "Post cmd %s",post_cmd.c_str());

    if (post_cmd == "delay") {
        if (c_webua->chitm == nullptr) {
            LOG_MSG(INF, NO_ERRNO, "null chitem");
        } else {
            for (indx=0;indx<post_sz;indx++){
                if (mystreq(post_info[indx].key_nm,"sync")) {
                    c_webua->chitm->ch_sync = 
                        mtoi(post_info[indx].key_val);
                    LOG_MSG(INF, NO_ERRNO, "New sync adjustment %d"
                        ,c_webua->chitm->ch_sync);
                }
            }
        }
    } else {
       LOG_MSG(INF, NO_ERRNO
            , "Invalid action requested: command: >%s< "
            , post_cmd.c_str());
    }

}

/*Append more data on to an existing entry in the post info structure */
void cls_webup::iterate_post_append(int indx
        , const char *data, size_t datasz)
{
    post_info[indx].key_val = (char*)realloc(
        post_info[indx].key_val
        , post_info[indx].key_sz + datasz + 1);

    memset(post_info[indx].key_val +
        post_info[indx].key_sz, 0, datasz + 1);

    if (datasz > 0) {
        memcpy(post_info[indx].key_val +
            post_info[indx].key_sz, data, datasz);
    }

    post_info[indx].key_sz += datasz;
    LOG_MSG(DBG, NO_ERRNO
        , "Index: %d Key: >%s< val: >%s< "
        , indx, post_info[indx].key_nm, post_info[indx].key_val);
}

/*Create new entry in the post info structure */
void cls_webup::iterate_post_new(const char *key
        , const char *data, size_t datasz)
{
    int retcd;

    post_sz++;
    if (post_sz == 1) {
        post_info = (ctx_key *)malloc(sizeof(ctx_key));
    } else {
        post_info = (ctx_key *)realloc(post_info
            , (uint)post_sz * sizeof(ctx_key));
    }

    post_info[post_sz-1].key_nm = (char*)malloc(strlen(key)+1);
    retcd = snprintf(post_info[post_sz-1].key_nm, strlen(key)+1, "%s", key);

    post_info[post_sz-1].key_val = (char*)malloc(datasz+1);
    memset(post_info[post_sz-1].key_val,0,datasz+1);
    if (datasz > 0) {
        memcpy(post_info[post_sz-1].key_val, data, datasz);
    }

    post_info[post_sz-1].key_sz = datasz;

   LOG_MSG(DBG, NO_ERRNO
        , "Indx: %d Key: >%s< val: >%s< "
        , post_sz-1
        , post_info[post_sz-1].key_nm
        , post_info[post_sz-1].key_val);

    if (retcd < 0) {
       LOG_MSG(INF, NO_ERRNO, "Error processing post data");
    }
}

mhdrslt cls_webup::iterate_post (const char *key, const char *data, size_t datasz)
{
    int indx;

    for (indx=0; indx < post_sz; indx++) {
        if (mystreq(post_info[indx].key_nm, key)) {
            break;
        }
    }
    if (indx < post_sz) {
        iterate_post_append(indx, data, datasz);
    } else {
        iterate_post_new(key, data, datasz);
    }

    return MHD_YES;
}

mhdrslt cls_webup::processor_init()
{
    post_processor = MHD_create_post_processor (c_webua->connection
        , WEBUI_POST_BFRSZ, webup_iterate_post, (void *)this);
    if (post_processor == NULL) {
        return MHD_NO;
    }
    return MHD_YES;
}

mhdrslt cls_webup::processor_start(const char *upload_data, size_t *upload_data_size)
{
     mhdrslt    retcd;

    if (*upload_data_size != 0) {        
        retcd = MHD_post_process (post_processor, upload_data, *upload_data_size);
        *upload_data_size = 0;
    } else {
        pthread_mutex_lock(&c_app->mutex_post);
            process_actions();
        pthread_mutex_unlock(&c_app->mutex_post);
        c_webua->resp_page =
            "<!DOCTYPE html>\n"
            "<html>\n"
            "<body>\n"
            "<p>OK</p>\n"
            "</body>\n"
            "</html>\n";
        c_webua->mhd_send();
        retcd = MHD_YES;
    }
    return retcd;
}

cls_webup::cls_webup(cls_app *p_app, cls_webua *p_webua)
{
    c_app    = p_app;
    c_webu   = p_app->webu;
    c_webua  = p_webua;

    post_processor  = nullptr;
    post_info   = nullptr;
    post_sz     = 0;

}

cls_webup::~cls_webup()
{
    int indx;

    if (post_processor != nullptr) {
        MHD_destroy_post_processor (post_processor);
    }

    for (indx = 0; indx<post_sz; indx++) {
        myfree(post_info[indx].key_nm);
        myfree(post_info[indx].key_val);
    }
    myfree(post_info);
}