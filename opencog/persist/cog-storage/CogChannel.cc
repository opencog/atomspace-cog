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

// Number of threads to run.
#define NTHREADS 4

template<typename Client, typename Data>
CogChannel<Client, Data>::CogChannel(void) :
	_servinfo(nullptr),
	_msg_buffer(this, &CogChannel::reply_handler, NTHREADS)
{
}

template<typename Client, typename Data>
CogChannel<Client, Data>::~CogChannel()
{
	close_connection();
	freeaddrinfo((struct addrinfo*) _servinfo);
}

/* ================================================================ */

template<typename Client, typename Data>
void CogChannel<Client, Data>::open_connection(const std::string& uri)
{
#define URIX_LEN (sizeof("cog://") - 1)  // Should be 6
	if (strncmp(uri.c_str(), "cog://", URIX_LEN))
		throw IOException(TRACE_INFO, "Unknown URI '%s'\n", uri.c_str());

	_uri = uri;

	// We expect the URI to be for the form
	//    cog://ipv4-addr/atomspace-name
	//    cog://ipv4-addr:port/atomspace-name
	//    cog://ipv4-addr/atomspace-name?stuff&more-stuff
	// except that there aren't any vaid 'stuffs' at this time.

	// Look for connection arguments
	size_t parg = _uri.find('?');
	if (_uri.npos != parg)
	{
		std::string args = _uri.substr(parg+1);
		_uri = _uri.substr(0, parg);

		size_t pamp = args.find('&');
		while (args.npos != pamp)
		{
			// _stuff.push_back(args.substr(0, pamp));
			args = args.substr(pamp+1);
			pamp = args.find('&');
		}
		// _stuff.push_back(args);
	}

	_host = uri.substr(URIX_LEN);
	size_t slash = _host.find_first_of(":/");
	if (std::string::npos != slash)
		_host = _host.substr(0, slash);

#define DEFAULT_COGSERVER_PORT "17001"
	_port = DEFAULT_COGSERVER_PORT;
	size_t colon = _uri.find(':', URIX_LEN + _host.length());
	if (std::string::npos != colon)
	{
		_port = _uri.substr(colon+1);
		slash = _port.find('/');
		if (std::string::npos != slash)
			_port = _port.substr(0, slash);
	}

	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	struct addrinfo *srvinfo;
	int rc = getaddrinfo(_host.c_str(), _port.c_str(), &hints, &srvinfo);
	if (rc)
		throw IOException(TRACE_INFO, "Unknown host %s: %s",
			_host.c_str(), strerror(rc));
	_servinfo = srvinfo;;

	// Try to open a connection, so that we find out immediately
	// if a cogserver is actually there. If not, clean up and throw.
	try {
		do_send(".\n.\n");
		do_recv(true);
		close(s._sockfd);
		s._sockfd = 0;
	}
	catch (const IOException& ex) {
		freeaddrinfo((struct addrinfo *) _servinfo);
		_servinfo = nullptr;
		if (0 != s._sockfd) close(s._sockfd);
		s._sockfd = 0;
		throw;
	}
	if (0 != s._sockfd) close(s._sockfd);
	s._sockfd = 0;

	// Make sure the buffer has some threads going.
	_msg_buffer.open(NTHREADS);
}

template<typename Client, typename Data>
int CogChannel<Client, Data>::open_sock()
{
	if (nullptr == _servinfo) return -1;

	struct addrinfo *srvinfo = (struct addrinfo *) _servinfo;
	int sockfd = socket(srvinfo->ai_family, srvinfo->ai_socktype, srvinfo->ai_protocol);

	if (0 > sockfd)
	{
		int norr = errno;
		throw IOException(TRACE_INFO, "Unable to create socket to host %s: %s",
			_host.c_str(), strerror(norr));
	}

	int rc = connect(sockfd, srvinfo->ai_addr, srvinfo->ai_addrlen);
	if (0 > rc)
	{
		int norr = errno;
		throw IOException(TRACE_INFO, "Unable to connect to host %s: %s",
			_host.c_str(), strerror(norr));
	}

	// We are going to be sending oceans of tiny packets,
	// and we want the fastest-possible responses.
	int flags = 1;
	rc = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags));
	if (0 > rc)
		fprintf(stderr, "Error setting sockopt: %s", strerror(errno));
#ifndef __APPLE__
	flags = 1;
	rc = setsockopt(sockfd, IPPROTO_TCP, TCP_QUICKACK, &flags, sizeof(flags));
	if (0 > rc)
		fprintf(stderr, "Error setting sockopt: %s", strerror(errno));
#endif

	// Get the s-expression shell.
	std::string eval = "sexpr\n";
	rc = send(sockfd, eval.c_str(), eval.size(), 0);
	if (0 > rc)
		throw IOException(TRACE_INFO,
			"Unable to talk to cogserver at host %s: %s",
			_host.c_str(), strerror(errno));

	// Throw away the cogserver prompt.
	s._sockfd = sockfd;
	do_recv(true);

	_nsocks++;
	return sockfd;
}

template<typename Client, typename Data>
bool CogChannel<Client, Data>::connected(void)
{
	return nullptr != _servinfo;
}

template<typename Client, typename Data>
void CogChannel<Client, Data>::close_connection(void)
{
	_msg_buffer.barrier();
	_msg_buffer.close();

	freeaddrinfo((struct addrinfo *) _servinfo);
	_servinfo = nullptr;
}

/* ================================================================== */

template<typename Client, typename Data>
std::atomic_int CogChannel<Client, Data>::_nsocks = 0;

