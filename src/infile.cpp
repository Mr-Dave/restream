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


void cls_infile::defaults()
{
    ifile.audio.index = -1;
    ifile.audio.last_pts = -1;
    ifile.audio.start_pts = -1;
    ifile.audio.codec_ctx = nullptr;
    ifile.audio.strm = nullptr;
    ifile.audio.base_pdts = 0;
    ifile.audio.sync_pts = 0;    
    ifile.video = ifile.audio;
    ifile.fmt_ctx = nullptr;
    ifile.time_start = -1;
    ofile = ifile;
}

int cls_infile::decoder_init_video()
{
    int retcd;
    AVStream *stream;
    const AVCodec *dec;
    AVCodecContext *codec_ctx;

    ifile.video.codec_ctx = nullptr;
    stream = ifile.fmt_ctx->streams[ifile.video.index];
    ifile.video.strm = stream;
    dec = avcodec_find_decoder(stream->codecpar->codec_id);

    if (dec == nullptr) {
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Failed to find video decoder for input stream"
            , ch_nbr.c_str());
        return -1;
    }

    codec_ctx = avcodec_alloc_context3(dec);
    if (codec_ctx == nullptr) {
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Failed to allocate video decoder for input stream"
            , ch_nbr.c_str());
        return -1;
    }

    retcd = avcodec_parameters_to_context(codec_ctx, stream->codecpar);
    if (retcd < 0) {
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Failed to copy decoder parameters for stream"
            , ch_nbr.c_str());
        return -1;
    }

    codec_ctx->framerate = av_guess_frame_rate(ifile.fmt_ctx, stream, NULL);
    codec_ctx->pkt_timebase = stream->time_base;
    codec_ctx->gop_size = 2;
    codec_ctx->keyint_min = 5;
    av_opt_set(codec_ctx->priv_data, "profile", "main", 0);
    av_opt_set(codec_ctx->priv_data, "crf", "22", 0);
    av_opt_set(codec_ctx->priv_data, "tune", "zerolatency", 0);
    av_opt_set(codec_ctx->priv_data, "preset", "superfast",0);
    av_opt_set(codec_ctx->priv_data, "keyint", "5",0);

    retcd = avcodec_open2(codec_ctx, dec, NULL);
    if (retcd < 0) {
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Failed to open video codec for input stream "
            , ch_nbr.c_str());
        return -1;
    }

    ifile.video.codec_ctx = codec_ctx;
    return 0;
}

int cls_infile::decoder_init_audio()
{
    int retcd;
    AVStream *stream;
    const AVCodec *dec;
    AVCodecContext *codec_ctx;

    ifile.audio.codec_ctx = nullptr;
    stream = ifile.fmt_ctx->streams[ifile.audio.index];
    ifile.audio.strm = stream;
    dec = avcodec_find_decoder(stream->codecpar->codec_id);

    if (dec == nullptr) {
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Failed to find decoder for stream"
            , ch_nbr.c_str());
        return -1;
    }

    codec_ctx = avcodec_alloc_context3(dec);
    if (codec_ctx == nullptr) {
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Failed to allocate decoder for stream "
            , ch_nbr.c_str());
        return -1;
    }

    retcd = avcodec_parameters_to_context(codec_ctx, stream->codecpar);
    if (retcd < 0) {
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Failed to copy decoder parameters for stream "
            , ch_nbr.c_str());
        return -1;
    }
    codec_ctx->pkt_timebase = stream->time_base;

    retcd = avcodec_open2(codec_ctx, dec, NULL);
    if (retcd < 0) {
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Failed to open decoder for stream "
            , ch_nbr.c_str());
        return -1;
    }

    ifile.audio.codec_ctx = codec_ctx;

    return 0;
}

