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

static int webu_mpegts_avio_buf(void *opaque, myuint *buf, int buf_size)
{
    cls_webuts *webuts =(cls_webuts *)opaque;

    return webuts->mpegts_avio_buf(buf, buf_size);
}

static ssize_t webu_mpegts_response(void *cls, uint64_t pos, char *buf, size_t max)
{
    (void)pos;
    cls_webuts *webuts =(cls_webuts *)cls;

    return webuts->mpegts_response(pos, buf, max);
}

/**************************************************/

int cls_webuts::mpegts_avio_buf(myuint *buf, int buf_size)
{
    if (resp_size < (size_t)(buf_size + resp_used)) {
        resp_size = (size_t)(buf_size + resp_used);
        resp_image = (unsigned char*)realloc(
            resp_image, resp_size);
    }

    memcpy(resp_image + resp_used, buf, buf_size);
    resp_used += buf_size;

    return buf_size;
}

ssize_t cls_webuts::mpegts_response(uint64_t pos, char *buf, size_t max)
{
    (void)pos;
    size_t sent_bytes;

    if (c_webu->wb_finish == true) {
        return -1;
    }

    if ((stream_pos == 0) && (resp_used == 0)) {
        getimg();
    }

    if (resp_used == 0) {
        resetpos();
        return 0;
    }

    if ((resp_used - stream_pos) > max) {
        sent_bytes = max;
    } else {
        sent_bytes = resp_used - stream_pos;
    }

    memcpy(buf, resp_image + stream_pos, sent_bytes);

    stream_pos = stream_pos + sent_bytes;
    if (stream_pos >= resp_used) {
        stream_pos = 0;
        resp_used = 0;
    }
    return sent_bytes;
}

void cls_webuts::free_context()
{
    if (wfile.audio.codec_ctx != nullptr) {
        myavcodec_close(wfile.audio.codec_ctx);
        wfile.audio.codec_ctx = nullptr;
    }
    if (wfile.video.codec_ctx != nullptr) {
        myavcodec_close(wfile.video.codec_ctx);
        wfile.video.codec_ctx = nullptr;
    }

    if (wfile.fmt_ctx != nullptr) {
        if (wfile.fmt_ctx->pb != nullptr) {
            if (wfile.fmt_ctx->pb->buffer != nullptr) {
                av_free(wfile.fmt_ctx->pb->buffer);
                wfile.fmt_ctx->pb->buffer = nullptr;
            }
            avio_context_free(&wfile.fmt_ctx->pb);
            wfile.fmt_ctx->pb = nullptr;
        }
        avformat_free_context(wfile.fmt_ctx);
        wfile.audio.strm = nullptr;
        wfile.video.strm = nullptr;
        wfile.fmt_ctx = nullptr;
    }

    LOG_MSG(DBG, NO_ERRNO, "Ch%s: Completed"
        , chitm->ch_nbr.c_str());
}

void cls_webuts::resetpos()
{
    stream_pos = 0;
    resp_used = 0;
}

void cls_webuts::packet_wait()
{
    int64_t tm_diff, pts_diff, tot_diff;
    int64_t sec_full, sec_msec;

    if (c_webu->wb_finish == true) {
        return;
    }

    if (pkt->pts == AV_NOPTS_VALUE) {
        return;
    }


    return;

    SLEEP(0, msec_cnt * 1000);
    return;

    /* How much time the pts wants us to be at since the start of the movie */
    if (pkt->stream_index == wfile.audio.index) {
        pts_diff = av_rescale_q(
            pkt->pts - wfile.audio.start_pts
            , AVRational{1,1}
            ,wfile.audio.strm->time_base);
    } else {
        pts_diff = av_rescale_q(
            pkt->pts - wfile.video.start_pts
            , AVRational{1,1}
            ,wfile.video.strm->time_base);
    }

    /* How much time has really elapsed since the start of movie*/
    tm_diff = av_gettime_relative() - wfile.time_start;

    /* How much time we need to wait to get in sync*/
    tot_diff = pts_diff - tm_diff;

    if (tot_diff > 0) {
        sec_full = int(tot_diff / 1000000L);
        sec_msec = (tot_diff % 1000000L);
        if (sec_full < 100){
            SLEEP(sec_full, sec_msec * 1000);
        } else {
            if (pkt->stream_index ==wfile.audio.index) {
                LOG_MSG(INF, NO_ERRNO, "sync index %d last %d pktpts %d base %d "
                    , pkt->stream_index
                    , wfile.audio.last_pts
                    , pkt->pts
                    , wfile.audio.base_pdts
                );
            } else {
                LOG_MSG(INF, NO_ERRNO, "sync index %d last %d pktpts %d base %d"
                    , pkt->stream_index
                    , wfile.video.last_pts
                    , pkt->pts
                    , wfile.video.base_pdts
                );
            }
        }
    }
}

