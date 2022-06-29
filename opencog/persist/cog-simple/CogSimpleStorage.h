/*
 * FILE:
 * opencog/persist/cog-simple/CogSimpleStorage.h
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

#ifndef _SIMPLE_COG_STORAGE_H
#define _SIMPLE_COG_STORAGE_H

#include <opencog/persist/api/StorageNode.h>

namespace opencog
{
/** \addtogroup grp_persist
 *  @{
 */

class CogSimpleStorage : public StorageNode
{
	private:
		void init(const char *);
		std::string _uri;
		std::string _proxy;

		// Socket API ... is single-threaded.
		std::mutex _mtx;
		int _sockfd;
		void do_send(const std::string&);
		std::string do_recv(bool=false);

		void decode_atom_list(AtomSpace*);
		void ro_decode_alist(AtomSpace*, const Handle&, const std::string&);

		// True if working with more than one atomspace.
		bool _multi_space;
		// The Handles are *always* AtomSpacePtr's
		std::unordered_map<Handle, const std::string> _frame_map;
		std::unordered_map<std::string, Handle> _fid_map;
		std::mutex _mtx_frame;
		void cacheFrame(const Handle&);
		std::string writeFrame(const Handle&);
		std::string writeFrame(AtomSpace* as) {
			return writeFrame(HandleCast(as));
		}
		Handle getFrame(const std::string&);


	public:
		CogSimpleStorage(std::string uri);
		CogSimpleStorage(const CogSimpleStorage&) = delete; // disable copying
		CogSimpleStorage& operator=(const CogSimpleStorage&) = delete; // disable assignment
		virtual ~CogSimpleStorage();

		void open(void);
		void close(void);
		bool connected(void); // connection to DB is alive

		void create(void) {}
		void destroy(void) { kill_data(); }
		void erase(void) { kill_data(); }

		void kill_data(void); // destroy DB contents

		// AtomStorage interface
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
		HandleSeq loadFrameDAG(void); // Load AtomSpace DAG
		void storeFrameDAG(AtomSpace*); // Store AtomSpace DAG
		void barrier(AtomSpace* = nullptr);

		// Debugging and performance monitoring
		void print_stats(void);
		void clear_stats(void); // reset stats counters.
};

class CogSimpleStorageNode : public CogSimpleStorage
{
	public:
		CogSimpleStorageNode(Type t, const std::string&& uri) :
			CogSimpleStorage(std::move(uri))
		{}
		CogSimpleStorageNode(const std::string&& uri) :
			CogSimpleStorage(std::move(uri))
		{}
		static Handle factory(const Handle&);
};

typedef std::shared_ptr<CogSimpleStorageNode> CogSimpleStorageNodePtr;
static inline CogSimpleStorageNodePtr CogSimpleStorageNodeCast(const Handle& h)
	{ return std::dynamic_pointer_cast<CogSimpleStorageNode>(h); }

#define createCogSimpleStorageNode std::make_shared<CogSimpleStorageNode>


/** @}*/
} // namespace opencog

#endif // _SIMPLE_COG_STORAGE_H
