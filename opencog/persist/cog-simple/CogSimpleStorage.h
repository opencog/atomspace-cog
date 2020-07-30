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

#include <opencog/atomspace/AtomTable.h>
#include <opencog/atomspace/BackingStore.h>

namespace opencog
{
/** \addtogroup grp_persist
 *  @{
 */

class CogSimpleStorage : public BackingStore
{
	private:
		void init(const char *);
		std::string _uri;

		// Socket API ... is single-threaded.
		std::mutex _mtx;
		int _sockfd;
		void do_send(const std::string&);
		std::string do_recv(void);

		void decode_atom_list(AtomTable&);

	public:
		CogSimpleStorage(std::string uri);
		CogSimpleStorage(const CogSimpleStorage&) = delete; // disable copying
		CogSimpleStorage& operator=(const CogSimpleStorage&) = delete; // disable assignment
		virtual ~CogSimpleStorage();
		bool connected(void); // connection to DB is alive

		void kill_data(void); // destroy DB contents

		void registerWith(AtomSpace*);
		void unregisterWith(AtomSpace*);

		// AtomStorage interface
		Handle getNode(Type, const char *);
		Handle getLink(Type, const HandleSeq&);
		void getIncomingSet(AtomTable&, const Handle&);
		void getIncomingByType(AtomTable&, const Handle&, Type t);
		void storeAtom(const Handle&, bool synchronous = false);
		void removeAtom(const Handle&, bool recursive);
		void storeValue(const Handle& atom, const Handle& key);
		ValuePtr loadValue(const Handle& atom, const Handle& key);
//		ValuePtr runQuery(const Handle&, const Handle&,
//		                  const Handle&, bool);
		void loadType(AtomTable&, Type);
		void loadAtomSpace(AtomTable&); // Load entire contents
		void storeAtomSpace(const AtomTable&); // Store entire contents
		void barrier();

		// Debugging and performance monitoring
		void print_stats(void);
		void clear_stats(void); // reset stats counters.
};

/** @}*/
} // namespace opencog

#endif // _SIMPLE_COG_STORAGE_H