void cls_webuts::packet_pts()
{
    int64_t ts_interval, base_pdts, last_pts;
    int64_t strm_st_pts, src_pts, temp_pts;
    AVRational tmpdst;

    src_pts = pkt->pts;
    if (wfile.time_start == -1) {
        wfile.time_start = av_gettime_relative();
        file_cnt = pkt_file_cnt;
    }
/*
        LOG_MSG(DBG, NO_ERRNO
            ,"init %d %d pts %d %d %d/%d st-a %d %d/%d st-v %d %d/%d"
            , pkt->stream_index
            , file_cnt
            , src_pts
            , chitm->pktarray[indx].start_pts
            , chitm->pktarray[indx].timebase.num
            , chitm->pktarray[indx].timebase.den
            , wfile.audio.start_pts
            , wfile.audio.strm->time_base.num
            , wfile.audio.strm->time_base.den
            , wfile.video.start_pts
            , wfile.video.strm->time_base.num
            , wfile.video.strm->time_base.den
            );
*/


    if (pkt->stream_index == wfile.audio.index) {
        if (wfile.audio.start_pts == -1 ) {
            wfile.audio.start_pts = av_rescale_q(
                src_pts - pkt_start_pts
                , pkt_timebase
                , wfile.audio.strm->time_base);
            if (wfile.audio.start_pts <= 0) {
                wfile.audio.start_pts = 1;
            }
            if (wfile.video.start_pts != -1 ) {
                temp_pts = av_rescale_q(
                    wfile.video.start_pts
                    , wfile.video.strm->time_base
                    , wfile.audio.strm->time_base);
                if (temp_pts < wfile.audio.start_pts) {
                    wfile.audio.start_pts = temp_pts;
                } else {
                    wfile.video.start_pts = av_rescale_q(
                        wfile.audio.start_pts
                        , wfile.audio.strm->time_base
                        , wfile.video.strm->time_base);
                }
            }
            wfile.audio.last_pts = 0;
            wfile.audio.base_pdts = 0;
        }
        tmpdst = wfile.audio.strm->time_base;
        last_pts = wfile.audio.last_pts;
        base_pdts = wfile.audio.base_pdts;
        strm_st_pts = wfile.audio.start_pts;
    } else {
        if (wfile.video.start_pts == -1 ) {
            wfile.video.start_pts = av_rescale_q(
                src_pts - pkt_start_pts
                , pkt_timebase
                , wfile.video.strm->time_base);
            if (wfile.video.start_pts <= 0) {
                wfile.video.start_pts = 1;
            }
            if (wfile.audio.start_pts != -1 ) {
                temp_pts = av_rescale_q(
                    wfile.audio.start_pts
                    , wfile.audio.strm->time_base
                    , wfile.video.strm->time_base);
                if (temp_pts < wfile.video.start_pts) {
                    wfile.video.start_pts = temp_pts;
                } else {
                    wfile.audio.start_pts = av_rescale_q(
                        wfile.video.start_pts
                        , wfile.video.strm->time_base
                        , wfile.audio.strm->time_base);
                }
            }
            wfile.video.last_pts = 0;
            wfile.video.base_pdts = 0;
        }
        tmpdst = wfile.video.strm->time_base;
        last_pts = wfile.video.last_pts;
        base_pdts = wfile.video.base_pdts;
        strm_st_pts = wfile.video.start_pts;
    }


    if (pkt->pts != AV_NOPTS_VALUE) {
        if (file_cnt == pkt_file_cnt) {
            pkt->pts = av_rescale_q(pkt->pts - pkt_start_pts
                ,pkt_timebase, tmpdst) - strm_st_pts + base_pdts;
        } else {
            file_cnt = pkt_file_cnt;
            base_pdts = last_pts + strm_st_pts;
            pkt->pts = av_rescale_q(pkt->pts - pkt_start_pts
                ,pkt_timebase, tmpdst) - strm_st_pts + base_pdts;
            if (pkt->pts == last_pts) {
                pkt->pts++;
            }
            if (pkt->stream_index == wfile.audio.index) {
                wfile.audio.base_pdts = base_pdts;
                wfile.video.base_pdts = av_rescale_q(base_pdts
                    , wfile.audio.strm->time_base
                    , wfile.video.strm->time_base);
            } else {
                wfile.video.base_pdts = base_pdts;
                wfile.audio.base_pdts = av_rescale_q(base_pdts
                    , wfile.video.strm->time_base
                    , wfile.audio.strm->time_base);
            }
        }
        if (pkt->pts <= 0) {
            if (pkt->pts < 0) {
                LOG_MSG(DBG, NO_ERRNO
                    ,"Skipping %d",pkt->pts);

                //abort();
                return;
            }
            pkt->pts = 1;
        }
    }

    if (pkt->dts != AV_NOPTS_VALUE) {
        if (file_cnt == pkt_file_cnt) {
            pkt->dts = av_rescale_q(pkt->dts - pkt_start_pts
                ,pkt_timebase, tmpdst) - strm_st_pts + base_pdts;
        } else {
            file_cnt = pkt_file_cnt;
            base_pdts = last_pts + strm_st_pts;
            pkt->dts = av_rescale_q(pkt->dts - pkt_start_pts
                , pkt_timebase, tmpdst) - strm_st_pts + base_pdts;
            if (pkt->dts == last_pts) {
                pkt->dts++;
            }
            if (pkt->stream_index == wfile.audio.index) {
                wfile.audio.base_pdts = base_pdts;
                wfile.video.base_pdts = av_rescale_q(base_pdts
                    , wfile.audio.strm->time_base
                    , wfile.video.strm->time_base);
            } else {
                wfile.video.base_pdts = base_pdts;
                wfile.audio.base_pdts = av_rescale_q(base_pdts
                    , wfile.video.strm->time_base
                    , wfile.audio.strm->time_base);
            }
        }
        if (pkt->dts <= 0) {
            pkt->dts = 1;
        }
    }

    ts_interval = pkt->duration;
    pkt->duration = av_rescale_q(ts_interval,pkt_timebase, tmpdst);
    bool chk;
    if (pkt->flags & AV_PKT_FLAG_KEY) {
        chk = true;
    } else {
        chk = false;
    }
    if (chk) {
        chk = false;
    }
/*
    LOG_MSG(DBG, NO_ERRNO
        ,"data %d %d src %d  %d/%d newpts %d %d tb-a %d/%d %d %d tb-v %d/%d %d %d"
        , pkt->stream_index
        , file_cnt
        , src_pts
        , pkt_timebase.num
        , pkt_timebase.den
        , pkt->pts
        , chk
        , wfile.audio.strm->time_base.num
        , wfile.audio.strm->time_base.den
        , wfile.audio.base_pdts
        , wfile.audio.start_pts
        , wfile.video.strm->time_base.num
        , wfile.video.strm->time_base.den
        , wfile.video.base_pdts
        , wfile.video.start_pts
        );
*/
}

