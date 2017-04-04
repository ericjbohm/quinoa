package Quinoa_2_Linux.buildTypes

import jetbrains.buildServer.configs.kotlin.v10.*
import jetbrains.buildServer.configs.kotlin.v10.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v10.buildSteps.ScriptBuildStep.*
import jetbrains.buildServer.configs.kotlin.v10.buildSteps.script

object Quinoa_2_Linux_Matrix : Template({
    uuid = "dfd6c8ef-72e2-4e44-be66-fea7a62d455e"
    extId = "Quinoa_2_Linux_Matrix"
    name = "Matrix"

    vcs {
        root(Quinoa_2.vcsRoots.Quinoa_2_GitGithubComQuinoacomputingQuinoaGitRefsHeadsMaster)

    }

    steps {
        script {
            name = "Verify commit"
            id = "RUNNER_17"
            scriptContent = """/ccs/opt/git/bin/git verify-commit %build.vcs.number% 2>&1 | grep "Good signature""""
        }
        script {
            name = "Build code"
            id = "RUNNER_18"
            scriptContent = """
                ${'$'}SPACK_ROOT/share/spack/setup-env.sh
                [ %compiler% == clang ] && module load clang/latest openmpi/1.10.2/clang/latest
                [ %compiler% == gnu ] && module load openmpi-2.0.1-gcc-4.8.5-jv7w2de
                [ %compiler% == intel ] && module load intel/latest openmpi/1.10.2/intel/latest
                [ %mathlib% == mkl ] && module load mkl/latest
                [ %mathlib% == lapack ] && module load netlib-lapack-3.6.1-gcc-4.8.5-snwxnfw
                [ %rngsse2% == true ] && module load rngsse2
                [ %testu01% == true ] && module load testu01
                module load charm/%compiler%-%stdlibcpp% h5part/%compiler%-%stdlibcpp% trilinos/%compiler%-%stdlibcpp%/%mathlib%
                module load pugixml pegtl pstreams boost-1.61.0-gcc-4.8.5-q2hywin hypre-2.10.1-gcc-4.8.5-beuxbxv gmsh-2.12.0-gcc-4.8.5-p3vpjfb hdf5-1.10.0-patch1-gcc-4.8.5-mmtlfty netcdf-4.4.1-gcc-4.8.5-5xen4a5 random123 tut cartesian_product numdiff libc++
                module list
                rm -rf build && mkdir build && cd build
                cmake -DCMAKE_CXX_COMPILER=mpicxx -DCMAKE_C_COMPILER=mpicc -DCMAKE_BUILD_TYPE=%buildtype% -DSTDLIBCPP=%stdlibcpp% -DCMAKE_DISABLE_FIND_PACKAGE_RNGSSE2=!%rngsse2% -DCMAKE_DISABLE_FIND_PACKAGE_TestU01=!%testu01% -DCMAKE_CXX_FLAGS=-Werror ../src
                make -j16
            """.trimIndent()
        }
        script {
            name = "Run tests"
            id = "RUNNER_19"
            workingDir = "build"
            scriptContent = """
                . ${'$'}SPACK_ROOT/share/spack/setup-env.sh
                [ %compiler% == clang ] && module load clang/latest openmpi/1.10.2/clang/latest
                [ %compiler% == gnu ] && module load openmpi-2.0.1-gcc-4.8.5-jv7w2de
                [ %compiler% == intel ] && module load intel/latest openmpi/1.10.2/intel/latest
                [ %mathlib% == mkl ] && module load mkl/latest
                [ %mathlib% == lapack ] && module load netlib-lapack-3.6.1-gcc-4.8.5-snwxnfw
                [ %rngsse2% == true ] && module load rngsse2
                [ %testu01% == true ] && module load testu01
                module load charm/%compiler%-%stdlibcpp% h5part/%compiler%-%stdlibcpp% trilinos/%compiler%-%stdlibcpp%/%mathlib%
                module load pugixml pegtl pstreams boost-1.61.0-gcc-4.8.5-q2hywin hypre-2.10.1-gcc-4.8.5-beuxbxv gmsh-2.12.0-gcc-4.8.5-p3vpjfb hdf5-1.10.0-patch1-gcc-4.8.5-mmtlfty netcdf-4.4.1-gcc-4.8.5-5xen4a5 random123 tut cartesian_product numdiff libc++
                module list
                ../script/run_tests.sh 16
            """.trimIndent()
        }
    }

    requirements {
        equals("teamcity.agent.jvm.os.name", "Linux", "RQ_19")
        contains("teamcity.agent.name", "ccscs", "RQ_20")
    }
})