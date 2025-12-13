#include <opencog/atomspace/AtomSpace.h>
#include <opencog/atoms/atom_types/atom_types.h>
#include <opencog/atoms/value/FloatValue.h>
#include <opencog/atoms/value/VoidValue.h>
#include <opencog/atoms/atom_types/atom_names.h>
#include <opencog/cogserver/atoms/CogServerNode.h>
#include <opencog/cogserver/types/atom_types.h>
#include <opencog/persist/api/StorageNode.h>

using namespace opencog;

// Selectively clear test data from the remote atomspace without
// removing the CogServerNode itself. This replaces kill_data() which
// would destroy the CogServerNode and cause segfaults.
inline void kill_data(StorageNode* store, AtomSpace* asp)
{
    // Load all nodes from remote storage
    store->fetch_all_atoms_of_type(NODE, asp);
    store->barrier();

    HandleSeq nodes;
    asp->get_handles_by_type(nodes, NODE, true);
    for (const Handle& h : nodes)
    {
        Type t = h->get_type();
        // Keep the CogServerNode
        if (t == COG_SERVER_NODE) continue;
        // Keep infrastructure PredicateNodes (those starting with "*-")
#if 0
        if (t == PREDICATE_NODE)
        {
            const std::string& name = h->get_name();
            if (name.size() > 2 && name[0] == '*' && name[1] == '-')
                continue;
        }
#endif
        store->remove_atom(asp, h, true);
    }
    store->barrier();
}

#define DECLARE_TEST_COGSERVER \
    AtomSpacePtr _test_asp; \
    CogServerNodePtr _test_csrv;

#define INIT_TEST_COGSERVER(port) \
    _test_asp = createAtomSpace(); \
    _test_csrv = CogServerNodeCast( \
        _test_asp->add_node(COG_SERVER_NODE, "test-cogserver")); \
    _test_csrv->setValue(_test_asp->add_atom(Predicate("*-telnet-port-*")), \
                         createFloatValue((double)(port))); \
    _test_csrv->setValue(_test_asp->add_atom(Predicate("*-web-port-*")), \
                         createFloatValue(0.0)); \
    _test_csrv->setValue(_test_asp->add_atom(Predicate("*-mcp-port-*")), \
                         createFloatValue(0.0)); \
    _test_csrv->setValue(_test_asp->add_atom(Predicate("*-start-*")), \
                         createVoidValue());

#define STOP_TEST_COGSERVER \
    _test_csrv->setValue(_test_asp->add_atom(Predicate("*-stop-*")), \
                         createVoidValue());