/* Write the packet in the array at indx to the output format context */
void cls_webuts::packet_write()
{
    int retcd;
    char errstr[128];

    if (c_webu->wb_finish == true) {
        return;
    }

    if ((wfile.audio.index != chitm->infile->ofile.audio.index) ||
        (wfile.video.index != chitm->infile->ofile.video.index)) {
        if (pkt->stream_index == chitm->infile->ofile.audio.index) {
            LOG_MSG(DBG, NO_ERRNO,"Swapping audio");
            pkt->stream_index = wfile.audio.index;
        } else if (pkt->stream_index == chitm->infile->ofile.video.index) {
            LOG_MSG(DBG, NO_ERRNO,"Swapping video");
            pkt->stream_index = wfile.video.index;
        }
    }

    packet_pts();

    if (pkt->stream_index == wfile.audio.index) {
        if (pkt->pts > wfile.audio.last_pts) {
            wfile.audio.last_pts = pkt->pts;
        } else {
            return;
        }
    } else {
        if (pkt->pts > wfile.video.last_pts) {
            wfile.video.last_pts = pkt->pts;
        } else {
            return;
        }
    }

    packet_wait();
/*
    LOG_MSG(NTC, NO_ERRNO
        ,"%s: Writing frame index %d id %d"
        , chitm->ch_nbr.c_str()
        , pkt_index, pkt_idnbr);
*/
    if ((start_cnt == 1) &&
        (pkt_key == false) &&
        (pkt->stream_index = chitm->infile->ofile.video.index)) {
        return;
    }
    start_cnt = 0;

    retcd = av_interleaved_write_frame(wfile.fmt_ctx, pkt);
    if (retcd < 0) {
        av_strerror(retcd, errstr, sizeof(errstr));
        LOG_MSG(ERR, NO_ERRNO
            ,"Error writing frame index %d id %d err %s"
            , pkt_index, pkt_idnbr, errstr);
    }

}

