/*
 * FILE:
 * opencog/persist/cog-storage/CogChannel.h
 *
 * FUNCTION:
 * CogServer I/O communications channel.
 *
 * HISTORY:
 * Copyright (c) 2020 Linas Vepstas <linasvepstas@gmail.com>
 *
 * LICENSE:
 * SPDX-License-Identifier: AGPL-3.0-or-later
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _OPENCOG_COG_CHANNEL_H
#define _OPENCOG_COG_CHANNEL_H

#include <atomic>
#include <string>
#include <unistd.h> /* for close() */

#include <opencog/util/async_buffer.h>

namespace opencog
{
/** \addtogroup grp_persist
 *  @{
 */

template<typename Client, typename Data>
class CogChannel
{
	private:
		std::string _uri;
		std::string _host;
		std::string _port;
		void* _servinfo;
		static std::atomic_int _nsocks;

		// Socket API.
		static thread_local struct tlso {
			int _sockfd;
			tlso() { _sockfd = 0; }
			~tlso() { if (_sockfd) { close(_sockfd); _nsocks--; }}
		} s;
		int open_sock();
		void do_send(const std::string&);
		std::string do_recv(bool=false);

		struct Msg
		{
			std::string str_to_send;
			Data data;
			Client* client;
			void (Client::*callback)(const std::string&, const Data&);

			// Need operator<() in order to queue up the messages.
			int operator<(const Msg& other) const
			{ return this < &other; }
		};

		async_buffer<CogChannel, Msg> _msg_buffer;
		void reply_handler(const Msg&);

	public:
		CogChannel(void);
		CogChannel(const CogChannel&) = delete; // disable copying
		CogChannel& operator=(const CogChannel&) = delete; // disable assignment
		~CogChannel();

		void open_connection(const std::string& uri);
		void close_connection(void);
		bool connected(void); // connection to DB is alive

		void enqueue(Client*, const std::string&, Data&,
		             void (Client::*)(const std::string&, const Data&));
		void synchro(Client*, const std::string&, Data&,
		             void (Client::*)(const std::string&, Data&));

		void barrier();
		void flush();

		void clear_stats();
		std::string print_stats();
};

/** @}*/
} // namespace opencog

#endif // _OPENCOG_COG_CHANNEL_H
