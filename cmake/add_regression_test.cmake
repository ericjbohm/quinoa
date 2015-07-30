# ##############################################################################
# Function used to add a regression test to the ctest test suite
#
# add_regression_test( <test_name> <executable>
#                      [NUMPES n]
#                      [INPUTFILES file1 file2 ...]
#                      [ARGS arg1 arg2 ...]
#                      [TEXT_DIFF_PROG txtdiff]
#                      [BIN_DIFF_PROG bindiff]
#                      [TEXT_BASELINE stat.std]
#                      [TEXT_RESULT stat.txt] )
#                      [TEXT_DIFF_PROG_CONF ndiff.cfg]
#
# Mandatory arguments:
# --------------------
#
# <test_name>      Name of the test. Note that "_PEs{NUMPES}" will be appended
# to the name. See also argument NUMPES below.
#
# <executable>      Name of the executable to test.
#
# Optional arguments:
# -------------------
#
# NUMPES n      The number PEs to pass to charmrun, i.e., +pn. Default: 1.
#
# INPUTFILES file1 file2 ...      Input files required for the test. This list
# of files includes files needed for running the test, e.g., control file
# (containing user input), mesh file, etc., as well as file required for
# evaluating the test, e.g., diff program configuration file. All input files
# are soft-linked from the source dir to the build dir. Default: "".
#
# ARGS arg1 arg2 ...      Arguments to pass to executable tested. Default: "".
#
# TEXT_DIFF_PROG txtdiff      Diff program used for textual diffs. Default:
# numdiff.
#
# BIN_DIFF_PROG bindiff      Diff program used for binary diffs. Default:
# exodiff.
#
# TEXT_BASELINE stat.std      Textual file containing the known good solution.
# If unspecified, no textual diff is performed. Default: "".
#
# TEXT_RESULT stat.txt      Textual file produced by the test to be tested. If
# unspecified, no textual diff is performed. Default: "".
#
# TEXT_DIFF_PROG_CONF ndiff.cfg      Textual diff program configuration file.
# Default: "".
#
# Author: J. Bakosi
#
# ##############################################################################
function(ADD_REGRESSION_TEST test_name executable)

  set(oneValueArgs NUMPES TEXT_DIFF_PROG BIN_DIFF_PROG TEXT_BASELINE TEXT_RESULT TEXT_DIFF_PROG_CONF)
  set(multiValueArgs INPUTFILES ARGS)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}"
                        ${ARGN})

  # Will collect test properties
  set(test_properties)

  # Set number of processing elements
  set(NUMPES 1)
  if (ARG_NUMPES)
    set(NUMPES ${ARG_NUMPES})
    list(APPEND test_properties PROCESSORS ${ARG_NUMPES})
  endif()
  # Set textual diff tool
  set(TEXT_DIFF_PROG ${NDIFF_EXECUTABLE})
  if (ARG_TEXT_DIFF_PROG)
    set(TEXT_DIFF_PROG ${ARG_TEXT_DIFF_PROG})
  endif()
  # Set binary diff tool
  set(BIN_DIFF_PROG ${EXODIFF_EXECUTABLE})
  if (ARG_BIN_DIFF_PROG)
    set(BIN_DIFF_PROG ${ARG_BIN_DIFF_PROG})
  endif()

  # Append NUMPES to test name
  set(test_name "${test_name}_pe${NUMPES}")

  # Construct and echo configuration for test being added
  set(msg "Add regression test ${test_name} for ${executable}")
  if (ARG_ARGS)
    string(REPLACE ";" " " ARGUMENTS "${ARG_ARGS}")
    string(CONCAT msg "${msg}, args: '${ARGUMENTS}'")
  endif()
  message(STATUS ${msg})

  # Set and create test run directory
  set(workdir ${CMAKE_CURRENT_BINARY_DIR}/${test_name})
  file(MAKE_DIRECTORY ${workdir})

  # Create list of files required to softlink to build directory
  set(reqfiles)
  foreach(file ${ARG_INPUTFILES})
    list(APPEND reqfiles "${CMAKE_CURRENT_SOURCE_DIR}/${file}")
  endforeach()

  # Softlink files required to build directory
  foreach(target ${reqfiles})
    set(LN_COMMAND "ln -sf ${target} ${workdir}")
    exec_program(${LN_COMMAND} OUTPUT_VARIABLE ln_output RETURN_VALUE ln_retval)
    if("${ln_retval}" GREATER 0)
      message(FATAL_ERROR "Problem creating symlink from \"${target}\" to \"${workdir}\":\n${ln_output}")
    endif()
  endforeach()

  # Add the test
  add_test(NAME ${test_name}
           COMMAND ${CMAKE_COMMAND}
           -DTEST_NAME=${test_name}
           -DWORKDIR=${workdir}
           -DCHARMRUN=${CHARMRUN}
           -DTEST_EXECUTABLE=${CMAKE_BINARY_DIR}/Main/${executable}
           -DTEST_EXECUTABLE_ARGS=${ARGUMENTS}
           -DNUMPES=${NUMPES}
           -DTEXT_DIFF_PROG=${TEXT_DIFF_PROG}
           -DTEXT_DIFF_PROG_ARGS=
           -DTEXT_DIFF_PROG_CONF=${ARG_TEXT_DIFF_PROG_CONF}
           -DTEXT_BASELINE=${ARG_TEXT_BASELINE}
           -DTEXT_RESULT=${ARG_TEXT_RESULT}
           -DBIN_DIFF_PROG=${BIN_DIFF_PROG}
           -DBIN_DIFF_PROG_ARGS=
           -DBIN_DIFF_PROG_CONFFILE=
           -DBIN_BASELINE=
           -DBIN_RESULT=
           -P ${TEST_RUNNER}
           WORKING_DIRECTORY ${workdir})

  # Instruct ctest to check textual diff output against the regular expressions
  # specified. At least one of the regular expressions has to match, otherwise
  # the test will fail.
  list(APPEND test_properties PASS_REGULAR_EXPRESSION ".*${test_name}.*PASS")

  # Set test properties
  set_tests_properties(${test_name} PROPERTIES ${test_properties})

endfunction()