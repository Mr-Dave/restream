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
#include "guide.hpp"
#include "infile.hpp"
#include "webu.hpp"

void decoder_init_video(ctx_channel_item *chitm)
{
    int retcd;
    AVStream *stream;
    const AVCodec *dec;
    AVCodecContext *codec_ctx;

    chitm->ifile.video.codec_ctx = nullptr;
    stream = chitm->ifile.fmt_ctx->streams[chitm->ifile.video.index];
    chitm->ifile.video.strm = stream;
    dec = avcodec_find_decoder(stream->codecpar->codec_id);

    if (dec == nullptr) {
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Failed to find video decoder for input stream"
            , chitm->ch_nbr.c_str());
        return;
    }

    codec_ctx = avcodec_alloc_context3(dec);
    if (codec_ctx == nullptr) {
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Failed to allocate video decoder for input stream"
            , chitm->ch_nbr.c_str());
        return;
    }

    retcd = avcodec_parameters_to_context(codec_ctx, stream->codecpar);
    if (retcd < 0) {
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Failed to copy decoder parameters for stream"
            , chitm->ch_nbr.c_str());
        return;
    }

    codec_ctx->framerate = av_guess_frame_rate(chitm->ifile.fmt_ctx, stream, NULL);
    codec_ctx->pkt_timebase = stream->time_base;

    retcd = avcodec_open2(codec_ctx, dec, NULL);
    if (retcd < 0) {
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Failed to open video codec for input stream "
            , chitm->ch_nbr.c_str());
        return;
    }

    chitm->ifile.video.codec_ctx = codec_ctx;
}

void decoder_init_audio(ctx_channel_item *chitm)
{
    int retcd;
    AVStream *stream;
    const AVCodec *dec;
    AVCodecContext *codec_ctx;

    chitm->ifile.audio.codec_ctx = nullptr;
    stream = chitm->ifile.fmt_ctx->streams[chitm->ifile.audio.index];
    chitm->ifile.audio.strm = stream;
    dec = avcodec_find_decoder(stream->codecpar->codec_id);

    if (dec == nullptr) {
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Failed to find decoder for stream"
            , chitm->ch_nbr.c_str());
        return;
    }

    codec_ctx = avcodec_alloc_context3(dec);
    if (codec_ctx == nullptr) {
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Failed to allocate decoder for stream "
            , chitm->ch_nbr.c_str());
        return;
    }

    retcd = avcodec_parameters_to_context(codec_ctx, stream->codecpar);
    if (retcd < 0) {
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Failed to copy decoder parameters for stream "
            , chitm->ch_nbr.c_str());
        return;
    }
    codec_ctx->pkt_timebase = stream->time_base;

    retcd = avcodec_open2(codec_ctx, dec, NULL);
    if (retcd < 0) {
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Failed to open decoder for stream "
            , chitm->ch_nbr.c_str());
        return;
    }

    chitm->ifile.audio.codec_ctx = codec_ctx;

}

void decoder_init(ctx_channel_item *chitm)
{
    int retcd, indx;
    char errstr[128];
    std::string fnm;
    AVMediaType strm_typ;

    fnm = chitm->playlist[chitm->playlist_index].fullnm;

    LOG_MSG(NTC, NO_ERRNO, "%s: Opening file '%s'"
        , chitm->ch_nbr.c_str(), fnm.c_str());

    retcd = avformat_open_input(&chitm->ifile.fmt_ctx
        , fnm.c_str(), NULL, NULL);
    if (retcd < 0) {
        av_strerror(retcd, errstr, sizeof(errstr));
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Could not open input file '%s': %s"
            , chitm->ch_nbr.c_str(), fnm.c_str(), errstr);
        return;
    }

    retcd = avformat_find_stream_info(chitm->ifile.fmt_ctx, NULL);
    if (retcd < 0) {
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Failed to retrieve input stream information"
            , chitm->ch_nbr.c_str());
        return;
    }

    for (indx = 0; indx < (int)chitm->ifile.fmt_ctx->nb_streams; indx++) {
        strm_typ = chitm->ifile.fmt_ctx->streams[indx]->codecpar->codec_type;
        if ((strm_typ == AVMEDIA_TYPE_VIDEO) &&
            (chitm->ifile.video.index == -1)) {
            chitm->ifile.video.index = indx;
            decoder_init_video(chitm);
        } else if ((strm_typ == AVMEDIA_TYPE_AUDIO) &&
            (chitm->ifile.audio.index == -1)) {
            chitm->ifile.audio.index = indx;
            decoder_init_audio(chitm);
        }
    }

}

