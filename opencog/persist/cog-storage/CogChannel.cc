/*
 * FILE:
 * opencog/persist/cog-storage/CogChannel.cc
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

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>

#include <opencog/util/exceptions.h>
#include <opencog/persist/cog-storage/CogChannel.h>

using namespace opencog;

/* ================================================================ */
// Constructors

template<typename Client>
void CogChannel<Client>::open_connection(const std::string& uri)
{
#define URIX_LEN (sizeof("cog://") - 1)  // Should be 6
	if (strncmp(uri.c_str(), "cog://", URIX_LEN))
		throw IOException(TRACE_INFO, "Unknown URI '%s'\n", uri);

	std::lock_guard<std::mutex> lck(_mtx);
	_uri = uri;

	// We expect the URI to be for the form
	//    cog://ipv4-addr/atomspace-name
	//    cog://ipv4-addr:port/atomspace-name

	std::string host(uri.substr(URIX_LEN));
	size_t slash = host.find_first_of(":/");
	if (std::string::npos != slash)
		host = host.substr(0, slash);

#define DEFAULT_COGSERVER_PORT "17001"
	std::string port = DEFAULT_COGSERVER_PORT;
	size_t colon = _uri.find(':', URIX_LEN + host.length());
	if (std::string::npos != colon)
	{
		port = _uri.substr(colon+1);
		slash = port.find('/');
		if (std::string::npos != slash)
			port = port.substr(0, slash);
	}

	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	struct addrinfo *servinfo;
	int rc = getaddrinfo(host.c_str(), port.c_str(), &hints, &servinfo);
	if (rc)
		throw IOException(TRACE_INFO, "Unknown host %s: %s",
			host.c_str(), strerror(rc));

	_sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

	if (0 > _sockfd)
	{
		int norr = errno;
		free(servinfo);
		throw IOException(TRACE_INFO, "Unable to create socket to host %s: %s",
			host.c_str(), strerror(norr));
	}

	rc = connect(_sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
	if (0 > rc)
	{
		int norr = errno;
		free(servinfo);
		throw IOException(TRACE_INFO, "Unable to connect to host %s: %s",
			host.c_str(), strerror(norr));
	}
	free(servinfo);
	if (0 > rc)
		fprintf(stderr, "Error setting sockopt: %s", strerror(errno));

	// We are going to be sending oceans of tiny packets,
	// and we want the fastest-possible responses.
	int flags = 1;
	rc = setsockopt(_sockfd, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags));
	flags = 1;
	rc = setsockopt(_sockfd, IPPROTO_TCP, TCP_QUICKACK, &flags, sizeof(flags));

	// Get to the scheme prompt, but make it be silent.
	std::string eval = "scm hush\n";
	rc = send(_sockfd, eval.c_str(), eval.size(), 0);
	if (0 > rc)
		throw IOException(TRACE_INFO, "Unable to talk to cogserver at host %s: %s",
			host.c_str(), strerror(errno));

	// Throw away the cogserver prompt.
	do_recv();

	do_send("(cog-set-server-mode! #t)\n");
	do_recv();
}

template<typename Client>
CogChannel<Client>::CogChannel(void)
	: _sockfd(-1)
{
}

template<typename Client>
CogChannel<Client>::~CogChannel()
{
	if (connected())
		close(_sockfd);
}

template<typename Client>
bool CogChannel<Client>::connected(void)
{
	return 0 < _sockfd;
}

/* ================================================================== */

template<typename Client>
void CogChannel<Client>::enqueue(Client* client,
                                 const std::string& msg,
                                 void (Client::*handler)(const std::string&))
{
	std::lock_guard<std::mutex> lck(_mtx);
	do_send(msg);
	std::string reply = do_recv();
	client->handler(reply);
}

template<typename Client>
void CogChannel<Client>::do_send(const std::string& str)
{
	if (not connected())
		throw IOException(TRACE_INFO, "Not connected to cogserver!");

	int rc = send(_sockfd, str.c_str(), str.size(), 0);
	if (0 > rc)
		throw IOException(TRACE_INFO, "Unable to talk to cogserver: %s",
			strerror(errno));
}

template<typename Client>
std::string CogChannel<Client>::do_recv()
{
	if (not connected())
		throw IOException(TRACE_INFO, "Not connected to cogserver!");

	// XXX FIXME the strategy below is rather fragile.
	// I don't think its trustworthy for production use,
	// but I guess its OK for proof-of-concept.
	std::string rb;
	bool first_time = true;
	while (true)
	{
		// Receive 4K bytes of message.
		char buf[4097];
		int len = recv(_sockfd, buf, 4096, 0);

		if (0 > len)
			throw IOException(TRACE_INFO, "Unable to talk to cogserver: %s",
				strerror(errno));
		if (0 == len)
		{
			close(_sockfd);
			_sockfd = 0;
			throw IOException(TRACE_INFO, "Cogserver unexpectedly closed connection");
		}
		buf[len] = 0;

		// If we have a short read, assume we are done.
		if (first_time and len < 4096)
			return buf;

		first_time = false;
		if (len < 4096)
		{
			rb += buf;
			return rb;
		}

		// If we have a long read, assume that there's more.
		// XXXX FIXME this fails, of course, for buffers of
		// exactly 4096...
		rb += buf;
	}
	return rb;
}

/* ============================= END OF FILE ================= */
