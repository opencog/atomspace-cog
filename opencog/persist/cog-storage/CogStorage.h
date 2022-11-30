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
			AtomSpace* table;
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

		void ro_decode_alist(AtomSpace*, const Handle&, const std::string&);

	public:
		CogStorage(std::string uri);
		CogStorage(const CogStorage&) = delete; // disable copying
		CogStorage& operator=(const CogStorage&) = delete; // disable assignment
		virtual ~CogStorage();

		// StorageNode API implementation (virtual methods)
		void open(void);
		void close(void);
		bool connected(void); // connection to DB is alive

		void create(void) {}
		void destroy(void) { kill_data(); }
		void erase(void) { kill_data(); }
		void kill_data(void); // destroy DB contents

		void proxy_open(void);
		void proxy_close(void);
		void set_proxy(const Handle&);

		// BackingStore interface implementation (virual methods)
		void getAtom(const Handle&);
		void fetchIncomingSet(AtomSpace*, const Handle&);
		void fetchIncomingByType(AtomSpace*, const Handle&, Type t);
		void storeAtom(const Handle&, bool synchronous = false);
		void removeAtom(AtomSpace*, const Handle&, bool recursive);
		void storeValue(const Handle& atom, const Handle& key);
		void updateValue(const Handle&, const Handle&, const ValuePtr&);
		void loadValue(const Handle& atom, const Handle& key);
		void runQuery(const Handle&, const Handle&,
		              const Handle&, bool);
		void loadType(AtomSpace*, Type);
		void loadAtomSpace(AtomSpace*); // Load entire contents
		void storeAtomSpace(const AtomSpace*); // Store entire contents
		void barrier(AtomSpace* = nullptr);

		// Debugging and performance monitoring
		std::string monitor(void);
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

NODE_PTR_DECL(CogStorageNode)
#define createCogStorageNode CREATE_DECL(CogStorageNode)

/** @}*/
} // namespace opencog

#endif // _OPENCOG_COG_STORAGE_H