void decoder_get_ts(ctx_channel_item *chitm)
{
    int retcd, indx;

    /* Read some pkts to get correct start times */
    indx = 0;
    chitm->ifile.video.start_pts = 0;
    chitm->ifile.audio.start_pts = 0;
    while (indx < 100) {
        av_packet_free(&chitm->pkt);
        chitm->pkt = av_packet_alloc();
        chitm->pkt->data = NULL;
        chitm->pkt->size = 0;

        retcd = av_read_frame(chitm->ifile.fmt_ctx, chitm->pkt);
        if (retcd < 0){
            LOG_MSG(NTC, NO_ERRNO
                , "%s: Failed to read first packets for stream %d"
                , chitm->ch_nbr.c_str(), indx);
            return;
        }

        if (chitm->pkt->pts != AV_NOPTS_VALUE) {
            if (chitm->pkt->stream_index == chitm->ifile.video.index) {
                chitm->ifile.video.start_pts = chitm->pkt->pts;
            }
            if (chitm->pkt->stream_index == chitm->ifile.audio.index) {
                chitm->ifile.audio.start_pts = chitm->pkt->pts;
            }
        }

        if ((chitm->ifile.video.start_pts > 0) &&
            (chitm->ifile.audio.start_pts > 0)) {
            break;
        }

        indx++;
    }

    chitm->ifile.time_start = av_gettime_relative();
    chitm->ifile.audio.last_pts = chitm->ifile.audio.start_pts;
    chitm->ifile.video.last_pts = chitm->ifile.video.start_pts;
    chitm->file_cnt++;
}

static void infile_wait(ctx_channel_item *chitm)
{
    int64_t tm_diff, pts_diff, tot_diff;
    int indx_last, chk;

    if (finish == true) {
        return;
    }

    if (chitm->cnct_cnt > 0 ) {
        indx_last = pktarray_get_lastwritten(chitm);
        chk = 0;
        while ((chitm->pktarray_index == indx_last) && (chk <1000)) {
            /*
            LOG_MSG(ERR, NO_ERRNO, "sleeping %d ",chk);
            */
            SLEEP(0, 250000000L);
            indx_last = pktarray_get_lastwritten(chitm);
            chk++;
        }
        if (chk == 1000) {
            LOG_MSG(NTC, NO_ERRNO
                , "%s: Excessive wait for connection writing."
                , chitm->ch_nbr.c_str());
        }
        return;
    }

    if ((chitm->pkt->stream_index != chitm->ifile.video.index) ||
        (chitm->pkt->pts == AV_NOPTS_VALUE)  ||
        (chitm->pkt->pts < chitm->ifile.video.last_pts)) {
        return;
    }

    chitm->ifile.video.last_pts = chitm->pkt->pts;

    /* How much time has really elapsed since the start of movie*/
    tm_diff = av_gettime_relative() - chitm->ifile.time_start;

    /* How much time the pts wants us to be at since the start of the movie */
    pts_diff = av_rescale(chitm->pkt->pts - chitm->ifile.video.start_pts
        , 1000000, chitm->ifile.video.strm->time_base.den);

    /* How much time we need to wait to get in sync*/
    tot_diff = pts_diff - tm_diff;

    if (tot_diff > 0){
        if (tot_diff < 1000000){
            SLEEP(0, tot_diff * 1000);
        } else {
            /* reset all our times to see if we can get in sync*/
            chitm->ifile.time_start = av_gettime_relative();
            chitm->ifile.video.start_pts = chitm->pkt->pts;
            chitm->ifile.video.last_pts =chitm->pkt->pts;
        }
    }
}