void cls_webuts::pkt_copy(int indx)
{
    int retcd;
    char errstr[128];
    ctx_packet_item *pkt_src;

    pkt_src = &chitm->pktarray->array[indx];
    retcd = mycopy_packet(pkt, pkt_src->packet);
    if (retcd < 0) {
        av_strerror(retcd, errstr, sizeof(errstr));
        LOG_MSG(INF, NO_ERRNO, "av_copy_packet: %s",errstr);
        mypacket_free(pkt);
        pkt = nullptr;
        return;
    }
    pkt_index     = indx;
    pkt_idnbr     = pkt_src->idnbr;
    pkt_start_pts = pkt_src->start_pts;
    pkt_timebase  = pkt_src->timebase;
    pkt_file_cnt  = pkt_src->file_cnt;
    pkt_key       = pkt_src->iskey;
}

bool cls_webuts::pkt_get(int indx)
{
    bool pktready;
    pthread_mutex_lock(&chitm->pktarray->mtx);
        if ((chitm->pktarray->array[indx].packet != nullptr) &&
            (chitm->pktarray->array[indx].idnbr > pkt_idnbr) ) {
            pkt_copy(indx);
            if (pkt == nullptr) {
                pktready = false;
            } else {
                pktready = true;
            }
        } else {
            pktready = false;
        }
    pthread_mutex_unlock(&chitm->pktarray->mtx);
    return pktready;
}

void cls_webuts::getimg()
{
    int indx_next, chk;
    bool pktready;

    pthread_mutex_lock(&chitm->pktarray->mtx);
        if (chitm->pktarray->count == 0) {
            pthread_mutex_unlock(&chitm->pktarray->mtx);
            return;
        }
    pthread_mutex_unlock(&chitm->pktarray->mtx);

    indx_next = pkt_index;
    indx_next = chitm->pktarray->index_next(indx_next);

    pkt = mypacket_alloc(pkt);

    pktready = pkt_get( indx_next);

    chk = 0;
    while (
        (chk < 1000000) &&
        (pktready == false) &&
        (c_webu->wb_finish == false)) {

        SLEEP(0, msec_cnt * 1000);
        pktready = pkt_get(indx_next);
        chk++;
    }

    if (chk == 1000000) {
        LOG_MSG(INF, NO_ERRNO,"Excessive wait for new packet");
        msec_cnt++;
    } else {
        packet_write();
    }
    mypacket_free(pkt);
    pkt = nullptr;
}

