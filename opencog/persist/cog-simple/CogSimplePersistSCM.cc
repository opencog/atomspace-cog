/*
 * opencog/persist/cog-simple/CogSimplePersistSCM.cc
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
#include <opencog/atomspace/BackingStore.h>
#include <opencog/guile/SchemePrimitive.h>

#include "CogSimpleStorage.h"
#include "CogSimplePersistSCM.h"

using namespace opencog;


// =================================================================

CogSimplePersistSCM::CogSimplePersistSCM(AtomSpace *as)
{
    _as = as;
    _backing = nullptr;

    static bool is_init = false;
    if (is_init) return;
    is_init = true;
    scm_with_guile(init_in_guile, this);
}

void* CogSimplePersistSCM::init_in_guile(void* self)
{
    scm_c_define_module("opencog persist-cog-simple", init_in_module, self);
    scm_c_use_module("opencog persist-cog-simple");
    return NULL;
}

void CogSimplePersistSCM::init_in_module(void* data)
{
   CogSimplePersistSCM* self = (CogSimplePersistSCM*) data;
   self->init();
}

void CogSimplePersistSCM::init(void)
{
    define_scheme_primitive("cog-simple-open", &CogSimplePersistSCM::do_open, this, "persist-cog-simple");
    define_scheme_primitive("cog-simple-close", &CogSimplePersistSCM::do_close, this, "persist-cog-simple");
    define_scheme_primitive("cog-simple-stats", &CogSimplePersistSCM::do_stats, this, "persist-cog-simple");
    define_scheme_primitive("cog-simple-clear-stats", &CogSimplePersistSCM::do_clear_stats, this, "persist-cog-simple");
}

CogSimplePersistSCM::~CogSimplePersistSCM()
{
    if (_backing) delete _backing;
}

void CogSimplePersistSCM::do_open(const std::string& uri)
{
    if (_backing)
        throw RuntimeException(TRACE_INFO,
             "cogserver-open: Error: Already connected to a database!");

    // Unconditionally use the current atomspace, until the next close.
    AtomSpace *as = SchemeSmob::ss_get_env_as("cogserver-open");
    if (nullptr != as) _as = as;

    if (nullptr == _as)
        throw RuntimeException(TRACE_INFO,
             "cogserver-open: Error: Can't find the atomspace!");

    // Allow only one connection at a time.
    if (_as->isAttachedToBackingStore())
        throw RuntimeException(TRACE_INFO,
             "cogserver-open: Error: Atomspace connected to another storage backend!");
    // Use the CogServer driver.
    CogSimpleStorage *store = new CogSimpleStorage(uri);
    if (!store)
        throw RuntimeException(TRACE_INFO,
            "cogserver-open: Error: Unable to open the database");

    if (!store->connected())
    {
        delete store;
        throw RuntimeException(TRACE_INFO,
            "cogserver-open: Error: Unable to connect to the database");
    }

    _backing = store;
    _backing->registerWith(_as);
}

void CogSimplePersistSCM::do_close(void)
{
    if (nullptr == _backing)
        throw RuntimeException(TRACE_INFO,
             "cogserver-close: Error: AtomSpace not connected to CogServer!");

    CogSimpleStorage *backing = _backing;
    _backing = nullptr;

    // The destructor might run for a while before its done; it will
    // be emptying the pending store queues, which might take a while.
    // So unhook the atomspace first -- this will prevent new writes
    // from accidentally being queued. (It will also drain the queues)
    // Only then actually call the dtor.
    backing->unregisterWith(_as);
    delete backing;
}

void CogSimplePersistSCM::do_stats(void)
{
    if (nullptr == _backing) {
        printf("cogserver-stats: AtomSpace not connected to CogServer!\n");
        return;
    }

    printf("cogserver-stats: Atomspace holds %lu atoms\n", _as->get_size());
    _backing->print_stats();
}

void CogSimplePersistSCM::do_clear_stats(void)
{
    if (nullptr == _backing) {
        printf("cogserver-stats: AtomSpace not connected to CogServer!\n");
        return;
    }

    _backing->clear_stats();
}

void opencog_persist_cog_simple_init(void)
{
    static CogSimplePersistSCM patty(NULL);
}