static void decoder_send(ctx_channel_item *chitm)
{
    int retcd;
    char errstr[128];

    if (chitm->pkt->stream_index == chitm->ifile.video.index) {
        retcd = avcodec_send_packet(chitm->ifile.video.codec_ctx, chitm->pkt);
/*
        LOG_MSG(NTC, NO_ERRNO
            , "%s: stream %d pts %d stpts %d"
            , chitm->ch_nbr.c_str()
            , chitm->pkt->stream_index
            , chitm->pkt->pts
            , chitm->ifile.video.start_pts);
*/
    } else {
        retcd = avcodec_send_packet(chitm->ifile.audio.codec_ctx, chitm->pkt);
    }
    if (retcd == AVERROR_INVALIDDATA) {
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Send ignoring packet stream %d with invalid data"
            , chitm->ch_nbr.c_str(), chitm->pkt->stream_index);
            return;
    } else if (retcd < 0 && retcd != AVERROR_EOF) {
        av_strerror(retcd, errstr, sizeof(errstr));
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Error sending packet to codec: %s"
            , chitm->ch_nbr.c_str(), errstr);
        return;
    }

}

static void decoder_receive(ctx_channel_item *chitm)
{
    int retcd;
    char errstr[128];

    if (chitm->frame != nullptr) {
        myframe_free(chitm->frame);
        chitm->frame = nullptr;
    }
    chitm->frame = myframe_alloc();

    if (chitm->pkt->stream_index == chitm->ifile.video.index) {
       retcd = avcodec_receive_frame(chitm->ifile.video.codec_ctx, chitm->frame);
    } else {
       retcd = avcodec_receive_frame(chitm->ifile.audio.codec_ctx, chitm->frame);
    }
    if (retcd < 0) {
        if (retcd == AVERROR_INVALIDDATA) {
            LOG_MSG(NTC, NO_ERRNO
                , "%s: Ignoring packet with invalid data"
                , chitm->ch_nbr.c_str());
        } else if (retcd != AVERROR(EAGAIN)) {
            av_strerror(retcd, errstr, sizeof(errstr));
            LOG_MSG(NTC, NO_ERRNO
                , "%s: Error receiving frame from decoder: %s"
                , chitm->ch_nbr.c_str(), errstr);
        }
        if (chitm->frame != nullptr) {
            myframe_free(chitm->frame);
            chitm->frame = nullptr;
        }
    }

}

static int encode_buffer_audio(ctx_channel_item *chitm)
{
    int frame_size, retcd;
    int64_t  pts, dts;
    char errstr[128];

    retcd = av_audio_fifo_realloc(chitm->fifo
        , chitm->ifile.audio.codec_ctx->frame_size +
          chitm->ofile.audio.codec_ctx->frame_size);
    if (retcd < 0) {
        LOG_MSG(NTC, NO_ERRNO, "%s: Could not allocate FIFO"
            , chitm->ch_nbr.c_str());
        return -1;
    }

    retcd = av_audio_fifo_write(
        chitm->fifo, (void **)chitm->frame->data
        , chitm->ifile.audio.codec_ctx->frame_size);
    if (retcd < chitm->ifile.audio.codec_ctx->frame_size) {
        LOG_MSG(NTC, NO_ERRNO, "%s: Could not write data to FIFO"
            , chitm->ch_nbr.c_str());
        return -1;
    }
    pts = chitm->frame->pts;
    dts = chitm->frame->pkt_dts;

    myframe_free(chitm->frame);
    chitm->frame = nullptr;

    retcd = av_audio_fifo_size(chitm->fifo);
    if (retcd < chitm->ofile.audio.codec_ctx->frame_size) {
        return -1;
    }

    frame_size = FFMIN(
        av_audio_fifo_size(chitm->fifo)
        , chitm->ofile.audio.codec_ctx->frame_size);

    chitm->frame = av_frame_alloc();
    if (chitm->frame == nullptr) {
        LOG_MSG(NTC, NO_ERRNO, "%s: Could not allocate output frame"
            , chitm->ch_nbr.c_str());
        return -1;
    }
    chitm->frame->nb_samples     = frame_size;
    av_channel_layout_copy(&chitm->frame->ch_layout
        , &chitm->ofile.audio.codec_ctx->ch_layout);
    chitm->frame->format         = chitm->ofile.audio.codec_ctx->sample_fmt;
    chitm->frame->sample_rate    = chitm->ofile.audio.codec_ctx->sample_rate;
    chitm->frame->pts = pts;
    chitm->frame->pkt_dts = dts;

    retcd = av_frame_get_buffer(chitm->frame, 0);
    if (retcd < 0) {
        av_strerror(retcd, errstr, sizeof(errstr));
        LOG_MSG(NTC, NO_ERRNO, "%s: Could not allocate output frame buffer %s"
            , chitm->ch_nbr.c_str(), errstr);
        av_frame_free(&chitm->frame);
        chitm->frame = nullptr;
        return -1;
    }

    retcd = av_audio_fifo_read(chitm->fifo
        , (void **)chitm->frame->data, frame_size);
    if (retcd < frame_size) {
        fprintf(stderr, "Could not read data from FIFO\n");
        LOG_MSG(NTC, NO_ERRNO, "%s: Could not read data from fifo"
            , chitm->ch_nbr.c_str());
        av_frame_free(&chitm->frame);
        chitm->frame = nullptr;
        return -1;
    }

    return 0;
}

