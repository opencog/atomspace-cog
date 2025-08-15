#
# Build the CogServer driver for the AtomSpace
#

ADD_LIBRARY (persist-cog-simple SHARED
	CogSimpleStorage.cc
	CogSimpleIO.cc
	CogSimplePersistSCM.cc
)

# Include the generated atom types header directory
TARGET_INCLUDE_DIRECTORIES(persist-cog-simple PRIVATE
	${CMAKE_BINARY_DIR}
)

TARGET_LINK_LIBRARIES(persist-cog-simple
	persist-cog-types
	${ATOMSPACE_STORAGE_LIBRARIES}
	${ATOMSPACE_LIBRARIES}
)

ADD_DEPENDENCIES(persist-cog-simple persist_cog_atom_types)

ADD_GUILE_EXTENSION(SCM_CONFIG persist-cog-simple
	"opencog-ext-path-persist-cog-simple")

INSTALL (TARGETS persist-cog-simple EXPORT AtomSpaceTargets
	DESTINATION "lib${LIB_DIR_SUFFIX}/opencog"
)

INSTALL (FILES
   CogSimpleStorage.h
   DESTINATION "include/opencog/persist/cog-simple"
)