int cls_infile::decoder_init(std::string fnm)
{
    int retcd, indx;
    char errstr[128];
    AVMediaType strm_typ;
    AVDictionaryEntry *lang;

    LOG_MSG(NTC, NO_ERRNO
        , "Ch%s: Opening file '%s'"
        , ch_nbr.c_str(), fnm.c_str());

    retcd = avformat_open_input(&ifile.fmt_ctx
        , fnm.c_str(), NULL, NULL);
    if (retcd < 0) {
        av_strerror(retcd, errstr, sizeof(errstr));
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Could not open input file '%s': %s"
            , ch_nbr.c_str(), fnm.c_str(), errstr);
        return -1;
    }

    retcd = avformat_find_stream_info(ifile.fmt_ctx, NULL);
    if (retcd < 0) {
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Failed to retrieve input stream information"
            , ch_nbr.c_str());
        return -1;
    }

    for (indx = 0; indx < (int)ifile.fmt_ctx->nb_streams; indx++) {
        strm_typ = ifile.fmt_ctx->streams[indx]->codecpar->codec_type;
        if ((strm_typ == AVMEDIA_TYPE_VIDEO) &&
            (ifile.video.index == -1)) {
            ifile.video.index = indx;
            if (decoder_init_video() != 0) {
                return -1;
            }
        } else if ((strm_typ == AVMEDIA_TYPE_AUDIO) &&
            (ifile.audio.index == -1)) {
            lang = av_dict_get(
                ifile.fmt_ctx->streams[indx]->metadata
                , "language", NULL,0);
            if (lang != nullptr) {
                if (mystrne(lang->value, app->conf->language_code.c_str())) {
                    return 0;
                }
            }
            ifile.audio.index = indx;
            if (decoder_init_audio() != 0) {
                return -1;
            }
        }
    }
    return 0;
}

int cls_infile::decoder_get_ts()
{
    int retcd, indx;
    int64_t temp_pts;

    /* Read some pkts to get correct start times */
    indx = 0;
    ifile.video.start_pts = -1;
    ifile.audio.start_pts = -1;
    while (indx < 100) {
        av_packet_free(&pkt_in);
        pkt_in = av_packet_alloc();
        pkt_in->data = nullptr;
        pkt_in->size = 0;

        retcd = av_read_frame(ifile.fmt_ctx, pkt_in);
        if (retcd < 0) {
            LOG_MSG(NTC, NO_ERRNO
                , "Ch%s: Failed to read first packets for stream %d"
                , ch_nbr.c_str(), indx);
            return -1;
        }

        if (pkt_in->pts != AV_NOPTS_VALUE) {
            if (pkt_in->stream_index == ifile.video.index) {
                if (ifile.video.start_pts == -1) {
                    ifile.video.start_pts = pkt_in->pts;
                }
            }
            if (pkt_in->stream_index == ifile.audio.index) {
                if (ifile.audio.start_pts == -1) {
                    ifile.audio.start_pts = pkt_in->pts;
                }
            }
        }

        if ((ifile.video.start_pts >= 0) &&
            (ifile.audio.start_pts >= 0)) {
            break;
        }

        indx++;
    }

    ifile.time_start = av_gettime_relative();
    if ((ifile.video.strm != nullptr) &&
        (ifile.audio.strm != nullptr)) {
        temp_pts = av_rescale_q(
            ifile.audio.start_pts
            , ifile.audio.strm->time_base
            , ifile.video.strm->time_base);
        if (temp_pts > ifile.video.start_pts ) {
            ifile.video.start_pts = temp_pts;
        } else {
            ifile.audio.start_pts = av_rescale_q(
            ifile.video.start_pts
            , ifile.video.strm->time_base
            , ifile.audio.strm->time_base);
        }
    }

    ifile.audio.last_pts = ifile.audio.start_pts;
    ifile.video.last_pts = ifile.video.start_pts;

    chitm->file_cnt++;
    return 0;
}