int cls_webuts::streams_video_h264()
{
    int retcd;
    char errstr[128];
    AVCodecContext  *wfl_ctx, *enc_ctx;
    AVDictionary    *opts;
    const AVCodec   *encoder;
    AVStream        *stream;

    opts = NULL;
    wfile.fmt_ctx->video_codec_id = AV_CODEC_ID_H264;
    wfile.fmt_ctx->video_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    encoder = wfile.fmt_ctx->video_codec;

    wfile.video.strm = avformat_new_stream(
        wfile.fmt_ctx, encoder);
    if (wfile.video.strm == nullptr) {
        LOG_MSG(ERR, NO_ERRNO, "Could not alloc video stream");
        free_context();
        return -1;
    }
    wfile.video.index = wfile.video.strm->index;
    stream = wfile.video.strm;

    wfile.video.codec_ctx = avcodec_alloc_context3(encoder);
    if (wfile.video.codec_ctx == nullptr) {
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Could not allocate video context"
            , chitm->ch_nbr.c_str());
        free_context();
        return -1;
    }

    enc_ctx = chitm->infile->ofile.video.codec_ctx;
    wfl_ctx = wfile.video.codec_ctx;

    wfl_ctx->gop_size      = enc_ctx->gop_size;
    wfl_ctx->codec_id      = enc_ctx->codec_id;
    wfl_ctx->codec_type    = enc_ctx->codec_type;
    wfl_ctx->bit_rate      = enc_ctx->bit_rate;
    wfl_ctx->width         = enc_ctx->width;
    wfl_ctx->height        = enc_ctx->height;
    wfl_ctx->time_base     = enc_ctx->time_base;
    wfl_ctx->pix_fmt       = enc_ctx->pix_fmt;
    wfl_ctx->max_b_frames  = enc_ctx->max_b_frames;
    wfl_ctx->framerate     = enc_ctx->framerate;
    wfl_ctx->keyint_min    = enc_ctx->keyint_min;

    av_dict_set( &opts, "profile", "main", 0 );
    av_dict_set( &opts, "crf", "17", 0 );
    av_dict_set( &opts, "tune", "zerolatency", 0 );
    av_dict_set( &opts, "preset", "fast", 0 );
    av_dict_set( &opts, "keyint", "5", 0 );
    av_dict_set( &opts, "scenecut", "200", 0 );

    if (wfile.fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    retcd = avcodec_open2(wfl_ctx, encoder, &opts);
    if (retcd < 0) {
        av_strerror(retcd, errstr, sizeof(errstr));
        LOG_MSG(ERR, NO_ERRNO,"Failed to open codec: %s", errstr);
        av_dict_free(&opts);
        free_context();
        return -1;
    }
    av_dict_free(&opts);

    retcd = avcodec_parameters_from_context(stream->codecpar, wfl_ctx);
    if (retcd < 0) {
        av_strerror(retcd, errstr, sizeof(errstr));
        LOG_MSG(ERR, NO_ERRNO
            ,"Failed to copy decoder parameters!: %s", errstr);
        free_context();
        return -1;
    }
    stream->time_base = chitm->infile->ofile.video.strm->time_base;

    return 0;
}

