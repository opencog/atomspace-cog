/*
 * FILE:
 * opencog/persist/cog-storage/CogStorage.h
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

#ifndef _OPENCOG_COG_STORAGE_H
#define _OPENCOG_COG_STORAGE_H

#include <opencog/atomspace/AtomTable.h>
#include <opencog/persist/api/StorageNode.h>
#include <opencog/persist/cog-storage/CogChannel.h>

namespace opencog
{
/** \addtogroup grp_persist
 *  @{
 */

class CogStorage : public StorageNode
{
	private:
		void init(const char *);
		std::string _uri;

		struct Pkt
		{
			AtomTable* table;
			Handle h;
			Handle key;
		};

		CogChannel<CogStorage, Pkt> _io_queue;

		void noop_const(const std::string&, const Pkt&) {}
		void noop(const std::string&, Pkt&) {}
		void decode_atom_list(const std::string&, const Pkt&);
		void decode_value(const std::string&, const Pkt&);
		void decode_kvp_list_const(const std::string&, const Pkt&);
		void decode_kvp_list(const std::string& s, Pkt& p)
		{ decode_kvp_list_const(s, p); }
		void is_ok(const std::string&, Pkt&);

	public:
		CogStorage(std::string uri);
		CogStorage(const CogStorage&) = delete; // disable copying
		CogStorage& operator=(const CogStorage&) = delete; // disable assignment
		virtual ~CogStorage();

		void open(void);
		void close(void);
		bool connected(void); // connection to DB is alive

		void create(void) {}
		void destroy(void) { kill_data(); }
		void erase(void) { kill_data(); }

		void kill_data(void); // destroy DB contents

		// AtomStorage interface
		void getAtom(const Handle&);
		void getIncomingSet(AtomTable&, const Handle&);
		void getIncomingByType(AtomTable&, const Handle&, Type t);
		void storeAtom(const Handle&, bool synchronous = false);
		void removeAtom(const Handle&, bool recursive);
		void storeValue(const Handle& atom, const Handle& key);
		void loadValue(const Handle& atom, const Handle& key);
		void runQuery(const Handle&, const Handle&,
		              const Handle&, bool);
		void loadType(AtomTable&, Type);
		void loadAtomSpace(AtomTable&); // Load entire contents
		void storeAtomSpace(const AtomTable&); // Store entire contents
		void barrier();

		// Debugging and performance monitoring
		void print_stats(void);
		void clear_stats(void); // reset stats counters.
};

class CogStorageNode : public CogStorage
{
	public:
		CogStorageNode(Type t, const std::string&& uri) :
			CogStorage(std::move(uri))
		{}
		CogStorageNode(const std::string&& uri) :
			CogStorage(std::move(uri))
		{}
		static Handle factory(const Handle&);
};

typedef std::shared_ptr<CogStorageNode> CogStorageNodePtr;
static inline CogStorageNodePtr CogStorageNodeCast(const Handle& h)
	{ return std::dynamic_pointer_cast<CogStorageNode>(h); }

#define createCogStorageNode std::make_shared<CogStorageNode>


/** @}*/
} // namespace opencog

#endif // _OPENCOG_COG_STORAGE_H
