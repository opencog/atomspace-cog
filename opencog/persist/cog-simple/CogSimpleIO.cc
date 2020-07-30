/*
 * CogSimpleIO.cc
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

#include "CogSimpleStorage.h"

using namespace opencog;

// Design note: each of the calls below grabs a lock to protect the
// send-recv pair.  This is so that mutiple threads sharing the same
// socket do not accidentally get confused about whose data is whose.
// If you want faster throughput, then open multiple sockets to the
// cogserver, it can handle that just fine.

void CogSimpleStorage::storeAtom(const Handle& h, bool synchronous)
{
	// If there are no values, be sure to reset the TV to the default TV.
	std::string msg;
	if (h->haveValues())
		msg = "(cog-set-values! " + Sexpr::encode_atom(h) +
			Sexpr::encode_atom_values(h) + ")\n";
	else
		msg = "(cog-set-tv! " + Sexpr::encode_atom(h) + " (stv 1 0))\n";

	std::lock_guard<std::mutex> lck(_mtx);
	do_send(msg);

	// Flush the response.
	do_recv();
}

void CogSimpleStorage::storeValue(const Handle& h, const Handle& key)
{
	std::string msg;
	msg = "(cog-set-value! " + Sexpr::encode_atom(h) +
	      Sexpr::encode_atom(key) +
	      Sexpr::encode_value(h->getValue(key)) + ")\n";

	std::lock_guard<std::mutex> lck(_mtx);
	do_send(msg);

	// Flush the response.
	do_recv();
}

void CogSimpleStorage::loadValue(const Handle& h, const Handle& key)
{
	std::string msg;
	msg = "(cog-value " + Sexpr::encode_atom(h) +
	      Sexpr::encode_atom(key) + ")\n";

	std::lock_guard<std::mutex> lck(_mtx);
	do_send(msg);

	std::string rply = do_recv();
	size_t pos = 0;
	ValuePtr vp = Sexpr::decode_value(rply, pos);
	h->setValue(key, vp);
}

void CogSimpleStorage::removeAtom(const Handle& h, bool recursive)
{
	std::string msg;
	if (recursive)
		msg = "(cog-extract-recursive! " + Sexpr::encode_atom(h) + ")\n";
	else
		msg = "(cog-extract! " + Sexpr::encode_atom(h) + ")\n";

	std::lock_guard<std::mutex> lck(_mtx);
	do_send(msg);

	// Flush the response.
	do_recv();
}

Handle CogSimpleStorage::getNode(Type t, const char * str)
{
	std::string typena = nameserver().getTypeName(t) + " \"" + str + "\"";

	// Does the cogserver even know about this atom?
	std::lock_guard<std::mutex> lck(_mtx);
	do_send("(cog-node '" + typena + ")\n");
	std::string msg = do_recv();
	if (0 == msg.compare(0, 2, "()"))
		return Handle();

	// Yes, the cogserver knows about this atom
	Handle h = createNode(t, str);

	// Get all of the keys.
	std::string get_keys = "(cog-keys->alist (" + typena + "))\n";
	do_send(get_keys);
	msg = do_recv();
	Sexpr::decode_alist(h, msg);

	return h;
}

Handle CogSimpleStorage::getLink(Type t, const HandleSeq& hs)
{
	std::string typena = nameserver().getTypeName(t) + " ";
	for (const Handle& ho: hs)
		typena += Sexpr::encode_atom(ho);

	// Does the cogserver even know about this atom?
	std::lock_guard<std::mutex> lck(_mtx);
	do_send("(cog-link '" + typena + ")\n");
	std::string msg = do_recv();
	if (0 == msg.compare(0, 2, "()"))
		return Handle();

	// Yes, the cogserver knows about this atom
	Handle h = createLink(hs, t);

	// Get all of the keys.
	std::string get_keys = "(cog-keys->alist (" + typena + "))\n";
	do_send(get_keys);
	msg = do_recv();
	Sexpr::decode_alist(h, msg);

	return h;
}

void CogSimpleStorage::decode_atom_list(AtomTable& table)
{
	// XXX FIXME .. this WILL fail if the returned list is large.
	// Basically, we don't know quite when all the bytes have been
	// received on the socket... For now, we punt.
	std::string expr = do_recv();

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
		Handle h = table.add(Sexpr::decode_atom(expr, l, r, 0));

		// Get all of the keys.
		std::string get_keys = "(cog-keys->alist " + expr.substr(l, r-l+1) + ")\n";
		do_send(get_keys);
		std::string msg = do_recv();
		Sexpr::decode_alist(h, msg);

		// advance to next.
		l = r+1;
		r = end;
	}
}

void CogSimpleStorage::getIncomingSet(AtomTable& table, const Handle& h)
{
	std::string atom = "(cog-incoming-set " + Sexpr::encode_atom(h) + ")\n";
	std::lock_guard<std::mutex> lck(_mtx);
	do_send(atom);
	decode_atom_list(table);
}

void CogSimpleStorage::getIncomingByType(AtomTable& table, const Handle& h, Type t)
{
	std::string msg = "(cog-incoming-by-type " + Sexpr::encode_atom(h)
		+ " '" + nameserver().getTypeName(t) + ")\n";
	std::lock_guard<std::mutex> lck(_mtx);
	do_send(msg);
	decode_atom_list(table);
}

void CogSimpleStorage::loadAtomSpace(AtomTable &table)
{
	std::lock_guard<std::mutex> lck(_mtx);

	// Get nodes and links separately, in an effort to get
	// smaller replies.
	std::string msg = "(cog-get-atoms 'Node #t)\n";
	do_send(msg);
	decode_atom_list(table);

	msg = "(cog-get-atoms 'Link #t)\n";
	do_send(msg);
	decode_atom_list(table);
}

void CogSimpleStorage::loadType(AtomTable &table, Type t)
{
	std::string msg = "(cog-get-atoms '" + nameserver().getTypeName(t) + ")\n";

	std::lock_guard<std::mutex> lck(_mtx);
	do_send(msg);
	decode_atom_list(table);
}

void CogSimpleStorage::storeAtomSpace(const AtomTable &table)
{
	HandleSet all_atoms;
	table.getHandleSetByType(all_atoms, ATOM, true);
	for (const Handle& h : all_atoms)
		storeAtom(h);
}

void CogSimpleStorage::kill_data(void)
{
	std::lock_guard<std::mutex> lck(_mtx);
	do_send("(cog-atomspace-clear)\n");
	do_recv();
}

void CogSimpleStorage::runQuery(const Handle& query, const Handle& key,
                                const Handle& meta, bool fresh)
{
	std::string msg = "(cog-execute-cache! " +
		Sexpr::encode_atom(query) +
		Sexpr::encode_atom(key);

	if (meta)
	{
		msg += Sexpr::encode_atom(meta);

		if (fresh) msg += " #t";
	}
	msg += ")\n";

	std::string rply;
	{
		std::lock_guard<std::mutex> lck(_mtx);
		do_send(msg);
		rply = do_recv();
	}

	size_t pos = 0;
	ValuePtr vp = Sexpr::decode_value(rply, pos);
	query->setValue(key, vp);
}
