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

#ifndef _INCLUDE_CHANNEL_HPP_
#define _INCLUDE_CHANNEL_HPP_
    class cls_channel {
        public:
            cls_channel(int p_indx, std::string p_conf);
            ~cls_channel();

            cls_infile      *infile;
            cls_pktarray    *pktarray;
            int64_t         file_cnt;
            int             cnct_cnt;

            bool            ch_finish;
            bool            ch_running;
            std::string     ch_nbr;
            std::string     ch_encode;
            int             ch_sync;
            std::vector<ctx_playlist_item>    playlist;
            int             playlist_index;
            int             playlist_count;
            time_t          start_tm;

            void    process();

        private:
            std::string     ch_conf;
            ctx_params      ch_params;
            bool            ch_tvhguide;
            std::string     ch_sort;
            std::string     ch_dir;
            int             ch_index;

            void            playlist_load();

            void guide_times(
                std::string f1, std::string &st1,std::string &en1,
                std::string f2, std::string &st2,std::string &en2);
            void guide_write_xml(std::string &xml);
            void guide_process();
    };

#endif
