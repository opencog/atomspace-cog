
LINK_LIBRARIES(
	persist-cog
	persist
	server
	atomspace
)

# The seven unit tests, ported over from the
# atomspace/persist/sql/multi-driver unit tests.
# Same as the atomspace/persist/ipfs unit tests.
ADD_CXXTEST(BasicSaveUTest)
ADD_CXXTEST(ValueSaveUTest)
ADD_CXXTEST(PersistUTest)
ADD_CXXTEST(FetchUTest)
TARGET_LINK_LIBRARIES(FetchUTest execution)
ADD_CXXTEST(BasicDeleteUTest)
ADD_CXXTEST(DeleteUTest)
ADD_CXXTEST(AlphaEquivUTest)
ADD_CXXTEST(QueryPersistUTest)
ADD_CXXTEST(MultiPersistUTest)
ADD_CXXTEST(MultiUserUTest)
ADD_CXXTEST(MultiDeleteUTest)

# XXX FIXME Disable these two tests for now;
# they take half an hour to run!
ADD_CXXTEST(LargeFlatUTest)
ADD_CXXTEST(LargeZipfUTest)