int cls_webuts::streams_video_mpeg()
{
    int retcd;
    char errstr[128];
    AVCodecContext  *wfl_ctx, *enc_ctx;
    AVDictionary    *opts;
    const AVCodec   *encoder;
    AVStream        *stream;

    opts = NULL;
    wfile.fmt_ctx->video_codec_id = AV_CODEC_ID_MPEG2VIDEO;
    wfile.fmt_ctx->video_codec = avcodec_find_encoder(AV_CODEC_ID_MPEG2VIDEO);

    encoder = wfile.fmt_ctx->video_codec;

    wfile.video.strm = avformat_new_stream(wfile.fmt_ctx, encoder);
    if (wfile.video.strm == nullptr) {
        LOG_MSG(ERR, NO_ERRNO, "Could not alloc video stream");
        free_context();
        return -1;
    }
    wfile.video.index = wfile.video.strm->index;
    stream = wfile.video.strm;

    wfile.video.codec_ctx = avcodec_alloc_context3(encoder);
    if (wfile.video.codec_ctx == nullptr) {
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Could not allocate video context"
            , chitm->ch_nbr.c_str());
        free_context();
        return -1;
    }

    enc_ctx = chitm->infile->ofile.video.codec_ctx;
    wfl_ctx = wfile.video.codec_ctx;

    wfl_ctx->codec_id      = enc_ctx->codec_id;
    wfl_ctx->codec_type    = enc_ctx->codec_type;
    wfl_ctx->width         = enc_ctx->width;
    wfl_ctx->height        = enc_ctx->height;
    wfl_ctx->time_base     = enc_ctx->time_base;
    wfl_ctx->max_b_frames  = enc_ctx->max_b_frames;
    wfl_ctx->framerate     = enc_ctx->framerate;
    wfl_ctx->bit_rate      = enc_ctx->bit_rate;
    wfl_ctx->gop_size      = enc_ctx->gop_size;
    wfl_ctx->pix_fmt       = enc_ctx->pix_fmt;
    wfl_ctx->sw_pix_fmt    = enc_ctx->sw_pix_fmt;
    if (wfile.fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        wfl_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    retcd = avcodec_open2(wfl_ctx, encoder, &opts);
    if (retcd < 0) {
        av_strerror(retcd, errstr, sizeof(errstr));
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Could not open video encoder: %s %dx%d %d %d %d/%d"
            , chitm->ch_nbr.c_str(), errstr
            , wfl_ctx->width, wfl_ctx->height
            , wfl_ctx->pix_fmt, wfl_ctx->time_base.den
            , wfl_ctx->framerate.num, wfl_ctx->framerate.den);
        av_dict_free(&opts);
        free_context();
        abort();
        return -1;
    }
    av_dict_free(&opts);

    retcd = avcodec_parameters_from_context(stream->codecpar, wfl_ctx);
    if (retcd < 0) {
        av_strerror(retcd, errstr, sizeof(errstr));
        LOG_MSG(ERR, NO_ERRNO
            ,"Failed to copy decoder parameters!: %s", errstr);
        free_context();
        return -1;
    }
    stream->time_base = chitm->infile->ofile.video.strm->time_base;
    stream->r_frame_rate = enc_ctx->framerate;
    stream->avg_frame_rate= enc_ctx->framerate;

/*
    LOG_MSG(NTC, NO_ERRNO
        , "%s: Opened video encoder: %dx%d %d %d %d/%d %d/%d %d/%d"
        , chitm->ch_nbr.c_str()
        , wfl_ctx->width, wfl_ctx->height
        , wfl_ctx->pix_fmt, wfl_ctx->time_base.den
        , wfl_ctx->framerate.num, wfl_ctx->framerate.den
        , wfile.video.strm->avg_frame_rate.num,wfile.video.strm->avg_frame_rate.den
        , wfile.video.strm->r_frame_rate.num, wfile.video.strm->r_frame_rate.den
        );
*/
    return 0;
}

