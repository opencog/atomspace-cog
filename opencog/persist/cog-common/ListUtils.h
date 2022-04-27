/*
 * ListUtils.h
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

#ifndef _COG_STORE_UTILS_H
#define _COG_STORE_UTILS_H

#include <opencog/atoms/base/Handle.h>
namespace opencog
{
/** \addtogroup grp_persist
 *  @{
 */
void ro_decode_alist(const Handle&, const std::string&);

/** @}*/
} // namespace opencog

#endif // _COG_STORE_UTILS_H
