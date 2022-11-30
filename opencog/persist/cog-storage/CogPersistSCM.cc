/*
 * opencog/persist/cog-storage/CogPersistSCM.cc
 * Scheme Guile API wrappers for the backend.
 *
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

#include <libguile.h>

#include <opencog/atomspace/AtomSpace.h>
#include <opencog/persist/api/PersistSCM.h>
#include <opencog/persist/api/StorageNode.h>
#include <opencog/guile/SchemePrimitive.h>

#include "CogStorage.h"
#include "CogPersistSCM.h"

using namespace opencog;


// =================================================================

CogPersistSCM::CogPersistSCM(AtomSpace *as)
{
    if (as)
        _as = AtomSpaceCast(as->shared_from_this());

    static bool is_init = false;
    if (is_init) return;
    is_init = true;
    scm_with_guile(init_in_guile, this);
}

void* CogPersistSCM::init_in_guile(void* self)
{
    scm_c_define_module("opencog persist-cog", init_in_module, self);
    scm_c_use_module("opencog persist-cog");
    return NULL;
}

void CogPersistSCM::init_in_module(void* data)
{
   CogPersistSCM* self = (CogPersistSCM*) data;
   self->init();
}

void CogPersistSCM::init(void)
{
    define_scheme_primitive("cog-storage-open", &CogPersistSCM::do_open, this, "persist-cog");
    define_scheme_primitive("cog-storage-close", &CogPersistSCM::do_close, this, "persist-cog");
}

CogPersistSCM::~CogPersistSCM()
{
    _storage = nullptr;
}

void CogPersistSCM::do_open(const std::string& uri)
{
    if (_storage)
        throw RuntimeException(TRACE_INFO,
             "cogserver-open: Error: Already connected to a database!");

    // Unconditionally use the current atomspace, until the next close.
    AtomSpacePtr asp = SchemeSmob::ss_get_env_as("cogserver-open");
    if (nullptr != asp) _as = asp;

    if (nullptr == _as)
        throw RuntimeException(TRACE_INFO,
             "cogserver-open: Error: Can't find the atomspace!");

    // Adding the CogStorageNode to the atomspace will fail on
    // read-only atomspaces.
    if (_as->get_read_only())
        throw RuntimeException(TRACE_INFO,
             "cogserver-open: Error: AtomSpace is read-only!");

    // Use the CogServer driver.
    Handle hsn = _as->add_node(COG_STORAGE_NODE, std::string(uri));
    _storage = CogStorageNodeCast(hsn);
    _storage->open();

    if (!_storage->connected())
    {
        _as->extract_atom(hsn);
        _storage = nullptr;
        throw RuntimeException(TRACE_INFO,
            "cogserver-open: Error: Unable to connect to the database");
    }

    PersistSCM::set_connection(_storage);
}

void CogPersistSCM::do_close(void)
{
    if (nullptr == _storage)
        throw RuntimeException(TRACE_INFO,
             "cogserver-close: Error: AtomSpace not connected to CogServer!");

    // The destructor might run for a while before its done; it will
    // be emptying the pending store queues, which might take a while.
    // So unhook the atomspace first -- this will prevent new writes
    // from accidentally being queued. (It will also drain the queues)
    // Only then actually call the dtor.
    _storage->close();
    _as->extract_atom(HandleCast(_storage));
    PersistSCM::set_connection(nullptr);
    _storage = nullptr;
}

void opencog_persist_cog_init(void)
{
	static CogPersistSCM patty(nullptr);
}
