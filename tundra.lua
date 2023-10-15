local msvc_common = {
    -- Unicode
    "/DUNICODE",
    "/D_UNICODE", -- _tcslen -> wcslen
    -- Defines _DEBUG, _MT, and _DLL and causes the application to use the debug multithread-specific and DLL-specific version of the run-time library.
    {
        "/MDd",
        Config = "*-msvc-debug-*"
    }, -- Visual Studio 2019 version 16.9
    -- https://docs.microsoft.com/en-us/cpp/sanitizers/asan?view=msvc-170
    -- https://github.com/google/sanitizers/wiki/AddressSanitizer
    -- https://github.com/google/sanitizers/wiki/AddressSanitizerAlgorithm
    -- {"/fsanitize=address", Config = "*-msvc-debug-*"},
    "/D_CRT_SECURE_NO_WARNINGS",
    "/D_WINSOCK_DEPRECATED_NO_WARNINGS",
    "/W4", -- treat as error
    "/we4701", -- Potentially uninitialized local variable 'name' used
    "/we4715", -- 'function' : not all control paths return a value
    "/we4244", -- conversion' conversion from 'type1' to 'type2', possible loss of data
    -- disable (really?)
    "/wd4100", -- unreferenced formal parameter
    "/wd4706", -- assignment within conditional expression
    "/wd4324", -- structure was padded due to alignment specifier
    --
    -- https://twitter.com/niklasfrykholm/status/1302085215313997825
    -- You want both 4820 and 4121. 4121 is on with W4.
    --
    -- windows.h requires Microsoft language extensions?
    -- "/Za" -- emits an error for language constructs that are not compatible with ANSI C89 or ISO C++11
    -- Disables run-time type information

    "/GR-", -- Use SEE (with VEX encoding) but not AVX

    "/GS-", -- Disable Buffer Security Check

    "/arch:AVX", -- Enable most speed optimizations

    -- (this is an absolute must, without this option it's not possible to write fast code)
    {
        "/Od",
        Config = "*-msvc-debug-*"
    },
    {
        "/Ox", -- Maximize Speed
        Config = "*-msvc-release-*"
    },

    -- https://docs.microsoft.com/en-us/cpp/build/reference/fp-specify-floating-point-behavior?view=msvc-170#fast
    "/fp:fast"

    -- generate auto vectorization reports
    -- /Qvec-report:1

    -- {"/FAsu", Config = "*-msvc-release-*"} --Creates a listing file containing assembler code
}

local lpp_link_common = {
    {
        "/FUNCTIONPADMIN",
        Config = "*-msvc-debug-*"
    },
    {
        "/OPT:NOREF",
        Config = "*-msvc-debug-*"
    },
    {
        "/OPT:NOICF",
        Config = "*-msvc-debug-*"
    },
    {
        "/DEBUG:FULL",
        Config = "*-msvc-debug-*"
    }
}

local clang_common = {
    "-march=haswell", -- Haswell should be compatible with the 8th console generation
    "-m64",
    "-Wthread-safety",
    "-g", -- debug info?
    "-pthread",
    "-fsanitize=address"
}

-- common link options for clang on linux (feed to compiler and linker)
local lld_common = {
    "-pthread",
    "-fsanitize=address"
}

Build {
    Passes = {
        CodeGeneration = {
            Name = "Generate sources",
            BuildOrder = 1
        }
    },
    Configs = {
        {
            Name = "win64-msvc",
            DefaultOnHost = "windows",
            Tools = {
                {
                    "msvc-vs2019",
                    TargetArch = "x64"
                }
            },
            Env = {
                CCOPTS = {
                    msvc_common,
                    "/std:c11"
                },
                CXXOPTS = {
                    msvc_common,
                    "/std:c++14",
                    "/EHsc" -- catch C++ exceptions only and tells the compiler to assume that functions declared as extern "C" never throw a C++ exception
                },
                PROGOPTS = {
                    lpp_link_common
                },
                GENERATE_PDB = {
                    "1",
                    Config = "*-msvc-debug-*"
                }
            }
        },
        {
            Name = "linux-clang",
            DefaultOnHost = "linux",
            Tools = {
                {
                    "clang-llvm",
                    Version = "14"
                }
            },
            Env = {
                -- drop LIBPREFIX prefix?
                -- drop SHLIBPREFIX prefix?
                CCOPTS = {
                    clang_common
                },
                CXXOPTS = {
                    clang_common
                },
                PROGOPTS = {
                    lld_common
                    -- "-lm", --math
                }
            }
        }
    },
    Units = {
        "units-vendor.lua",
        -- "units-conv.lua", -- convention based build utilities
        -- "units.lua" 
        "units-generated.lua",
        "units2.lua"
    }
}
