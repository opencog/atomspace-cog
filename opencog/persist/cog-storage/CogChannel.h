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

#include <mutex>
#include <string>

namespace opencog
{
/** \addtogroup grp_persist
 *  @{
 */

template<typename Client>
class CogChannel
{
	private:
		std::string _uri;

		// Socket API ... is single-threaded.
		std::mutex _mtx;
		int _sockfd;
		void do_send(const std::string&);
		std::string do_recv(void);

	public:
		CogChannel(void);
		CogChannel(const CogChannel&) = delete; // disable copying
		CogChannel& operator=(const CogChannel&) = delete; // disable assignment
		~CogChannel();

		void open_connection(const std::string& uri);
		bool connected(void); // connection to DB is alive

		void enqueue(Client*, const std::string&, void (Client::*)(const std::string&));
};

/** @}*/
} // namespace opencog

#endif // _OPENCOG_COG_CHANNEL_H