static void encoder_send(ctx_channel_item *chitm)
{
    int retcd;
    char errstr[128];

    if (chitm->frame == nullptr) {
        return;
    }

    if (chitm->pkt->stream_index == chitm->ifile.video.index) {
        retcd = avcodec_send_frame(chitm->ofile.video.codec_ctx, chitm->frame);
    } else {
        if (chitm->ifile.audio.codec_ctx->codec_id == AV_CODEC_ID_AAC) {
            retcd = encode_buffer_audio(chitm);
            if (retcd < 0) {
                return;
            }
        }
        retcd = avcodec_send_frame(chitm->ofile.audio.codec_ctx, chitm->frame);
    }
    if (retcd < 0 ) {
        av_strerror(retcd, errstr, sizeof(errstr));
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Error sending %d frame for encoding: %s"
            , chitm->ch_nbr.c_str(), chitm->pkt->stream_index, errstr);
        abort();
    }

    myframe_free(chitm->frame);
    chitm->frame = nullptr;

}

static void packetarray_resize(ctx_channel_item *chitm)
{
    ctx_packet_item pktitm;
    int indx;

    LOG_MSG(DBG, NO_ERRNO, "Resizing array");

    pktitm.idnbr = -1;
    pktitm.iskey = false;
    pktitm.iswritten = false;
    pktitm.packet = nullptr;
    pktitm.file_cnt = 0;
    pktitm.start_pts = 0;
    pktitm.timebase={0,0};

    pthread_mutex_lock(&chitm->mtx_pktarray);
        /* 180 = 3 second buffer... usually... */
        for (indx=1; indx <= 180; indx++) {
            chitm->pktarray.push_back(pktitm);
        }
        chitm->pktarray_count = 180;
    pthread_mutex_unlock(&chitm->mtx_pktarray);
}

int pktarray_get_lastwritten(ctx_channel_item *chitm)
{
    int retval;
    pthread_mutex_lock(&chitm->mtx_pktarray);
        retval = chitm->pktarray_lastwritten;
    pthread_mutex_unlock(&chitm->mtx_pktarray);
    return retval;
}

int pktarray_get_index(ctx_channel_item *chitm)
{
    int retval;
    pthread_mutex_lock(&chitm->mtx_pktarray);
        retval = chitm->pktarray_lastwritten;
    pthread_mutex_unlock(&chitm->mtx_pktarray);
    return retval;
}

int pktarray_indx_next(int index)
{
    if (index == 179) {
        return 0;
    } else {
        return ++index;
    }
}

int pktarray_indx_prev(int index)
{
    if (index == 0) {
        return 179;
    } else {
        return --index;
    }
}

