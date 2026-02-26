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

#ifndef _INCLUDE_UTIL_HPP_
#define _INCLUDE_UTIL_HPP_

    #if (MYFFVER >= 56000)
        #define MY_PIX_FMT_YUV420P   AV_PIX_FMT_YUV420P
        #define MY_PIX_FMT_YUVJ420P  AV_PIX_FMT_YUVJ420P
        #define MyPixelFormat AVPixelFormat
    #else  //Old ffmpeg pixel formats
        #define MY_PIX_FMT_YUV420P   PIX_FMT_YUV420P
        #define MY_PIX_FMT_YUVJ420P  PIX_FMT_YUVJ420P
        #define MyPixelFormat PixelFormat
    #endif  //Libavformat >= 56

    #if (MYFFVER > 54006)
        #define MY_FLAG_READ       AVIO_FLAG_READ
        #define MY_FLAG_WRITE      AVIO_FLAG_WRITE
        #define MY_FLAG_READ_WRITE AVIO_FLAG_READ_WRITE
    #else  //Older versions
        #define MY_FLAG_READ       URL_RDONLY
        #define MY_FLAG_WRITE      URL_WRONLY
        #define MY_FLAG_READ_WRITE URL_RDWR
    #endif

    #if (MYFFVER >= 56000)
        #define MY_CODEC_ID_MSMPEG4V2  AV_CODEC_ID_MSMPEG4V2
        #define MY_CODEC_ID_FLV1       AV_CODEC_ID_FLV1
        #define MY_CODEC_ID_FFV1       AV_CODEC_ID_FFV1
        #define MY_CODEC_ID_NONE       AV_CODEC_ID_NONE
        #define MY_CODEC_ID_MPEG2VIDEO AV_CODEC_ID_MPEG2VIDEO
        #define MY_CODEC_ID_H264       AV_CODEC_ID_H264
        #define MY_CODEC_ID_HEVC       AV_CODEC_ID_HEVC
        #define MY_CODEC_ID_THEORA     AV_CODEC_ID_THEORA
        #define MY_CODEC_ID_VP8        AV_CODEC_ID_VP8
        #define MY_CODEC_ID_VP9        AV_CODEC_ID_VP9
    #else
        #define MY_CODEC_ID_MSMPEG4V2  CODEC_ID_MSMPEG4V2
        #define MY_CODEC_ID_FLV1       CODEC_ID_FLV1
        #define MY_CODEC_ID_FFV1       CODEC_ID_FFV1
        #define MY_CODEC_ID_NONE       CODEC_ID_NONE
        #define MY_CODEC_ID_MPEG2VIDEO CODEC_ID_MPEG2VIDEO
        #define MY_CODEC_ID_H264       CODEC_ID_H264
        #define MY_CODEC_ID_HEVC       CODEC_ID_H264
        #define MY_CODEC_ID_THEORA     CODEC_ID_THEORA
        #define MY_CODEC_ID_VP8        CODEC_ID_VP8
        #define MY_CODEC_ID_VP9        CODEC_ID_VP9
    #endif
    #if (MYFFVER <= 60016)
        typedef uint8_t myuint;         /* Version independent uint */
        #define MY_PROFILE_H264_HIGH   FF_PROFILE_H264_HIGH
    #else
        typedef const uint8_t myuint;   /* Version independent uint */
        #define MY_PROFILE_H264_HIGH   AV_PROFILE_H264_HIGH
    #endif
    #if (LIBAVCODEC_VERSION_MAJOR >= 57)
        #define MY_CODEC_FLAG_GLOBAL_HEADER AV_CODEC_FLAG_GLOBAL_HEADER
        #define MY_CODEC_FLAG_QSCALE        AV_CODEC_FLAG_QSCALE
    #else
        #define MY_CODEC_FLAG_GLOBAL_HEADER CODEC_FLAG_GLOBAL_HEADER
        #define MY_CODEC_FLAG_QSCALE        CODEC_FLAG_QSCALE
    #endif

    #if (LIBAVCODEC_VERSION_MAJOR >= 59)
        typedef const AVCodec myAVCodec; /* Version independent definition for AVCodec*/
    #else
        typedef AVCodec myAVCodec; /* Version independent definition for AVCodec*/
    #endif

    #if MHD_VERSION >= 0x00097002
        typedef enum MHD_Result mhdrslt; /* Version independent return result from MHD */
    #else
        typedef int             mhdrslt; /* Version independent return result from MHD */
    #endif

    void myfree(void *ptr_addr);

    void *mymalloc(size_t nbytes);
    void *myrealloc(void *ptr, size_t size, const char *desc);
    FILE *myfopen(const char *path, const char *mode);
    int myfclose(FILE *fh);

    void mythreadname_set(const char *abbr, int threadnbr, const char *threadname);
    void mythreadname_get(char *threadname);

    int mystrceq(const char* var1, const char* var2);
    int mystrcne(const char* var1, const char* var2);
    int mystreq(const char* var1, const char* var2);
    int mystrne(const char* var1, const char* var2);
    void myltrim(std::string &parm);
    void myrtrim(std::string &parm);
    void mytrim(std::string &parm);
    void myunquote(std::string &parm);

    AVFrame *myframe_alloc(void);
    void myframe_free(AVFrame *frame);
    void mypacket_free(AVPacket *pkt);
    void myavcodec_close(AVCodecContext *codec_context);
    int myimage_get_buffer_size(enum MyPixelFormat pix_fmt, int width, int height);
    int myimage_copy_to_buffer(AVFrame *frame,uint8_t *buffer_ptr,enum MyPixelFormat pix_fmt,int width,int height,int dest_size);
    int myimage_fill_arrays(AVFrame *frame,uint8_t *buffer_ptr,enum MyPixelFormat pix_fmt,int width,int height);
    int mycopy_packet(AVPacket *dest_pkt, AVPacket *src_pkt);
    AVPacket *mypacket_alloc(AVPacket *pkt);

    void util_parms_parse(ctx_params &params, std::string parm_desc, std::string confline);
    void util_parms_add_default(ctx_params &params, std::string parm_nm, std::string parm_vl);
    void util_parms_add_default(ctx_params &params, std::string parm_nm, int parm_vl);
    void util_parms_add(ctx_params &params, std::string parm_nm, std::string parm_val);
    void util_parms_update(ctx_params &params, std::string &confline);

#endif /* _INCLUDE_UTIL_HPP_ */