void cls_infile::infile_wait()
{
    int64_t tm_diff, pts_diff, tot_diff;
    int64_t sec_full, sec_msec;

    if (chitm->ch_finish == true) {
        return;
    }

    if (chitm->pktarray->start > 0)  {
        chitm->pktarray->start--;
/*
        LOG_MSG(NTC, NO_ERRNO
            , "%s: skipping first packets %d id %d index %d"
            , ch_nbr.c_str()
            , chitm->pktarray_start
            , chitm->pktarray[chitm->pktarray_index].idnbr
            , chitm->pktarray_index);
*/
        /* Slightly adjust time start to consider these skipped packets*/
        if ((pkt_in->stream_index == ifile.video.index) &&
            (pkt_in->pts != AV_NOPTS_VALUE)) {
            tm_diff = av_gettime_relative() - ifile.time_start;
            pts_diff = av_rescale_q(
                pkt_in->pts - ifile.video.start_pts
                , AVRational{1,1}
                , ifile.video.strm->time_base);
            tot_diff = pts_diff - tm_diff;
            ifile.time_start -= tot_diff;
        }
        return;
    }


    if ((pkt_in->stream_index != ifile.video.index) ||
        (pkt_in->pts == AV_NOPTS_VALUE)  ||
        (pkt_in->pts < ifile.video.last_pts)) {
        return;
    }

    ifile.video.last_pts = pkt_in->pts;

    /* How much time has really elapsed since the start of movie*/
    tm_diff = av_gettime_relative() - ifile.time_start;

    /* How much time the pts wants us to be at since the start of the movie */
    /* At this point the pkt pts is in ifile timebase.*/
    pts_diff = av_rescale_q(
        pkt_in->pts - ifile.video.start_pts
        , AVRational{1,1}
        , ifile.video.strm->time_base);

    /* How much time we need to wait to get in sync*/
    tot_diff = pts_diff - tm_diff;

    if (tot_diff > 0) {
        sec_full = int(tot_diff / 1000000L);
        sec_msec = (tot_diff % 1000000L);
        if (sec_full < 100){
            SLEEP(sec_full, sec_msec * 1000);
        }
    }
}

void cls_infile::decoder_send()
{
    int retcd;
    char errstr[128];

    if (pkt_in->stream_index == ifile.video.index) {
        retcd = avcodec_send_packet(ifile.video.codec_ctx, pkt_in);
    } else {
        retcd = avcodec_send_packet(ifile.audio.codec_ctx, pkt_in);
    }
    if (retcd == AVERROR_INVALIDDATA) {
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Send ignoring packet stream %d with invalid data"
            , ch_nbr.c_str(), pkt_in->stream_index);
            return;
    } else if (retcd < 0 && retcd != AVERROR_EOF) {
        av_strerror(retcd, errstr, sizeof(errstr));
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Error sending packet to codec: %s"
            , ch_nbr.c_str(), errstr);
        return;
    }
}

void cls_infile::decoder_receive()
{
    int retcd;
    char errstr[128];

    if (frame != nullptr) {
        myframe_free(frame);
        frame = nullptr;
    }
    frame = myframe_alloc();

    if (pkt_in->stream_index == ifile.video.index) {
       retcd = avcodec_receive_frame(ifile.video.codec_ctx, frame);
    } else {
       retcd = avcodec_receive_frame(ifile.audio.codec_ctx, frame);
    }
    if (retcd < 0) {
        if (retcd == AVERROR_INVALIDDATA) {
            LOG_MSG(NTC, NO_ERRNO
                , "Ch%s: Ignoring packet with invalid data"
                , ch_nbr.c_str());
        } else if (retcd != AVERROR(EAGAIN)) {
            av_strerror(retcd, errstr, sizeof(errstr));
            LOG_MSG(NTC, NO_ERRNO
                , "Ch%s: Error receiving frame from decoder: %s"
                , ch_nbr.c_str(), errstr);
        }
        if (frame != nullptr) {
            myframe_free(frame);
            frame = nullptr;
        }
    }

}

