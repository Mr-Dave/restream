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

#ifndef _INCLUDE_WEBUP_HPP_
#define _INCLUDE_WEBUP_HPP_
    class cls_webup {
        public:
            cls_webup(cls_app *p_app, cls_webua *p_webua);
            ~cls_webup();

            mhdrslt iterate_post (const char *key, const char *data, size_t datasz);
            mhdrslt processor_init();
            mhdrslt processor_start(const char *upload_data, size_t *upload_data_size);

        private:
            cls_app     *c_app;
            cls_webu    *c_webu;
            cls_webua   *c_webua;

            std::string     post_cmd;
            int             post_sz;        /* The number of entries in the post info */
            ctx_key         *post_info;     /* Structure of the entries provided from the post data */
            struct MHD_PostProcessor    *post_processor; /* Processor for handling Post method connections */
            void parse_cmd();
            void iterate_post_append(int indx, const char *data, size_t datasz);
            void iterate_post_new(const char *key, const char *data, size_t datasz);
            void process_actions();

    };

#endif /* _INCLUDE_WEBUP_HPP_ */
