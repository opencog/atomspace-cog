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
#include <mutex>
#include <set>
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
		std::atomic_int _nsocks{0};

		// Socket API.
		static thread_local struct tlso {
			int _sockfd;
			CogChannel* _owner;
			tlso() : _sockfd(0), _owner(nullptr) {}
			~tlso() { if (_sockfd) { close(_sockfd); if (_owner) _owner->_nsocks--; }}
		} s;
		int open_sock();
		void do_send(const std::string&);
		std::string do_recv(bool=false);

		struct Msg
		{
			Client* client;
			void (Client::*callback)(const std::string&, const Data&);
			size_t sequence;
			bool noreply;  // If true, skip do_recv()

			std::string str_to_send;
			Data data;

			// Sequence counter for non-idempotent messages
			static std::atomic<size_t> _sequence_counter;

			// Default constructor required by concurrent_set
			Msg() : client(nullptr), callback(nullptr), sequence(0), noreply(true) {}

			// The mesage buffer is a de-duplicating buffer: identical
			// messages are added only once. This makes sense for almost
			// all messages: we do not need to store an Atom twice; the
			// store operation is idempotent. There are two exceptions.
			// One is the cog-value-increment message: these are never
			// idempotent; we use a sequence number to make sure each is
			// unique. The other case is the barrier. The exact same
			// barrier message must be sent to each socket. The barrier
			// code will make sure that the queues are all empty before
			// enqueuing the barrier, so ordering does not matter, but
			// having exactly NTHREADS copies does. It must not be
			// deduplicated So that is what this ctor does.
			Msg(Client* c, void (Client::*cb)(const std::string&, const Data&),
			    bool nr, const std::string& str, const Data& d)
				: client(c), callback(cb), noreply(nr),
				  str_to_send(str), data(d)
			{
				if (str.compare(0, 19, "(cog-update-value!") == 0 or
				    str.compare(0, 13, "(cog-barrier)") == 0)
				{
					sequence = ++_sequence_counter;
				}
				else
				{
					sequence = 0;
				}
			}

			// Need operator<() in order to queue up the messages.
			// If both sequence numbers are zero, compare by string content
			// (idempotent messages deduplicate). Otherwise compare by
			// sequence number (non-idempotent messages stay unique).
			bool operator<(const Msg& other) const
			{
				if (sequence == 0 and other.sequence == 0)
					return str_to_send < other.str_to_send;
				return sequence < other.sequence;
			}
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
		void enqueue_noreply(const std::string&);
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
