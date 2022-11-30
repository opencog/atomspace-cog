/*
 * FILE:
 * opencog/persist/cog-storage/CogStorage.cc
 *
 * FUNCTION:
 * Simple CogServer-backed persistent storage.
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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <errno.h>

#include "CogStorage.h"
#include "CogChannel.cc"

using namespace opencog;

/// This is a basic atomspace client built on the cogserver.

/* ================================================================ */
// Constructors

void CogStorage::init(const char * uri)
{
#define URIX_LEN (sizeof("cog://") - 1)  // Should be 6
	if (strncmp(uri, "cog://", URIX_LEN))
		throw IOException(TRACE_INFO, "Unknown URI '%s'\n", uri);

	_uri = uri;

	// Look for connection arguments
	size_t parg = _uri.find('?');
	if (_uri.npos != parg)
	{
		std::string args = _uri.substr(parg+1);
		size_t pamp = args.find('&');
		while (args.npos != pamp)
		{
			std::string pcfg = args.substr(0, pamp);

			// Verify acceptable formats. (There are none at this time.)
			throw IOException(TRACE_INFO,
				"Unknown configuration %s", pcfg.c_str());

			args = args.substr(pamp+1);
			pamp = args.find('&');
		}

		// Check the last one too.
		std::string pcfg = args;
		throw IOException(TRACE_INFO,
			"Unknown configuration %s", pcfg.c_str());
	}
}

CogStorage::CogStorage(std::string uri) :
	StorageNode(COG_STORAGE_NODE, std::move(uri))
{
	init(_name.c_str());
}

CogStorage::~CogStorage()
{
	close();
}

void CogStorage::open(void)
{
	if (connected()) return;
	_io_queue.open_connection(_uri.c_str());
}

bool CogStorage::connected(void)
{
	return _io_queue.connected();
}

void CogStorage::close(void)
{
	if (not connected()) return;

	proxy_close();
	_io_queue.barrier();
	_io_queue.close_connection();
}

/* ================================================================== */
/// Drain the pending store queue. This is a fencing operation; the
/// goal is to make sure that all writes that occurred before the
/// barrier really are performed before before all the writes after
/// the barrier.
///
void CogStorage::barrier(AtomSpace* as)
{
	_io_queue.barrier();
}

/* ================================================================ */

std::string CogStorage::monitor(void)
{
	// _io_queue.clear_stats();
	return "CogStorageNode I/O Queue Stats:\n" +
		_io_queue.print_stats();
}

DEFINE_NODE_FACTORY(CogStorageNode, COG_STORAGE_NODE)

/* ============================= END OF FILE ================= */