int cls_infile::encoder_buffer_audio()
{
    int frame_size, retcd, bufspc;
    int frmsz_src, frmsz_dst;
    int64_t  pts, dts, ptsadj, dtsadj;
    char errstr[128];

    frmsz_src = ifile.audio.codec_ctx->frame_size;
    frmsz_dst = ofile.audio.codec_ctx->frame_size;

    retcd = av_audio_fifo_realloc(fifo, frmsz_src+frmsz_dst);
    if (retcd < 0) {
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Could not allocate FIFO"
            , ch_nbr.c_str());
        return -1;
    }

    retcd = av_audio_fifo_write(
        fifo, (void **)frame->data,frmsz_src);
    if (retcd < frmsz_src) {
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Could not write data to FIFO"
            , ch_nbr.c_str());
        return -1;
    }
    pts = frame->pts;
    dts = frame->pkt_dts;

    myframe_free(frame);
    frame = nullptr;

    retcd = av_audio_fifo_size(fifo);
    if (retcd < frmsz_dst) {
        audio_last_pts = pts;
        audio_last_dts = dts;
        return -1;
    }

    frame_size = FFMIN(av_audio_fifo_size(fifo), frmsz_dst);

    bufspc = av_audio_fifo_space(fifo) - frmsz_dst;
    ptsadj =((float)((pts - audio_last_pts) / frmsz_src) * bufspc);
    dtsadj =((float)((dts - audio_last_dts) / frmsz_src) * bufspc);

    audio_last_pts = pts;
    audio_last_dts = dts;

    frame = av_frame_alloc();
    if (frame == nullptr) {
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Could not allocate output frame"
            , ch_nbr.c_str());
        return -1;
    }
    frame->nb_samples     = frame_size;
    av_channel_layout_copy(&frame->ch_layout
        , &ofile.audio.codec_ctx->ch_layout);
    frame->format         = ofile.audio.codec_ctx->sample_fmt;
    frame->sample_rate    = ofile.audio.codec_ctx->sample_rate;
    frame->pts = pts - ptsadj;
    frame->pkt_dts = dts-dtsadj;

    retcd = av_frame_get_buffer(frame, 0);
    if (retcd < 0) {
        av_strerror(retcd, errstr, sizeof(errstr));
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Could not allocate output frame buffer %s"
            , ch_nbr.c_str(), errstr);
        av_frame_free(&frame);
        frame = nullptr;
        return -1;
    }

    retcd = av_audio_fifo_read(fifo
        , (void **)frame->data, frame_size);
    if (retcd < frame_size) {
        fprintf(stderr, "Could not read data from FIFO\n");
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Could not read data from fifo"
            , ch_nbr.c_str());
        av_frame_free(&frame);
        frame = nullptr;
        return -1;
    }

    return 0;
}

void cls_infile::encoder_send()
{
    int retcd;
    char errstr[128];

    if (frame == nullptr) {
        return;
    }
    retcd = 0;
    if (pkt_in->stream_index == ifile.video.index) {
        if  (frame->pts != AV_NOPTS_VALUE) {
            frame->pts += chitm->ch_sync;
            if (frame->pts <= ofile.video.last_pts) {
                LOG_MSG(NTC, NO_ERRNO
                    , "Ch%s: PTS Problem %d %d"
                    , ch_nbr.c_str()
                    ,frame->pts
                    ,ofile.video.last_pts);
                myframe_free(frame);
                frame = nullptr;
                return;
            }
            ofile.video.last_pts = frame->pts;
        }
        frame->quality = ofile.video.codec_ctx->global_quality;
        retcd = avcodec_send_frame(ofile.video.codec_ctx, frame);
    } else if (pkt_in->stream_index == ifile.audio.index) {
        if (ifile.audio.codec_ctx->codec_id == AV_CODEC_ID_AAC) {
            retcd = encoder_buffer_audio();
            if (retcd < 0) {
                return;
            }
        }
        retcd = avcodec_send_frame(ofile.audio.codec_ctx, frame);
    }
    if (retcd < 0 ) {
        av_strerror(retcd, errstr, sizeof(errstr));
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Error sending %d frame for encoding: %s"
            , ch_nbr.c_str(), pkt_in->stream_index, errstr);
        int indx;
        for (indx=0;indx<chitm->pktarray->count;indx++) {
            if (chitm->pktarray->array[indx].packet != nullptr) {
                if (chitm->pktarray->array[indx].packet->stream_index == 0) {
                    LOG_MSG(NTC, NO_ERRNO
                        , "Ch%s: index %d stream %d frame %d pts %d arrayindex %d"
                        , ch_nbr.c_str(), indx
                        , chitm->pktarray->array[indx].packet->stream_index
                        , frame->pts
                        , chitm->pktarray->array[indx].packet->pts
                        , indx);
                }
            }
        }
        abort();
    }

    myframe_free(frame);
    frame = nullptr;

}

