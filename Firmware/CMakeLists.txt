include_directories("~/opt/Sming/Sming")
include_directories("~/opt/Sming/Sming/Libraries")
include_directories("~/opt/Sming/Sming/system/include")
include_directories("~/opt/esp-open-sdk/sdk/include")
include_directories("include")

file(GLOB SOURCE_FILES
        "app/*.cpp"
        )
add_executable(Pathfinder ${SOURCE_FILES})