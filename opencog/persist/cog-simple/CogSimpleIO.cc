/*
 * CogSimpleIO.cc
 * Save/restore of individual atoms.
 *
 * Copyright (c) 2020,2022 Linas Vepstas <linas@linas.org>
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

#define CLASSNAME CogSimpleStorage
#include "../cog-common/ListUtils.cc"
#undef CLASSNAME

using namespace opencog;

// Design note: each of the calls below grabs a lock to protect the
// send-recv pair.  This is so that mutiple threads sharing the same
// socket do not accidentally get confused about whose data is whose.
// If you want faster throughput, then open multiple sockets to the
// cogserver, it can handle that just fine.

void CogSimpleStorage::set_proxy(const Handle& h)
{
	// Some proxies require that the CogServer know about the
	// keys on the proxy. Thus, on behalf of the user, we send
	// those keys and values. (The user could do this themselves
	// but they (i.e. me) forget.)
	storeAtom(h);

	std::string msg =
		"(cog-set-proxy! " + Sexpr::encode_atom(h, false) + ")\n";

	std::lock_guard<std::mutex> lck(_mtx);
	do_send(msg);

	// Flush the response.
	do_recv();
}

void CogSimpleStorage::storeAtom(const Handle& h, bool synchronous)
{
	// Are there multiple AtomSpaces involved?
	if (not _multi_space
	    and nullptr != _atom_space
	    and h->getAtomSpace() != _atom_space)
		_multi_space = true;

	if (_multi_space) writeFrame(h->getAtomSpace());

	// If there are no values, be sure to reset the TV to the default TV.
	std::string msg;
	if (h->haveValues())
		msg = "(cog-set-values! " + Sexpr::encode_atom(h, _multi_space) +
			Sexpr::encode_atom_values(h) + ")\n";
	else
		msg = "(cog-set-tv! " + Sexpr::encode_atom(h, _multi_space) + " (stv 1 0))\n";

	std::lock_guard<std::mutex> lck(_mtx);
	do_send(msg);

	// Flush the response.
	do_recv();
}

void CogSimpleStorage::storeValue(const Handle& h, const Handle& key)
{
	// Are there multiple AtomSpaces involved?
	if (not _multi_space and h->getAtomSpace() != _atom_space)
		_multi_space = true;

	if (_multi_space) writeFrame(h->getAtomSpace());

	std::string msg;
	msg = "(cog-set-value! " + Sexpr::encode_atom(h) +
	      Sexpr::encode_atom(key, _multi_space) +
	      Sexpr::encode_value(h->getValue(key)) + ")\n";

	std::lock_guard<std::mutex> lck(_mtx);
	do_send(msg);

	// Flush the response.
	do_recv();
}

void CogSimpleStorage::updateValue(const Handle& h, const Handle& key,
                                   const ValuePtr& delta)
{
	// Are there multiple AtomSpaces involved?
	if (not _multi_space and h->getAtomSpace() != _atom_space)
		_multi_space = true;

	if (_multi_space) writeFrame(h->getAtomSpace());

	std::string msg;
	msg = "(cog-update-value! " + Sexpr::encode_atom(h) +
	      Sexpr::encode_atom(key, _multi_space) +
	      Sexpr::encode_value(delta) + ")\n";

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

	// If the Value has Atoms inside of it, make sure they
	// live in an AtomSpace.
	AtomSpace* as = h->getAtomSpace();
	if (as)
		vp = Sexpr::add_atoms(as, vp);
	h->setValue(key, vp);
}

void CogSimpleStorage::removeAtom(AtomSpace* frame, const Handle& h, bool recursive)
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

void CogSimpleStorage::getAtom(const Handle& h)
{
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
	std::lock_guard<std::mutex> lck(_mtx);
	do_send(iknow);
	std::string msg = do_recv();
	if (0 == msg.compare(0, 2, "()")) return;

	// Yes, the cogserver knows about this atom
	// Get all of the keys.
	std::string get_keys = "(cog-keys->alist (" + typena + "))\n";
	do_send(get_keys);
	msg = do_recv();
	// Sexpr::decode_alist(h, msg);
	ro_decode_alist(_atom_space, h, msg);
}

void CogSimpleStorage::decode_atom_list(AtomSpace* table)
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
		Handle h = Sexpr::decode_atom(expr, l, r, 0, _fid_map);
		if (nullptr == h->getAtomSpace())
			h = add_nocheck(table, h);

		// Get all of the keys.
		std::string get_keys = "(cog-keys->alist " + expr.substr(l, r-l+1) + ")\n";
		do_send(get_keys);
		std::string msg = do_recv();
		// Sexpr::decode_alist(h, msg);
		ro_decode_alist(table, h, msg);

		// advance to next.
		l = r+1;
		r = end;
	}
}

void CogSimpleStorage::fetchIncomingSet(AtomSpace* table, const Handle& h)
{
	std::string atom = "(cog-incoming-set " + Sexpr::encode_atom(h) + ")\n";
	std::lock_guard<std::mutex> lck(_mtx);
	do_send(atom);
	decode_atom_list(table);
}

void CogSimpleStorage::fetchIncomingByType(AtomSpace* table, const Handle& h, Type t)
{
	std::string msg = "(cog-incoming-by-type " + Sexpr::encode_atom(h)
		+ " '" + nameserver().getTypeName(t) + ")\n";
	std::lock_guard<std::mutex> lck(_mtx);
	do_send(msg);
	decode_atom_list(table);
}

void CogSimpleStorage::loadAtomSpace(AtomSpace* table)
{
	// If there's a hierarchy of frames, get those first.
	loadFrameDAG();

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

void CogSimpleStorage::loadType(AtomSpace* table, Type t)
{
	std::string msg = "(cog-get-atoms '" + nameserver().getTypeName(t) + ")\n";

	std::lock_guard<std::mutex> lck(_mtx);
	do_send(msg);
	decode_atom_list(table);
}

void CogSimpleStorage::storeAtomSpace(const AtomSpace* table)
{
	HandleSeq all_atoms;
	table->get_handles_by_type(all_atoms, ATOM, true);
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

	// If the Value has Atoms inside of it, make sure they
	// live in an AtomSpace.
	AtomSpace* as = query->getAtomSpace();
	if (as)
		vp = Sexpr::add_atoms(as, vp);

	query->setValue(key, vp);
}

// ===================================================================
// Frame-related stuffs

/// Store short atomspace names in the cache.
void CogSimpleStorage::cacheFrame(const Handle& hasp)
{
	// Recurse first
	for (const Handle& ho : hasp->getOutgoingSet())
		cacheFrame(ho);

	const std::string& name = hasp->get_name();
	std::string shorty = "(AtomSpace \"" + name + "\")";
	std::lock_guard<std::mutex> flck(_mtx_frame);
	_frame_map.insert({hasp, shorty});
	_fid_map.insert({name, hasp});
}

std::string CogSimpleStorage::writeFrame(const Handle& hasp)
{
	// Keep a map. This will be faster than doing string conversion
	// each time. We expect this to be small, no larger than a few
	// thousand entries, and so don't expect it to compete for RAM.
	{
		std::lock_guard<std::mutex> flck(_mtx_frame);
		auto it = _frame_map.find(hasp);
		if (it != _frame_map.end())
			return it->second;
	}

	_multi_space = true;

	std::string msg = "(define *-bogus-top-space-* "
		+ Sexpr::encode_frame(hasp) + ")\n";
	{
		std::lock_guard<std::mutex> lck(_mtx);
		do_send(msg);

		// Flush the response.
		do_recv();
	}

	// Store short-forms
	cacheFrame(hasp);

	// Return short-form
	std::string shorty = "(AtomSpace \"" + hasp->get_name() + "\")";
	return shorty;
}

void CogSimpleStorage::storeFrameDAG(AtomSpace* top)
{
	writeFrame(HandleCast(top));
	_multi_space = true;
}

// XXX FIXME; this just returns one top, instead of all of them.
HandleSeq CogSimpleStorage::loadFrameDAG(void)
{
	_multi_space = true;

	std::lock_guard<std::mutex> lck(_mtx);

	std::string msg = "(cog-atomspace)\n";
	do_send(msg);
	std::string rply = do_recv();

	if (rply.size() < 5) return HandleSeq();

	size_t pos = 0;
	Handle top = Sexpr::decode_frame(Handle::UNDEFINED, rply, pos, _fid_map);

	for (const auto& pr : _fid_map)
	{
		std::string shorty = "(AtomSpace \"" + pr.first + "\")";
		_frame_map.insert({pr.second, shorty});
	}

	HandleSeq tops;
	tops.push_back(top);
	return tops;
}

// ===================================================================
