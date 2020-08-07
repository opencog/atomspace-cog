
LINK_LIBRARIES(
	persist-cog-simple
	persist
	server
	atomspace
)

# The nine unit tests, ported over from the
# atomspace/persist/sql/multi-driver unit tests.
# Same as the atomspace/persist/cog-storage unit tests.
# (But with a different name to avoid CMake collisions)
ADD_CXXTEST(SimpleBasicSaveUTest)
ADD_CXXTEST(SimpleValueSaveUTest)
ADD_CXXTEST(SimplePersistUTest)
ADD_CXXTEST(SimpleFetchUTest)
TARGET_LINK_LIBRARIES(SimpleFetchUTest execution)
ADD_CXXTEST(SimpleDeleteUTest)
ADD_CXXTEST(SimpleMultiPersistUTest)
ADD_CXXTEST(SimpleMultiUserUTest)
ADD_CXXTEST(SimpleQueryPersistUTest)

ADD_CXXTEST(SimpleLargeFlatUTest)
ADD_CXXTEST(SimpleLargeZipfUTest)