void cls_infile::encoder_receive()
{
    int retcd;
    char errstr[128];
    AVPacket *pkt;

    retcd = 0;
    while (retcd == 0) {
        pkt = NULL;
        pkt = mypacket_alloc(pkt);
        if (pkt_in->stream_index == ifile.video.index) {
            retcd = avcodec_receive_packet(ofile.video.codec_ctx, pkt);
            pkt->stream_index =ofile.video.index;
        } else {
            retcd = avcodec_receive_packet(ofile.audio.codec_ctx, pkt);
            pkt->stream_index =ofile.audio.index;
        }
        if (retcd == AVERROR(EAGAIN)) {
            mypacket_free(pkt);
            pkt = NULL;
            return;
        } else if (retcd < 0 ) {
            av_strerror(retcd, errstr, sizeof(errstr));
            LOG_MSG(ERR, NO_ERRNO
                ,"Error receiving encoded packet video:%s", errstr);
            //Packet is freed upon failure of encoding
            return;
        } else {
            /* Some files start with negative pts values. */
            //LOG_MSG(NTC, NO_ERRNO, "%s: adding pkt sz %d"
            //    , ch_nbr.c_str(), pkt->size);
            if (pkt->pts > 0) {
                chitm->pktarray->add(pkt);
            }
            mypacket_free(pkt);
            pkt = NULL;
        }
    }
}

void cls_infile::read()
{
    int retcd;

    if (is_started == false) {
        return;
    }

    pthread_mutex_unlock(&mtx);

    while (chitm->ch_finish == false) {
        av_packet_free(&pkt_in);
        pkt_in = av_packet_alloc();

        pkt_in->data = NULL;
        pkt_in->size = 0;

        retcd = av_read_frame(ifile.fmt_ctx, pkt_in);
        if (retcd < 0) {
            break;
        }

        infile_wait();

        if (chitm->cnct_cnt > 0) {
            decoder_send();
            decoder_receive();
            encoder_send();
            encoder_receive();
        }
        if (ifile.fmt_ctx == NULL) {
            break;
        }
    }
    av_packet_free(&pkt_in);
    pthread_mutex_lock(&mtx);
}