template<typename Client, typename Data>
thread_local typename CogChannel<Client, Data>::tlso CogChannel<Client, Data>::s;

template<typename Client, typename Data>
void CogChannel<Client, Data>::do_send(const std::string& str)
{
	if (0 == s._sockfd) s._sockfd = open_sock();
	int rc = send(s._sockfd, str.c_str(), str.size(), MSG_NOSIGNAL);
	if (0 > rc)
		throw IOException(TRACE_INFO, "Unable to talk to cogserver: %s",
			strerror(errno));
}

// If the argument `garbage` is set to true, then assume that
// the first read contains the CogServer prompt, which is maybe
// colorized, and is, in any case, not newline teminated.
template<typename Client, typename Data>
std::string CogChannel<Client, Data>::do_recv(bool garbage)
{
	if (0 == s._sockfd)
		throw IOException(TRACE_INFO, "No open socket!");

	// The read strategy is as as folows:
	// messages are always terminated by a newline, with one exception:
	// Upon the initial connection to the CogServer, the server will
	// send it's default prompt. That prompt is not newline-terminated.
	// However, in that case, `garbage==true` and so we terminate the
	// read.
	std::string rb;
	bool first_time = true;
	while (true)
	{
		// Receive 8K bytes of message.
		char buf[8193];
		char* pbf = buf;
		int len = recv(s._sockfd, buf, 8192, 0);

		if (0 > len)
			throw IOException(TRACE_INFO, "Unable to talk to cogserver: %s",
				strerror(errno));
		if (0 == len)
		{
			close(s._sockfd);
			throw IOException(TRACE_INFO, "Cogserver unexpectedly closed connection");
		}
		buf[len] = 0;

		// Ignore solitary synchronous idle chars.
		// The CogServer sends these when it is congested
		// and is looking for half-open sockets.
		if (0x16 == buf[0])
		{
			pbf = &buf[1];
			while (0x16 == pbf[0] and (pbf-buf < len)) pbf++;
			if (pbf-buf == len) continue;
		}

		// Unlikely to get one at the end, but not impossible.
		if (0x16 == buf[len-1])
		{
			len--; buf[len] = 0;
			while (0x16 == buf[len-1] and 0 < len)
			{
				len--; buf[len] = 0;
			}
			if (0 == len) continue;
		}

		// If we have a short read, assume we are done.
		// Normal short reads are either newline-terminated,
		// or are reads of the cogserver prompt, which are
		// blank-space terminated.
		if (first_time and (('\n' == buf[len-1]) or garbage))
			return pbf;

		first_time = false;
		rb += pbf;

		// newline-terminated strings mean we are done.
		if ('\n' == buf[len-1])
			return rb;
	}
	return rb;
}

/* ================================================================== */

template<typename Client, typename Data>
void CogChannel<Client, Data>::synchro(Client* client,
                                       const std::string& msg,
                                       Data& data,
                  void (Client::*handler)(const std::string&, Data&))
{
	do_send(msg);
	std::string reply = do_recv();

	// Client is called unlocked.
	(client->*handler)(reply, data);
}

/* ================================================================== */

// Place the message int to queue
template<typename Client, typename Data>
void CogChannel<Client, Data>::enqueue(Client* client,
                                       const std::string& msg,
                                       Data& data,
                  void (Client::*handler)(const std::string&, const Data&))
{
	Msg block{msg, data, client, handler};
	_msg_buffer.insert(block);
}

// Run message from queue.
template<typename Client, typename Data>
void CogChannel<Client, Data>::reply_handler(const Msg& msg)
{
	do_send(msg.str_to_send);
	std::string reply = do_recv();

	// Client is called unlocked.
	(msg.client->*msg.callback)(reply, msg.data);
}

template<typename Client, typename Data>
void CogChannel<Client, Data>::barrier()
{
	_msg_buffer.barrier();
}

template<typename Client, typename Data>
void CogChannel<Client, Data>::flush()
{
	_msg_buffer.flush();
}

template<typename Client, typename Data>
void CogChannel<Client, Data>::clear_stats()
{
	_msg_buffer.clear_stats();
}

template<typename Client, typename Data>
std::string CogChannel<Client, Data>::print_stats()
{
	std::string rs =
		"Open socks: " + std::to_string(_nsocks.load()) +
		"  Connected to: " + _uri +
		"\n" +
		"Queue size: " + std::to_string(_msg_buffer.get_size()) +
		"  Busy: " + std::to_string(_msg_buffer.get_busy_writers()) +
		"/" + std::to_string(NTHREADS) +
		(_msg_buffer._in_drain ? " Draining now" : "") +
		"\n" +
		"Messages: " + std::to_string(_msg_buffer._item_count) +
		"  Duplicates: " + std::to_string(_msg_buffer._duplicate_count) +
		"\n" +
		"Flush count: " + std::to_string(_msg_buffer._flush_count) +
		"  Drains: " + std::to_string(_msg_buffer._drain_count) +
		"\n" +
		"Drain time (msec): " + std::to_string(_msg_buffer._drain_msec) +
		"  Slowest (msec): " + std::to_string(_msg_buffer._drain_count) +
		"  Concurrent: " + std::to_string(_msg_buffer._drain_concurrent) +
		"\n" +
		"Low/High watermarks: " +
		std::to_string(_msg_buffer.get_low_watermark()) +
		"/" +
		std::to_string(_msg_buffer.get_high_watermark()) +
		"  Stalled: " + (_msg_buffer.stalling() ? "true" : "false") +
		"\n";

	return rs;
}

/* ============================= END OF FILE ================= */