int cls_webuts::streams_audio()
{
    int retcd;
    AVCodecContext  *wfl_ctx, *enc_ctx;
    char errstr[128];
    AVDictionary *opts = NULL;
    const AVCodec *encoder;
    AVStream        *stream;

    wfile.audio.codec_ctx = nullptr;
    encoder = avcodec_find_encoder(AV_CODEC_ID_AC3);

    wfile.audio.strm = avformat_new_stream(wfile.fmt_ctx, encoder);
    if (wfile.audio.strm == nullptr) {
        LOG_MSG(ERR, NO_ERRNO, "Could not alloc audio stream");
        free_context();
        return -1;
    }
    stream = wfile.audio.strm;
    wfile.audio.index = wfile.audio.strm->index;

    wfile.audio.codec_ctx = avcodec_alloc_context3(encoder);
    if (wfile.audio.codec_ctx == nullptr) {
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Could not allocate audio context"
            , chitm->ch_nbr.c_str());
        free_context();
        return -1;
    }

    enc_ctx =chitm->infile->ofile.audio.codec_ctx;
    wfl_ctx =wfile.audio.codec_ctx;

    wfile.fmt_ctx->audio_codec_id = AV_CODEC_ID_AC3;
    wfile.fmt_ctx->audio_codec = avcodec_find_encoder(AV_CODEC_ID_AC3);
    stream->codecpar->codec_id=AV_CODEC_ID_AC3;
    stream->codecpar->bit_rate = enc_ctx->bit_rate;;
    stream->codecpar->frame_size = enc_ctx->frame_size;;
    av_channel_layout_default(&stream->codecpar->ch_layout
        , enc_ctx->ch_layout.nb_channels);

    stream->codecpar->format = enc_ctx->sample_fmt;
    stream->codecpar->sample_rate = enc_ctx->sample_rate;
    stream->time_base.den  = enc_ctx->sample_rate;
    stream->time_base.num = 1;

    wfl_ctx->bit_rate = enc_ctx->bit_rate;
    wfl_ctx->sample_fmt = enc_ctx->sample_fmt;
    wfl_ctx->sample_rate = enc_ctx->sample_rate;
    wfl_ctx->time_base = enc_ctx->time_base;
    av_channel_layout_default(&wfl_ctx->ch_layout
        , enc_ctx->ch_layout.nb_channels);
    wfl_ctx->frame_size = enc_ctx->frame_size;
    wfl_ctx->pkt_timebase = enc_ctx->pkt_timebase;

    retcd = avcodec_open2(wfl_ctx, encoder, &opts);
    if (retcd < 0) {
        av_strerror(retcd, errstr, sizeof(errstr));
        LOG_MSG(ERR, NO_ERRNO,"Failed to open codec: %s", errstr);
        free_context();
        abort();
        return -1;
    }
    av_dict_free(&opts);

    return 0;
}

int cls_webuts::open()
{
    int retcd, indx_curr;
    char errstr[128];
    unsigned char   *buf_image;
    AVDictionary    *opts;

    if (c_webu->wb_finish == true) {
        return -1;
    }

    opts = NULL;
    wfile.fmt_ctx = avformat_alloc_context();
    wfile.fmt_ctx->oformat = av_guess_format("mpegts", NULL, NULL);

    pthread_mutex_lock(&chitm->infile->mtx);
        if (chitm->infile->ofile.video.index != -1) {
            if (chitm->ch_encode == "h264") {
                retcd = streams_video_h264();
            } else {
                retcd = streams_video_mpeg();
            }
            if (retcd < 0) {
                pthread_mutex_unlock(&chitm->infile->mtx);
                free_context();
                return -1;
            }
        }
        if (chitm->infile->ofile.audio.index != -1) {
            retcd = streams_audio();
            if (retcd < 0) {
                pthread_mutex_unlock(&chitm->infile->mtx);
                free_context();
                return -1;
            }
        }
    pthread_mutex_unlock(&chitm->infile->mtx);

    resp_image  =(unsigned char*) mymalloc(WEBUA_LEN_RESP * 10);
    memset(resp_image,'\0',WEBUA_LEN_RESP);
    resp_size = WEBUA_LEN_RESP;
    resp_used = 0;

    aviobuf_sz = WEBUA_LEN_RESP;
    buf_image = (unsigned char*)av_malloc(aviobuf_sz);
    wfile.fmt_ctx->pb = avio_alloc_context(
        buf_image, (int)aviobuf_sz, 1, this
        , NULL, &webu_mpegts_avio_buf, NULL);
    wfile.fmt_ctx->flags = AVFMT_FLAG_CUSTOM_IO;
    av_dict_set(&opts, "movflags", "empty_moov", 0);

    retcd = avformat_write_header(wfile.fmt_ctx, &opts);
    if (retcd < 0) {
        av_strerror(retcd, errstr, sizeof(errstr));
        LOG_MSG(ERR, NO_ERRNO
            ,"Failed to write header!: %s", errstr);
        free_context();
        av_dict_free(&opts);
        return -1;
    }
    av_dict_free(&opts);

    stream_pos = 0;
    resp_used = 0;

    indx_curr = chitm->pktarray->index_curr();
    if (indx_curr < 0) {
        indx_curr = 0;
    }

    pkt_index = (int)(chitm->pktarray->count / 2) + indx_curr;
    if (pkt_index > chitm->pktarray->count) {
        pkt_index -= chitm->pktarray->count;
    }
    pkt_idnbr = 1;
    start_cnt = 1;

    LOG_MSG(NTC, NO_ERRNO
        , "%s: Setting start %d %d"
        , chitm->ch_nbr.c_str()
        , pkt_index, indx_curr);

    return 0;
}

