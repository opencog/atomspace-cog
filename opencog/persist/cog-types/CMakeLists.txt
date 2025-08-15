# Generate the atom types
OPENCOG_GEN_CXX_ATOMTYPES(atom_types.script
	atom_types.h
	atom_types.definitions
	atom_types.inheritance)

OPENCOG_GEN_SCM_ATOMTYPES(atom_types.script persist_cog_types.scm)
OPENCOG_GEN_PYTHON_ATOMTYPES(atom_types.script persist_cog_types.pyx)

ADD_CUSTOM_TARGET(persist_cog_atom_types
	DEPENDS atom_types.h persist_cog_types.scm persist_cog_types.pyx)

# Build the shared library
ADD_LIBRARY (persist-cog-types SHARED
	atom_types_init.cc
)

# The atom_types.h file is written to the build directory
TARGET_INCLUDE_DIRECTORIES(persist-cog-types PRIVATE ${CMAKE_BINARY_DIR})

TARGET_LINK_LIBRARIES(persist-cog-types
	${ATOMSPACE_STORAGE_LIBRARIES}
	${ATOMSPACE_LIBRARIES}
)

ADD_DEPENDENCIES(persist-cog-types persist_cog_atom_types)

# Install targets
INSTALL (TARGETS persist-cog-types EXPORT AtomSpaceCogStorageTargets
	DESTINATION "lib${LIB_DIR_SUFFIX}/opencog"
)

INSTALL (FILES
	${CMAKE_CURRENT_BINARY_DIR}/atom_types.h
	DESTINATION "include/opencog/persist/cog-types"
)

ADD_GUILE_MODULE (FILES
	${CMAKE_CURRENT_BINARY_DIR}/persist_cog_types.scm
	MODULE_DESTINATION "${GUILE_SITE_DIR}/opencog/persist/cog-types"
	DEPENDS persist_cog_atom_types
)

ADD_GUILE_EXTENSION(SCM_CONFIG persist-cog-types "opencog-ext-path-persist-cog-types")