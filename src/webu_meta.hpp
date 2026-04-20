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

#ifndef _INCLUDE_WEBU_META_HPP_
#define _INCLUDE_WEBU_META_HPP_
    class cls_webum {
        public:
            cls_webum(cls_app *p_app, cls_webua *p_webua);
            ~cls_webum();
            void main(std::string &p_resp, enum WEBUA_RESP &p_type);

        private:
            cls_app         *c_app;
            cls_config      *c_conf;
            cls_webu        *c_webu;
            cls_webua       *c_webua;
            cls_channel     *chitm;
            
            void get_m3u8(std::string &p_resp, enum WEBUA_RESP &p_type);
            void get_xmltv(std::string &p_resp
                , enum WEBUA_RESP &p_type, cls_channel *p_chitm);

    };

#endif /* _INCLUDE_WEBU_META_HPP_ */
