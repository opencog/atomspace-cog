
LINK_LIBRARIES(
	persist-cog-simple
	persist
	server
	atomspace
)

# The seven unit tests, ported over from the
# atomspace/persist/sql/multi-driver unit tests.
# Same as the atomspace/persist/ipfs unit tests.
ADD_CXXTEST(SimpleBasicSaveUTest)
ADD_CXXTEST(SimpleValueSaveUTest)
ADD_CXXTEST(SimplePersistUTest)
ADD_CXXTEST(SimpleFetchUTest)
TARGET_LINK_LIBRARIES(SimpleFetchUTest execution)
ADD_CXXTEST(SimpleDeleteUTest)
ADD_CXXTEST(SimpleMultiPersistUTest)
ADD_CXXTEST(SimpleMultiUserUTest)

# XXX FIXME Disable these two tests for now;
# they take half an hour to run!
ADD_CXXTEST(SimpleLargeFlatUTest)
ADD_CXXTEST(SimpleLargeZipfUTest)
