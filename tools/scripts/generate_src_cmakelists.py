import glob

header = """
# generated by tools/scripts/generate_src_cmakelists.py. DO NOT MODIFY

"""

footer = """

target_link_libraries(cedar uv_a ${CMAKE_DL_LIBS} -lgc -lgccpp -pthread)
set_target_properties(cedar PROPERTIES OUTPUT_NAME cedar)

target_link_libraries(cedar-lib uv_a ${CMAKE_DL_LIBS} -lgc -lgccpp -pthread)
set_target_properties(cedar-lib PROPERTIES OUTPUT_NAME cedar)
"""

with open('src/cedar/CMakeLists.txt', 'w') as f:
    f.write(header)
    f.write('add_executable(cedar\n')
    for filename in glob.iglob('src/**/*.cpp', recursive=True):
        f.write('\t%s\n' % (filename))
    f.write(")")
    f.write("\n");


    f.write('add_library(cedar-lib SHARED\n')
    for filename in glob.iglob('src/cedar/**/*.cpp', recursive=True):
        f.write('\t%s\n' % (filename))
    f.write(")")
    f.write("\n");

    f.write(footer)