int cls_infile::encoder_init_video_h264()
{
    const AVCodec *encoder;
    AVStream *stream;
    AVCodecContext *enc_ctx,*dec_ctx;
    AVDictionary *opts = NULL;
    char errstr[128];
    int retcd;

    ofile.video.codec_ctx = nullptr;
    stream = avformat_new_stream(ofile.fmt_ctx, NULL);
    ofile.video.strm = stream;
    if (stream == nullptr) {
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Failed allocating output video stream"
            , ch_nbr.c_str());
        return -1;
    }
    ofile.video.index = stream->index;

    encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (encoder == nullptr) {
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Could not find video encoder"
            , ch_nbr.c_str());
        return -1;
    }

    ofile.video.codec_ctx = avcodec_alloc_context3(encoder);
    if (ofile.video.codec_ctx == nullptr) {
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Could not allocate video encoder"
            , ch_nbr.c_str());
        return -1;
    }

    enc_ctx = ofile.video.codec_ctx;
    dec_ctx = ifile.video.codec_ctx;

    retcd = avcodec_parameters_from_context(stream->codecpar, dec_ctx);
    if (retcd < 0) {
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Could not copy parms from video decoder"
            , ch_nbr.c_str());
        return -1;
    }

    retcd = avcodec_parameters_to_context(enc_ctx, stream->codecpar);
    if (retcd < 0) {
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Could not copy parms to video encoder"
            , ch_nbr.c_str());
        return -1;
    }

    enc_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    enc_ctx->width = dec_ctx->width;
    enc_ctx->height = dec_ctx->height;
    enc_ctx->time_base.num = 1;
    enc_ctx->time_base.den = 90000;
    enc_ctx->max_b_frames = 4;
    enc_ctx->framerate = dec_ctx->framerate;
    enc_ctx->bit_rate = 400000;
    enc_ctx->gop_size = 4;
    if (dec_ctx->pix_fmt == -1) {
        enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    } else {
        enc_ctx->pix_fmt = dec_ctx->pix_fmt;
    }

    if (ofile.fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    av_dict_set( &opts, "profile", "baseline", 0 );
    av_dict_set( &opts, "crf", "17", 0 );
    av_dict_set( &opts, "tune", "zerolatency", 0 );
    av_dict_set( &opts, "preset", "superfast", 0 );
    av_dict_set( &opts, "keyint", "4", 0 );
    av_dict_set( &opts, "scenecut", "0", 0 );

    retcd = avcodec_open2(enc_ctx, encoder, &opts);
    if (retcd < 0) {
        av_strerror(retcd, errstr, sizeof(errstr));
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Could not open video encoder: %s %dx%d %d %d"
            , ch_nbr.c_str(), errstr
            , enc_ctx->width, enc_ctx->height
            , enc_ctx->pix_fmt, enc_ctx->time_base.den);
        return -1;
    }
    av_dict_free(&opts);

    retcd = avcodec_parameters_from_context(stream->codecpar, enc_ctx);
    if (retcd < 0) {
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Could not copy parms from decoder"
            , ch_nbr.c_str());
        return -1;
    }
    stream->time_base.num = 1;
    stream->time_base.den = 90000;
    return 0;
}

int cls_infile::encoder_init_video_mpeg()
{
    const AVCodec *encoder;
    AVStream *stream;
    AVCodecContext *enc_ctx, *dec_ctx;
    AVDictionary *opts = nullptr;
    char errstr[128];
    int retcd;

    ofile.video.codec_ctx = nullptr;

    stream = avformat_new_stream(ofile.fmt_ctx, NULL);
    ofile.video.strm = stream;
    if (stream == nullptr) {
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Failed allocating output video stream"
            , ch_nbr.c_str());
        return -1;
    }
    ofile.video.index = stream->index;

    encoder = avcodec_find_encoder(AV_CODEC_ID_MPEG2VIDEO);
    if (encoder == nullptr) {
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Could not find video encoder"
            , ch_nbr.c_str());
        return -1;
    }

    ofile.fmt_ctx->video_codec_id = AV_CODEC_ID_MPEG2VIDEO;

    ofile.video.codec_ctx = avcodec_alloc_context3(encoder);
    if (ofile.video.codec_ctx == nullptr) {
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Could not allocate video encoder"
            , ch_nbr.c_str());
        return -1;
    }

    enc_ctx = ofile.video.codec_ctx;
    dec_ctx = ifile.video.codec_ctx;

    retcd = avcodec_parameters_from_context(stream->codecpar, dec_ctx);
    if (retcd < 0) {
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Could not copy parms from video decoder"
            , ch_nbr.c_str());
        return -1;
    }

    enc_ctx->codec_id = AV_CODEC_ID_MPEG2VIDEO;
    enc_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    enc_ctx->width = dec_ctx->width;
    enc_ctx->height = dec_ctx->height;
    enc_ctx->max_b_frames = 4;
    enc_ctx->gop_size = 4;
    enc_ctx->framerate = dec_ctx->framerate;
    enc_ctx->framerate.num = 30;
    enc_ctx->framerate.den = 1;
    enc_ctx->time_base.num = enc_ctx->framerate.den;
    enc_ctx->time_base.den = enc_ctx->framerate.num;
    enc_ctx->bit_rate = 6000000;
    enc_ctx->sw_pix_fmt = AV_PIX_FMT_YUV420P;
    enc_ctx->global_quality = 150;
    enc_ctx->flags |= AV_CODEC_FLAG_QSCALE;
    //enc_ctx->global_quality = FF_QP2LAMBDA * qscale;

    if (dec_ctx->pix_fmt == -1) {
        enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    } else {
        enc_ctx->pix_fmt = dec_ctx->pix_fmt;
    }
    if (ofile.fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    retcd = avcodec_open2(enc_ctx, encoder, &opts);
    if (retcd < 0) {
        av_strerror(retcd, errstr, sizeof(errstr));
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Could not open video encoder: %s %dx%d %d %d %d/%d"
            , ch_nbr.c_str(),errstr
            , enc_ctx->width, enc_ctx->height
            , enc_ctx->pix_fmt, enc_ctx->time_base.den
            ,enc_ctx->framerate.num,enc_ctx->framerate.den);
            abort();
        return -1;
    }
    av_dict_free(&opts);

    retcd = avcodec_parameters_from_context(stream->codecpar, enc_ctx);
    if (retcd < 0) {
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Could not copy parms from decoder"
            , ch_nbr.c_str());
        return -1;
    }
    stream->time_base.num = 1;
    stream->time_base.den = 90000;
    return 0;

}

