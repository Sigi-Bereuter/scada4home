project(scada4home)

add_executable(scada4home SharedUtils.cpp ScadaItem.cpp ItemRepository.cpp HMIManager.cpp IntertechnoFSM.cpp LogTracer.cpp ControlManager.cpp CULManager.cpp CUNOEmulator.cpp PLCManager.cpp main.cpp mongoose.cpp)

target_link_libraries(scada4home /usr/local/lib/libmodbus.so)
target_link_libraries(scada4home /usr/lib/i386-linux-gnu/libpthread.so)
target_link_libraries(scada4home /usr/lib/i386-linux-gnu/libdl.so)
add_subdirectory(DefaultSitemaps)