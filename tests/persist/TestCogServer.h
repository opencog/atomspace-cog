#include <opencog/atomspace/AtomSpace.h>
#include <opencog/atoms/value/FloatValue.h>
#include <opencog/atoms/value/VoidValue.h>
#include <opencog/atoms/atom_types/atom_names.h>
#include <opencog/cogserver/atoms/CogServerNode.h>
#include <opencog/cogserver/types/atom_types.h>

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
