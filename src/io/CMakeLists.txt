set (io_sources
  BDATReader.cpp
  GLDatasetBase.cpp
  GLDataset.cpp
  GLGPUDataset.cpp
  GLGPU2DDataset.cpp
  GLGPU3DDataset.cpp
  GLGPU_IO_Helper.cpp
)
  
if (WITH_LIBMESH)
  set (io_sources ${io_sources}
    Condor2Dataset.cpp)
endif ()

add_library (glio STATIC ${io_sources})

if (WITH_LIBMESH)
  target_link_libraries (glio PUBLIC glcommon glpp ${LIBMESH_LIBRARY} ${NETCDF_LIBRARY})
else ()
  target_link_libraries (glio PUBLIC glcommon glpp)
endif ()
  
if (WITH_NETCDF)
  target_link_libraries (glio PUBLIC ${NETCDF_LIBRARIES})
endif ()

#install (TARGETS glio DESTINATION ${VTK_INSTALL_LIBRARY_DIR})

add_subdirectory (glpp)
