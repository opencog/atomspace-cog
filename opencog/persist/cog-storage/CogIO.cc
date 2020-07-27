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

using namespace opencog;

void CogStorage::storeAtom(const Handle& h, bool synchronous)
{
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

void CogStorage::removeAtom(const Handle& h, bool recursive)
{
	std::string msg;
	if (recursive)
		msg = "(cog-extract-recursive! " + Sexpr::encode_atom(h) + ")\n";
	else
		msg = "(cog-extract! " + Sexpr::encode_atom(h) + ")\n";

	Pkt pkt;
	_io_queue.enqueue(this, msg, pkt, &CogStorage::noop_const);
}

void CogStorage::is_ok(const std::string& reply, Pkt& pkt)
{
	if (reply.compare(0, 2, "()"))
		pkt.table = (AtomTable *) -1;
}

Handle CogStorage::getNode(Type t, const char * str)
{
	std::string typena = nameserver().getTypeName(t) + " \"" + str + "\"";

	// Does the cogserver even know about this atom?
	Pkt pkt{nullptr, Handle::UNDEFINED};
	_io_queue.synchro(this,"(cog-node '" + typena + ")\n",
	                  pkt, &CogStorage::is_ok);
	if (nullptr == pkt.table)
		return Handle::UNDEFINED;

	// Yes, the cogserver knows about this atom
	Handle h = createNode(t, str);

	// Get all of the keys.
	std::string get_keys = "(cog-keys->alist (" + typena + "))\n";

	Pkt pkta{nullptr, h};
	_io_queue.synchro(this, get_keys, pkta, &CogStorage::decode_kvp_list);

	return h;
}

Handle CogStorage::getLink(Type t, const HandleSeq& hs)
{
	std::string typena = nameserver().getTypeName(t) + " ";
	for (const Handle& ho: hs)
		typena += Sexpr::encode_atom(ho);

	// Does the cogserver even know about this atom?
	Pkt pkt{nullptr, Handle::UNDEFINED};
	_io_queue.synchro(this,"(cog-link '" + typena + ")\n",
	                  pkt, &CogStorage::is_ok);
	if (nullptr == pkt.table)
		return Handle::UNDEFINED;

	// Yes, the cogserver knows about this atom
	Handle h = createLink(hs, t);

	// Get all of the keys.
	std::string get_keys = "(cog-keys->alist (" + typena + "))\n";
	Pkt pkta{nullptr, h};
	_io_queue.synchro(this, get_keys, pkta, &CogStorage::decode_kvp_list);

	return h;
}

void CogStorage::decode_atom_list(const std::string& expr, const Pkt& pkt)
{
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
		Handle h = pkt.table->add(Sexpr::decode_atom(expr, l, r, 0));

		// Get all of the keys.
		std::string get_keys = "(cog-keys->alist " + expr.substr(l, r-l+1) + ")\n";
		Pkt pkt{nullptr, h};
		_io_queue.enqueue(this, get_keys, pkt, &CogStorage::decode_kvp_list_const);

		// advance to next.
		l = r+1;
		r = end;
	}
}

void CogStorage::getIncomingSet(AtomTable& table, const Handle& h)
{
	std::string msg = "(cog-incoming-set " + Sexpr::encode_atom(h) + ")\n";

	Pkt pkt{&table, Handle::UNDEFINED};
	_io_queue.enqueue(this, msg, pkt, &CogStorage::decode_atom_list);
}

void CogStorage::getIncomingByType(AtomTable& table, const Handle& h, Type t)
{
	std::string msg = "(cog-incoming-by-type " + Sexpr::encode_atom(h)
		+ " '" + nameserver().getTypeName(t) + ")\n";

	Pkt pkt{&table, Handle::UNDEFINED};
	_io_queue.enqueue(this, msg, pkt, &CogStorage::decode_atom_list);
}

void CogStorage::loadAtomSpace(AtomTable &table)
{
	// Get nodes and links separately, in an effort to get
	// smaller replies.
	Pkt pkt{&table, Handle::UNDEFINED};
	std::string msg = "(cog-get-atoms 'Node #t)\n";
	_io_queue.enqueue(this, msg, pkt, &CogStorage::decode_atom_list);

	_io_queue.flush();
	msg = "(cog-get-atoms 'Link #t)\n";
	_io_queue.enqueue(this, msg, pkt, &CogStorage::decode_atom_list);

	_io_queue.flush();
}

void CogStorage::loadType(AtomTable &table, Type t)
{
	std::string msg = "(cog-get-atoms '" + nameserver().getTypeName(t) + ")\n";

	Pkt pkt{&table, Handle::UNDEFINED};
	_io_queue.enqueue(this, msg, pkt, &CogStorage::decode_atom_list);
}

void CogStorage::storeAtomSpace(const AtomTable &table)
{
	HandleSet all_atoms;
	table.getHandleSetByType(all_atoms, ATOM, true);
	for (const Handle& h : all_atoms)
		storeAtom(h);
	_io_queue.flush();
}

void CogStorage::kill_data(void)
{
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
	Sexpr::decode_alist(h, reply);
}
