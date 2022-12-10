/*
 * CogIO.cc
 * Save/restore of individual atoms.
 *
 * Copyright (c) 2020 Linas Vepstas <linas@linas.org>
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

#include <opencog/atoms/base/Atom.h>
#include <opencog/atoms/base/Node.h>
#include <opencog/atoms/base/Link.h>
#include <opencog/atomspace/AtomSpace.h>
#include <opencog/persist/sexpr/Sexpr.h>

#include "CogStorage.h"
#include "CogChannel.cc"

#define CLASSNAME CogStorage
#include "../cog-common/ListUtils.cc"
#undef CLASSNAME


using namespace opencog;

#define CHECK_OPEN \
	if (not connected()) \
		throw IOException(TRACE_INFO, "CogStorageNode is not open!  '%s'\n", \
			_name.c_str());

void CogStorage::proxy_open(void)
{
	Pkt pkt;
	_io_queue.enqueue(this, "(cog-proxy-open)\n", pkt, &CogStorage::noop_const);
	_io_queue.barrier();
}

void CogStorage::proxy_close(void)
{
	Pkt pkt;
	_io_queue.enqueue(this, "(cog-proxy-close)\n", pkt, &CogStorage::noop_const);
	_io_queue.barrier();
}

void CogStorage::set_proxy(const Handle& h)
{
	CHECK_OPEN;

	// Some proxies require that the CogServer know about the
	// keys on the proxy. Thus, on behalf of the user, we send
	// those keys and values. (The user could do this themselves
	// but they (i.e. me) forget.)
	storeAtom(h);
	barrier();

	std::string msg =
		"(cog-set-proxy! " + Sexpr::encode_atom(h) + ")\n";

	Pkt pkt;
	_io_queue.enqueue(this, msg, pkt, &CogStorage::noop_const);
	_io_queue.barrier();
}

void CogStorage::storeAtom(const Handle& h, bool synchronous)
{
	CHECK_OPEN;
	// If there are no values, be sure to reset the TV to the default TV.
	std::string msg;
	if (h->haveValues())
		msg = "(cog-set-values! " + Sexpr::encode_atom(h) +
			Sexpr::encode_atom_values(h) + ")\n";
	else
		msg = "(cog-set-tv! " + Sexpr::encode_atom(h) + " (stv 1 0))\n";

	Pkt pkt;
	_io_queue.enqueue(this, msg, pkt, &CogStorage::noop_const);
}

void CogStorage::removeAtom(AtomSpace* frame, const Handle& h, bool recursive)
{
	CHECK_OPEN;
	std::string msg;
	if (recursive)
		msg = "(cog-extract-recursive! " + Sexpr::encode_atom(h) + ")\n";
	else
		msg = "(cog-extract! " + Sexpr::encode_atom(h) + ")\n";

	Pkt pkt;
	_io_queue.enqueue(this, msg, pkt, &CogStorage::noop_const);
}

void CogStorage::storeValue(const Handle& h, const Handle& key)
{
	CHECK_OPEN;
	std::string msg;
	msg = "(cog-set-value! " + Sexpr::encode_atom(h) +
	      Sexpr::encode_atom(key) +
	      Sexpr::encode_value(h->getValue(key)) + ")\n";

	Pkt pkt;
	_io_queue.enqueue(this, msg, pkt, &CogStorage::noop_const);
}

void CogStorage::updateValue(const Handle& h, const Handle& key,
                             const ValuePtr& delta)
{
	CHECK_OPEN;
	std::string msg;
	msg = "(cog-update-value! " + Sexpr::encode_atom(h) +
	      Sexpr::encode_atom(key) +
	      Sexpr::encode_value(delta) + ")\n";

	Pkt pkt;
	_io_queue.enqueue(this, msg, pkt, &CogStorage::noop_const);
}

void CogStorage::loadValue(const Handle& h, const Handle& key)
{
	CHECK_OPEN;
	std::string msg;
	msg = "(cog-value " + Sexpr::encode_atom(h) +
	      Sexpr::encode_atom(key) + ")\n";

	Pkt pkta{nullptr, h, key};
	_io_queue.enqueue(this, msg, pkta, &CogStorage::decode_value);
}

void CogStorage::decode_value(const std::string& reply, const Pkt& pkt)
{
	size_t pos = 0;
	ValuePtr vp = Sexpr::decode_value(reply, pos);

	// If the Value has Atoms inside of it, make sure they
	// live in an AtomSpace.
	AtomSpace* as = pkt.h->getAtomSpace();
	if (as)
		vp = Sexpr::add_atoms(as, vp);

	pkt.h->setValue(pkt.key, vp);
}

void CogStorage::is_ok(const std::string& reply, Pkt& pkt)
{
	if (reply.compare(0, 2, "()"))
		pkt.table = (AtomSpace *) -1;
}

void CogStorage::getAtom(const Handle& h)
{
	CHECK_OPEN;
	std::string typena = nameserver().getTypeName(h->get_type()) + " ";
	std::string iknow;
	if (h->is_node())
	{
		typena += "\"" + h->get_name() + "\"";
		iknow = "(cog-node '" + typena + ")\n";
	}
	else
	{
		for (const Handle& ho: h->getOutgoingSet())
			typena += Sexpr::encode_atom(ho);
		iknow = "(cog-link '" + typena + ")\n";
	}

	// Does the cogserver even know about this atom?
	Pkt pkt;
	_io_queue.synchro(this, iknow, pkt, &CogStorage::is_ok);
	if (nullptr == pkt.table) return;

	// Yes, the cogserver knows about this atom
	// Get all of the keys.
	std::string get_keys = "(cog-keys->alist (" + typena + "))\n";

	Pkt pkta{nullptr, h, Handle::UNDEFINED};
	// _io_queue.synchro(this, get_keys, pkta, &CogStorage::decode_kvp_list);
	_io_queue.enqueue(this, get_keys, pkta, &CogStorage::decode_kvp_list_const);
}

void CogStorage::decode_atom_list(const std::string& expr, const Pkt& pkt)
{
static std::unordered_map<std::string, Handle> _fid_map; // tmp placeholder
	// Loop and decode atoms.
	size_t l = expr.find('(') + 1; // skip the first paren.
	size_t end = expr.rfind(')');  // trim tailing paren.
	size_t r = end;
	if (l == r) return;
	while (true)
	{
		// get_next_expr() updates the l and r to bracket an expression.
		int pcnt = Sexpr::get_next_expr(expr, l, r, 0);
		if (l == r) break;
		if (0 < pcnt) break;
		Handle h = add_nocheck(pkt.table, Sexpr::decode_atom(expr, l, r, 0, _fid_map));

		// Get all of the keys.
		std::string get_keys = "(cog-keys->alist " + expr.substr(l, r-l+1) + ")\n";
		Pkt pkt{nullptr, h, Handle::UNDEFINED};
		_io_queue.enqueue(this, get_keys, pkt, &CogStorage::decode_kvp_list_const);

		// advance to next.
		l = r+1;
		r = end;
	}
}

void CogStorage::fetchIncomingSet(AtomSpace* table, const Handle& h)
{
	CHECK_OPEN;
	std::string msg = "(cog-incoming-set " + Sexpr::encode_atom(h) + ")\n";

	Pkt pkt{table, Handle::UNDEFINED, Handle::UNDEFINED,};
	_io_queue.enqueue(this, msg, pkt, &CogStorage::decode_atom_list);
}

void CogStorage::fetchIncomingByType(AtomSpace* table, const Handle& h, Type t)
{
	CHECK_OPEN;
	std::string msg = "(cog-incoming-by-type " + Sexpr::encode_atom(h)
		+ " '" + nameserver().getTypeName(t) + ")\n";

	Pkt pkt{table, Handle::UNDEFINED, Handle::UNDEFINED,};
	_io_queue.enqueue(this, msg, pkt, &CogStorage::decode_atom_list);
}

// FYI: Of the four sockts open to the cogserver, one of them will
// handle the `cog-get-atoms` command, and will transfer not very
// much data. The other three will transfer huge amounts of data,
// basically fetching the keys and values on each atom. So looking
// at the server stats will make it look like one socket is bare
// touched ... which is correct: its cause of this.
void CogStorage::loadAtomSpace(AtomSpace* table)
{
	CHECK_OPEN;
	// Get nodes and links separately, in an effort to get
	// smaller replies.
	Pkt pkt{table, Handle::UNDEFINED, Handle::UNDEFINED};
	std::string msg = "(cog-get-atoms 'Node #t)\n";
	_io_queue.enqueue(this, msg, pkt, &CogStorage::decode_atom_list);

	_io_queue.flush();
	msg = "(cog-get-atoms 'Link #t)\n";
	_io_queue.enqueue(this, msg, pkt, &CogStorage::decode_atom_list);

	_io_queue.barrier();
}

// See note on loadAtomSpace(), immediately above.
void CogStorage::loadType(AtomSpace* table, Type t)
{
	CHECK_OPEN;
	std::string msg = "(cog-get-atoms '" + nameserver().getTypeName(t) + ")\n";

	Pkt pkt{table, Handle::UNDEFINED, Handle::UNDEFINED,};
	_io_queue.enqueue(this, msg, pkt, &CogStorage::decode_atom_list);
}

void CogStorage::storeAtomSpace(const AtomSpace* table)
{
	CHECK_OPEN;
	HandleSeq all_atoms;
	table->get_handles_by_type(all_atoms, ATOM, true);
	for (const Handle& h : all_atoms)
		storeAtom(h);
	_io_queue.barrier();
}

void CogStorage::kill_data(void)
{
	CHECK_OPEN;
	_io_queue.barrier();
	Pkt pkt;
	_io_queue.synchro(this, "(cog-atomspace-clear)\n",
		pkt, &CogStorage::noop);
}

/// Decode a key-value-pair association list.
/// attach the key-value pairs to the atom in the packet.
void CogStorage::decode_kvp_list_const(const std::string& reply, const Pkt& pkt)
{
	Handle h = pkt.h;
	// Sexpr::decode_alist(h, reply);
	ro_decode_alist(pkt.table, h, reply);
}

void CogStorage::runQuery(const Handle& query, const Handle& key,
                          const Handle& meta, bool fresh)
{
	CHECK_OPEN;
	std::string msg = "(cog-execute-cache! " +
		Sexpr::encode_atom(query) +
		Sexpr::encode_atom(key);

	if (meta)
	{
		msg += Sexpr::encode_atom(meta);
		if (fresh) msg += " #t";
	}
	msg += ")\n";

	Pkt pkta{nullptr, query, key};
	_io_queue.enqueue(this, msg, pkta, &CogStorage::decode_value);
}