int cls_infile::encoder_init_audio()
{
    AVStream *stream;
    AVCodecContext *enc_ctx,*dec_ctx;
    int retcd;
    char errstr[128];
    AVDictionary *opts = NULL;
    const AVCodec *encoder;

    ofile.audio.codec_ctx = nullptr;

    encoder = avcodec_find_encoder(AV_CODEC_ID_AC3);

    stream = avformat_new_stream(ofile.fmt_ctx, encoder);
    ofile.audio.strm = stream;
    if (stream == nullptr) {
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Failed allocating output stream"
            , ch_nbr.c_str());
        return -1;
    }
    ofile.audio.index = stream->index;

    ofile.audio.codec_ctx = avcodec_alloc_context3(encoder);
    if (ofile.audio.codec_ctx == nullptr) {
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Could not allocate audio encoder"
            , ch_nbr.c_str());
        return -1;
    }

    enc_ctx = ofile.audio.codec_ctx;
    dec_ctx = ifile.audio.codec_ctx;

    ofile.fmt_ctx->audio_codec_id = AV_CODEC_ID_AC3;
    ofile.fmt_ctx->audio_codec = avcodec_find_encoder(AV_CODEC_ID_AC3);
    stream->codecpar->codec_id=AV_CODEC_ID_AC3;

    stream->codecpar->bit_rate = dec_ctx->bit_rate;
    stream->codecpar->frame_size = dec_ctx->frame_size;
    stream->codecpar->format = dec_ctx->sample_fmt;
    stream->codecpar->sample_rate = dec_ctx->sample_rate;
    stream->time_base.den  = dec_ctx->sample_rate;
    stream->time_base.num = 1;
    av_channel_layout_default(&stream->codecpar->ch_layout
        , dec_ctx->ch_layout.nb_channels);
    av_channel_layout_copy(&stream->codecpar->ch_layout
        , &dec_ctx->ch_layout);

    if (dec_ctx->bit_rate == 0) {
        enc_ctx->bit_rate = dec_ctx->sample_rate * 10;
    } else {
        enc_ctx->bit_rate = dec_ctx->bit_rate;
    }
    enc_ctx->frame_size = dec_ctx->frame_size;
    enc_ctx->sample_fmt = dec_ctx->sample_fmt;
    enc_ctx->sample_rate = dec_ctx->sample_rate;
    enc_ctx->time_base.num = 1;
    enc_ctx->time_base.den  = dec_ctx->sample_rate;
    av_channel_layout_default(&enc_ctx->ch_layout
        , dec_ctx->ch_layout.nb_channels);
    av_channel_layout_copy(&enc_ctx->ch_layout
        , &dec_ctx->ch_layout);

    retcd = avcodec_open2(enc_ctx, encoder, &opts);
    if (retcd < 0) {
        av_strerror(retcd, errstr, sizeof(errstr));
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: Could not open audio encoder %s"
            , ch_nbr.c_str(), errstr);
        return -1;
    }
    av_dict_free(&opts);


    if (dec_ctx->codec_id == AV_CODEC_ID_AAC ) {
        /* Create the FIFO buffer based on the specified output sample format. */
        fifo = av_audio_fifo_alloc(
            ofile.audio.codec_ctx->sample_fmt
            , ofile.audio.codec_ctx->ch_layout.nb_channels, 1);
        if (fifo == nullptr) {
            LOG_MSG(NTC, NO_ERRNO
                , "Ch%s: Could not allocate FIFO"
                , ch_nbr.c_str());
            return -1;
        }
    }

    return 0;
}

