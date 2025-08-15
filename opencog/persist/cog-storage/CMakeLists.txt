#
# Build the CogServer driver for the AtomSpace
#

ADD_LIBRARY (persist-cog SHARED
	CogIO.cc
	CogPersistSCM.cc
	CogStorage.cc
)

# Include the generated atom types header directory
TARGET_INCLUDE_DIRECTORIES(persist-cog PRIVATE
	${CMAKE_BINARY_DIR}
)

TARGET_LINK_LIBRARIES(persist-cog
	persist-cog-types
	${ATOMSPACE_STORAGE_LIBRARIES}
	${ATOMSPACE_LIBRARIES}
)

ADD_DEPENDENCIES(persist-cog persist_cog_atom_types)

ADD_GUILE_EXTENSION(SCM_CONFIG persist-cog "opencog-ext-path-persist-cog")

INSTALL (TARGETS persist-cog EXPORT AtomSpaceTargets
	DESTINATION "lib${LIB_DIR_SUFFIX}/opencog"
)

INSTALL (FILES
	CogChannel.h
	CogStorage.h
	DESTINATION "include/opencog/persist/cog-storage"
)