mhdrslt cls_webuts::main()
{
    mhdrslt retcd;
    struct MHD_Response *response;
    std::list<ctx_params_item>::iterator    it;

    if (c_webu->wb_finish == true) {
        return MHD_NO;
    }

    if (open() == -1) {
        return MHD_NO;
    }

    clock_gettime(CLOCK_MONOTONIC, &time_last);

    response = MHD_create_response_from_callback (MHD_SIZE_UNKNOWN
        , 32768, &webu_mpegts_response, this, NULL);
    if (response == nullptr) {
        LOG_MSG(ERR, NO_ERRNO, "Invalid response");
        return MHD_NO;
    }

    for (it  = c_webu->headers.params_array.begin();
         it != c_webu->headers.params_array.end(); it++) {
        MHD_add_response_header (response
            , it->param_name.c_str(), it->param_value.c_str());
    }

    MHD_add_response_header(response, "Cache-Control", "no-store,no-cache,must-revalidate,max-age=0");
    MHD_add_response_header(response, "Content-Transfer-Encoding", "BINARY");
    MHD_add_response_header(response, "Content-Type", "video/mp2t");

    retcd = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response (response);

    return retcd;
}

cls_webuts::cls_webuts(cls_app *p_app, cls_webua *p_webua)
{
    c_app = p_app;
    c_conf = p_app->conf;
    c_webu = p_app->webu;
    c_webua = p_webua;
    chitm = c_webua->chitm;

    connection = c_webua->connection;
    resp_image    = nullptr;                     /* Buffer for sending the images */
    resp_size     = WEBUA_LEN_RESP * 10;         /* The size of the resp_page buffer.  May get adjusted */
    resp_used     = 0;                           /* How many bytes used so far in resp_page*/
    aviobuf_sz    = 0;

    wfile.audio.index = -1;
    wfile.audio.last_pts = -1;
    wfile.audio.start_pts = -1;
    wfile.audio.codec_ctx = nullptr;
    wfile.audio.strm = nullptr;
    wfile.audio.base_pdts = 0;
    wfile.video = wfile.audio;
    wfile.fmt_ctx = nullptr;
    wfile.time_start = -1;

    file_cnt = 0;
    start_cnt = 50;
    stream_pos    = 0;                           /* Stream position of image being sent */
    stream_fps    = 300;                         /* Stream rate */
    time_last.tv_nsec = 0;
    time_last.tv_sec = 0;

    msec_cnt = 50;

    pkt = nullptr;
    pkt_index = 0;
    pkt_idnbr =1;
    pkt_start_pts=0;
    pkt_timebase.num = 1;
    pkt_timebase.den = 1000;
    pkt_file_cnt = 0;
    pkt_key = false;

}

cls_webuts::~cls_webuts()
{
    myfree(&resp_image);
    free_context();
}