/* Add a packet to the processing array */
static void packetarray_add(ctx_channel_item *chitm, AVPacket *pkt)
{
    int indx_next, retcd;
    char errstr[128];

    if (chitm->pktarray_count == 0) {
        packetarray_resize(chitm);
    }

    indx_next = pktarray_indx_next(chitm->pktarray_index);

    pthread_mutex_lock(&chitm->mtx_pktarray);

        chitm->pktnbr++;
        chitm->pktarray[indx_next].idnbr = chitm->pktnbr;

        if (chitm->pktarray[indx_next].packet != nullptr) {
            mypacket_free(chitm->pktarray[indx_next].packet);
            chitm->pktarray[indx_next].packet = nullptr;
        }
        chitm->pktarray[indx_next].packet = mypacket_alloc(chitm->pktarray[indx_next].packet);

        retcd = mycopy_packet(chitm->pktarray[indx_next].packet, pkt);
        if (retcd < 0) {
            av_strerror(retcd, errstr, sizeof(errstr));
            LOG_MSG(NTC, NO_ERRNO
                , "%s: Error copying packet: %s"
                , chitm->ch_nbr.c_str(), errstr);
            mypacket_free(chitm->pktarray[indx_next].packet);
            chitm->pktarray[indx_next].packet = NULL;
        }

        if (chitm->pktarray[indx_next].packet->flags & AV_PKT_FLAG_KEY) {
            chitm->pktarray[indx_next].iskey = true;
        } else {
            chitm->pktarray[indx_next].iskey = false;
        }
        chitm->pktarray[indx_next].iswritten = false;
        chitm->pktarray[indx_next].file_cnt = chitm->file_cnt;
        if (chitm->pkt->stream_index == chitm->ifile.video.index) {
            chitm->pktarray[indx_next].timebase = chitm->ifile.video.strm->time_base;
            chitm->pktarray[indx_next].start_pts= chitm->ifile.video.start_pts;
        } else {
            chitm->pktarray[indx_next].timebase = chitm->ifile.audio.strm->time_base;
            chitm->pktarray[indx_next].start_pts= chitm->ifile.audio.start_pts;
        }

        chitm->pktarray_index = indx_next;

    pthread_mutex_unlock(&chitm->mtx_pktarray);

/*
    LOG_MSG(ERR, NO_ERRNO," %d %d/%d %d/%d %d/%d %d/%d "
        ,chitm->ifile.video.codec_ctx->framerate
        ,chitm->ifile.video.strm->time_base.num
        ,chitm->ifile.video.strm->time_base.den
        ,chitm->ifile.video.codec_ctx->time_base.num
        ,chitm->ifile.video.codec_ctx->time_base.den
        ,chitm->ofile.video.strm->time_base.num
        ,chitm->ofile.video.strm->time_base.den
        ,chitm->ofile.video.codec_ctx->time_base.num
        ,chitm->ofile.video.codec_ctx->time_base.den
    );
*/

}

static void encoder_receive(ctx_channel_item *chitm)
{
    int retcd;
    char errstr[128];
    AVPacket *pkt;

    retcd = 0;
    while (retcd == 0) {
        pkt = NULL;
        pkt = mypacket_alloc(pkt);
        if (chitm->pkt->stream_index == chitm->ifile.video.index) {
            retcd = avcodec_receive_packet(chitm->ofile.video.codec_ctx, pkt);
            pkt->stream_index =chitm->ofile.video.index;
        } else {
            retcd = avcodec_receive_packet(chitm->ofile.audio.codec_ctx, pkt);
            pkt->stream_index =chitm->ofile.audio.index;
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
            //    , chitm->ch_nbr.c_str(), pkt->size);
            if (pkt->pts > 0) {
                packetarray_add(chitm, pkt);
            }
            mypacket_free(pkt);
            pkt = NULL;
        }
    }
}

static void process_packet(ctx_channel_item *chitm)
{
    if (chitm->pkt->stream_index == chitm->ifile.video.index) {
        //packetarray_add(chitm,chitm->pkt);
        //return;
        decoder_send(chitm);
        decoder_receive(chitm);
        encoder_send(chitm);
        encoder_receive(chitm);
    } else {
        decoder_send(chitm);
        decoder_receive(chitm);
        encoder_send(chitm);
        encoder_receive(chitm);
    }
}

void infile_read(ctx_channel_item *chitm)
{
    int retcd;

    while (finish == false) {
        av_packet_free(&chitm->pkt);
        chitm->pkt = av_packet_alloc();

        chitm->pkt->data = NULL;
        chitm->pkt->size = 0;

        retcd = av_read_frame(chitm->ifile.fmt_ctx, chitm->pkt);
        if (retcd < 0) break;

        infile_wait(chitm);

        if (chitm->cnct_cnt > 0) {
            process_packet(chitm);
        }
        if (chitm->ifile.fmt_ctx == NULL) break;
    }
    av_packet_free(&chitm->pkt);
}

