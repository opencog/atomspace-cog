
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
ADD_CXXTEST(SimpleBasicDeleteUTest)
ADD_CXXTEST(SimpleDeleteUTest)
ADD_CXXTEST(SimpleAlphaEquivUTest)
ADD_CXXTEST(SimpleMultiPersistUTest)
ADD_CXXTEST(SimpleMultiUserUTest)
ADD_CXXTEST(SimpleMultiDeleteUTest)
ADD_CXXTEST(SimpleQueryPersistUTest)
#
ADD_GUILE_TEST(SpaceFrame space-frame-test.scm)
# ADD_GUILE_TEST(SpaceDiamond space-diamond-test.scm)
# ADD_GUILE_TEST(SpaceWye space-wye-test.scm)
# ADD_GUILE_TEST(FrameDelete frame-delete-test.scm)
#
ADD_CXXTEST(SimpleLargeFlatUTest)
ADD_CXXTEST(SimpleLargeZipfUTest)
