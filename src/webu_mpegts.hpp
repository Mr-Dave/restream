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
 *    Copyright 2020-2026 MotionMrDave@gmail.com
*/

#ifndef _INCLUDE_WEBU_MPEGTS_HPP_
#define _INCLUDE_WEBU_MPEGTS_HPP_
    class cls_webuts {
        public:
            cls_webuts(cls_app *p_app, cls_webua *p_webua);
            ~cls_webuts();

            int mpegts_avio_buf(myuint *buf, int buf_size);
            ssize_t mpegts_response(uint64_t pos, char *buf, size_t max);
            mhdrslt main();

        private:
            cls_app         *c_app;
            cls_config      *c_conf;
            cls_webu        *c_webu;
            cls_webua       *c_webua;
            cls_channel     *chitm;

            struct MHD_Connection       *connection;    /* The MHD connection value from the client */

            unsigned char               *resp_image;    /* Response image to provide to user */

            size_t                      resp_size;      /* The allocated size of the response */
            size_t                      resp_used;      /* The amount of the response page used */
            size_t                      aviobuf_sz;     /* The size of the mpegts avio buffer */
            ctx_file_info               wfile;
            int64_t                     file_cnt;
            int                         start_cnt;
            uint64_t                    stream_pos;     /* Stream position of sent image */
            int                         stream_fps;     /* Stream rate per second */
            struct timespec             time_last;      /* Keep track of processing time for stream thread*/

            int64_t                     msec_cnt;
            AVPacket                    *pkt;
            int                         pkt_index;
            int64_t                     pkt_idnbr;
            int64_t                     pkt_start_pts;
            AVRational                  pkt_timebase;
            int64_t                     pkt_file_cnt;
            bool                        pkt_key;

            void free_context();
            void resetpos();
            void packet_wait();
            void packet_pts();
            void packet_write();
            void pkt_copy(int indx);
            bool pkt_get(int indx);
            void getimg();
            int streams_video_h264();
            int streams_video_mpeg();
            int streams_audio();
            int open();
    };

#endif /* _INCLUDE_WEBU_MPEGTS_HPP_ */
