#
# Build the CogServer driver for the AtomSpace
#

ADD_LIBRARY (persist-cog-simple SHARED
	CogSimpleStorage
	CogSimpleIO
	CogSimplePersistSCM
)

TARGET_LINK_LIBRARIES(persist-cog-simple
	${ATOMSPACE_LIBRARIES}
)

ADD_GUILE_EXTENSION(SCM_CONFIG persist-cog "opencog-ext-path-persist-cog")

INSTALL (TARGETS persist-cog-simple EXPORT AtomSpaceTargets
	DESTINATION "lib${LIB_DIR_SUFFIX}/opencog"
)