static void encoder_init_video(ctx_channel_item *chitm)
{
    const AVCodec *encoder;
    AVStream *stream;
    AVCodecContext *enc_ctx,*dec_ctx;
    int retcd;

    chitm->ofile.video.codec_ctx = nullptr;
    stream = avformat_new_stream(chitm->ofile.fmt_ctx, NULL);
    chitm->ofile.video.strm = stream;
    if (stream == nullptr) {
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Failed allocating output video stream"
            , chitm->ch_nbr.c_str());
        return;
    }
    chitm->ofile.video.index = stream->index;

    encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (encoder == nullptr) {
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Could not find video encoder"
            , chitm->ch_nbr.c_str());
        return;
    }

    chitm->ofile.video.codec_ctx = avcodec_alloc_context3(encoder);
    if (chitm->ofile.video.codec_ctx == nullptr) {
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Could not allocate video encoder"
            , chitm->ch_nbr.c_str());
        return;
    }

    enc_ctx = chitm->ofile.video.codec_ctx;
    dec_ctx = chitm->ifile.video.codec_ctx;

    retcd = avcodec_parameters_from_context(stream->codecpar, dec_ctx);
    if (retcd < 0) {
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Could not copy parms from video decoder"
            , chitm->ch_nbr.c_str());
        return;
    }

    retcd = avcodec_parameters_to_context(enc_ctx, stream->codecpar);
    if (retcd < 0) {
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Could not copy parms to video encoder"
            , chitm->ch_nbr.c_str());
        return;
    }

    enc_ctx->width = dec_ctx->width;
    enc_ctx->height = dec_ctx->height;
    enc_ctx->time_base.num = 1;
    enc_ctx->time_base.den = 90000;
    enc_ctx->framerate.num = 30;
    enc_ctx->framerate.den = 1;

    if (dec_ctx->pix_fmt == -1) {
        enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    } else {
        enc_ctx->pix_fmt = dec_ctx->pix_fmt;
    }
    av_opt_set(enc_ctx->priv_data, "profile", "main", 0);
    av_opt_set(enc_ctx->priv_data, "crf", "22", 0);
    av_opt_set(enc_ctx->priv_data, "tune", "zerolatency", 0);
    av_opt_set(enc_ctx->priv_data, "preset", "superfast",0);

    retcd = avcodec_open2(enc_ctx, encoder, NULL);
    if (retcd < 0) {
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Could not open video encoder"
            , chitm->ch_nbr.c_str());
        LOG_MSG(NTC, NO_ERRNO
            , "%s: %dx%d %d %d"
            , chitm->ch_nbr.c_str()
            , enc_ctx->width, enc_ctx->height
            , enc_ctx->pix_fmt, enc_ctx->time_base.den);
        return;
    }

    if (chitm->ofile.fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    stream->time_base = enc_ctx->time_base;

    retcd = avcodec_parameters_from_context(stream->codecpar, enc_ctx);
    if (retcd < 0) {
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Could not copy parms from decoder"
            , chitm->ch_nbr.c_str());
        return;
    }
}

