# client/CMakeLists.txt

add_executable(r-type_client
    src/main.cpp
)

# Temporarily commented out - rtype_engine not yet implemented
# target_link_libraries(r-type_client
#     PRIVATE
#         rtype_engine
# )