int cls_infile::encoder_init()
{
    if (ifile.fmt_ctx == NULL) {
        LOG_MSG(NTC, NO_ERRNO
            , "Ch%s: no input file provided"
            , ch_nbr.c_str());
        return -1;
    }

    ofile.fmt_ctx = avformat_alloc_context();
    ofile.fmt_ctx->oformat = av_guess_format("mpegts", NULL, NULL);

    if (chitm->pktarray->count == 0) {
        chitm->pktarray->resize();
    }

    if (ifile.video.index != -1) {
        if (chitm->ch_encode == "h264") {
            if (encoder_init_video_h264() != 0) {
                return -1;
            }
        } else {
            if (encoder_init_video_mpeg() != 0) {
                return -1;
            }
        }
    }
    if (ifile.audio.index != -1) {
        if (encoder_init_audio() != 0) {
            return -1;
        }
    }
    return 0;

}

void cls_infile::stop()
{
    LOG_MSG(NTC, NO_ERRNO, "Ch%s: Closing"
        , ch_nbr.c_str());

    if (ifile.audio.codec_ctx !=  nullptr) {
        avcodec_free_context(&ifile.audio.codec_ctx);
        ifile.audio.codec_ctx =  nullptr;
    }
    if (ifile.video.codec_ctx !=  nullptr) {
        avcodec_free_context(&ifile.video.codec_ctx);
        ifile.video.codec_ctx =  nullptr;
    }
    if (ifile.fmt_ctx != nullptr) {
        avformat_close_input(&ifile.fmt_ctx);
        ifile.fmt_ctx = nullptr;
    }

    if (ofile.audio.codec_ctx !=  nullptr) {
        avcodec_free_context(&ofile.audio.codec_ctx);
        ofile.audio.codec_ctx =  nullptr;
    }
    if (ofile.video.codec_ctx !=  nullptr) {
        avcodec_free_context(&ofile.video.codec_ctx);
        ofile.video.codec_ctx =  nullptr;
    }
    if (ofile.fmt_ctx != nullptr) {
        avformat_free_context(ofile.fmt_ctx);
        ofile.fmt_ctx = nullptr;
    }

    if (fifo != nullptr) {
        av_audio_fifo_free(fifo);
        fifo= nullptr;
    }
}

void cls_infile::start(std::string fnm)
{
    is_started = false;
    defaults();
    if (decoder_init(fnm) != 0) {
        return;
    }
    if (decoder_get_ts() != 0) {
        return;
    }
    if (encoder_init() != 0) {
        return;
    }
    is_started = true;
}

cls_infile::cls_infile(cls_channel *p_chitm)
{
    chitm = p_chitm;
    defaults();
    ch_nbr = p_chitm->ch_nbr;
    frame = nullptr;
    pkt_in = nullptr;
    fifo = nullptr;

    pthread_mutex_init(&mtx, NULL);

}

cls_infile::~cls_infile()
{
   pthread_mutex_destroy(&mtx);
}