static void encoder_init_audio(ctx_channel_item *chitm)
{
    AVStream *stream;
    AVCodecContext *enc_ctx,*dec_ctx;
    int retcd;
    char errstr[128];
    AVDictionary *opts = NULL;
    const AVCodec *encoder;

    chitm->ofile.audio.codec_ctx = nullptr;

    encoder = avcodec_find_encoder(AV_CODEC_ID_AC3);

    stream = avformat_new_stream(chitm->ofile.fmt_ctx, encoder);
    chitm->ofile.audio.strm = stream;
    if (stream == nullptr) {
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Failed allocating output stream"
            , chitm->ch_nbr.c_str());
        return;
    }
    chitm->ofile.audio.index = stream->index;

    chitm->ofile.audio.codec_ctx = avcodec_alloc_context3(encoder);
    if (chitm->ofile.audio.codec_ctx == nullptr) {
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Could not allocate audio encoder"
            , chitm->ch_nbr.c_str());
        return;
    }

    enc_ctx = chitm->ofile.audio.codec_ctx;
    dec_ctx = chitm->ifile.audio.codec_ctx;

    chitm->ofile.fmt_ctx->audio_codec_id = AV_CODEC_ID_AC3;
    chitm->ofile.fmt_ctx->audio_codec = avcodec_find_encoder(AV_CODEC_ID_AC3);
    stream->codecpar->codec_id=AV_CODEC_ID_AC3;

    stream->codecpar->bit_rate = dec_ctx->bit_rate;
    stream->codecpar->frame_size = dec_ctx->frame_size;
    stream->codecpar->format = dec_ctx->sample_fmt;
    stream->codecpar->sample_rate = dec_ctx->sample_rate;
    stream->time_base.den  = dec_ctx->sample_rate;
    stream->time_base.num = 1;
    av_channel_layout_default(&stream->codecpar->ch_layout
        , dec_ctx->ch_layout.nb_channels);

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

    retcd = avcodec_open2(enc_ctx, encoder, &opts);
    if (retcd < 0) {
        av_strerror(retcd, errstr, sizeof(errstr));
        LOG_MSG(NTC, NO_ERRNO
            , "%s: Could not open audio encoder %s"
            , chitm->ch_nbr.c_str(), errstr);
        abort();
        return;
    }
    av_dict_free(&opts);


    if (dec_ctx->codec_id == AV_CODEC_ID_AAC ) {
        /* Create the FIFO buffer based on the specified output sample format. */
        chitm->fifo = av_audio_fifo_alloc(
            chitm->ofile.audio.codec_ctx->sample_fmt
            , chitm->ofile.audio.codec_ctx->ch_layout.nb_channels, 1);
        if (chitm->fifo == nullptr) {
            LOG_MSG(NTC, NO_ERRNO, "%s: Could not allocate FIFO"
                , chitm->ch_nbr.c_str());
            return;
        }
    }

    return;
}

void encoder_init(ctx_channel_item *chitm)
{
    if (chitm->ifile.fmt_ctx == NULL) {
        LOG_MSG(NTC, NO_ERRNO
            , "%s: no input file provided"
            , chitm->ch_nbr.c_str());
        return;
    }

    chitm->ofile.fmt_ctx = avformat_alloc_context();
    chitm->ofile.fmt_ctx->oformat = av_guess_format("mpegts", NULL, NULL);

    if (chitm->ifile.video.index != -1) {
        encoder_init_video(chitm);
    }
    if (chitm->ifile.audio.index != -1) {
        encoder_init_audio(chitm);
    }

}

void streams_close(ctx_channel_item *chitm)
{
    LOG_MSG(NTC, NO_ERRNO, "%s: Closing "
        , chitm->ch_nbr.c_str());

    if (chitm->ifile.audio.codec_ctx !=  nullptr) {
        avcodec_free_context(&chitm->ifile.audio.codec_ctx);
        chitm->ifile.audio.codec_ctx =  nullptr;
    }
    if (chitm->ifile.video.codec_ctx !=  nullptr) {
        avcodec_free_context(&chitm->ifile.video.codec_ctx);
        chitm->ifile.video.codec_ctx =  nullptr;
    }
    if (chitm->ifile.fmt_ctx != nullptr) {
        avformat_close_input(&chitm->ifile.fmt_ctx);
        chitm->ifile.fmt_ctx = nullptr;
    }

    if (chitm->ofile.audio.codec_ctx !=  nullptr) {
        avcodec_free_context(&chitm->ofile.audio.codec_ctx);
        chitm->ofile.audio.codec_ctx =  nullptr;
    }
    if (chitm->ofile.video.codec_ctx !=  nullptr) {
        avcodec_free_context(&chitm->ofile.video.codec_ctx);
        chitm->ofile.video.codec_ctx =  nullptr;
    }
    if (chitm->ofile.fmt_ctx != nullptr) {
        avformat_free_context(chitm->ofile.fmt_ctx);
        chitm->ofile.fmt_ctx = nullptr;
    }

    if (chitm->fifo != nullptr) {
        av_audio_fifo_free(chitm->fifo);
    }

}


