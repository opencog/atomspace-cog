/*
 * ListUtils.cc
 * decode S-Expressions containg lists of Atomese Values.
 *
 * Copyright (c) 2019, 2022 Linas Vepstas <linas@linas.org>
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
#include <opencog/persist/sexpr/Sexpr.h>
#include <opencog/atomspace/AtomSpace.h>

using namespace opencog;

/* ================================================================== */

/**
 * Decode a Valuation association list.
 * This list has the format
 * ((KEY . VALUE)(KEY2 . VALUE2)...)
 * Store the results as values on the atom.
 *
 * Copy of code originally appearing in ValueSexpr.cc, adjusted to
 * work with read-only spaces.
 */
void ro_decode_alist(const Handle& atom,
                     const std::string& alist)
{
	size_t pos = 0;
	AtomSpace* as = atom->getAtomSpace();

	pos = alist.find_first_not_of(" \n\t", pos);
	if (std::string::npos == pos) return;
	if ('(' != alist[pos])
		throw SyntaxException(TRACE_INFO, "Badly formed alist: %s",
			alist.substr(pos).c_str());

	// Skip over opening paren
	pos++;
	size_t totlen = alist.size();
	pos = alist.find('(', pos);
	while (std::string::npos != pos and pos < totlen)
	{
		++pos;  // over first paren of pair
		Handle key(Sexpr::decode_atom(alist, pos));

		pos = alist.find(" . ", pos);
		pos += 3;
		ValuePtr val(Sexpr::decode_value(alist, pos));

		// Make sure all atoms have found a nice home.
		if (as)
		{
			Handle hkey = as->add_atom(key);
			if (hkey) key = hkey; // might be null, if `as` is read-only
			val = as->add_atoms(val);
			as->set_value(atom, key, val);
		}
		else
			atom->setValue(key, val);
		pos = alist.find('(', pos);
	}
}

/* ============================= END OF FILE ================= */
