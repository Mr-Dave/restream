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

#ifndef _INCLUDE_READER_H_
    #define _INCLUDE_READER_H_

    void *reader(void *parms);
    void reader_start(ctx_restream *restrm);
    void reader_startbyte(ctx_restream *restrm);
    void reader_close(ctx_restream *restrm);
    void reader_end(ctx_restream *restrm);
    void reader_flush(ctx_restream *restrm);
    void reader_init(ctx_restream *restrm);

#endif
