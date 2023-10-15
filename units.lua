local path = require "tundra.path"
local util = require "tundra.util"

-- convention based build utilities
local conv = require "conv"

-- package depedencies 
local package_deps = conv.flatten_package_deps {
    ["libcommon"] = {
        "xxhash"
    },
    ["libecs"] = {
        "libcommon",
        "libcomponents"
    },
    ["libmath"] = {
        "dxmath"
    },
    ["libloader"] = {
        "libcommon",
        "png",
        "zlib"
    },
    ["game"] = {
        "libcommon",
        "libecs",
        "imgui",
        "dx12ext"
    },
    ["game2"] = {
        "libcommon",
        "libecs",
        "librenderer-dx12"
    },
    ["vdv"] = {
        "libcommon",
        "librenderer-dx12"
    },
    ["librenderer-dx12"] = {
        "libcommon",
        "libloader",
        "imgui",
        "dx12ext"
    },
    ["dx12"] = {
        "dx12ext"
    }
}

-- source files grouped by package name 
local packages = conv.glob_cxx("src")

for dir, files in pairs(packages) do
    local pkgname = path.get_filename(dir) -- naked dirname  

    if pkgname:find("^cmd_") then
        local cmdname = pkgname:sub(5)
        Program {
            Name = cmdname,
            Depends = {
                "win32", -- Windows specific???
                package_deps[cmdname] -- most be last in deps
            },
            Sources = {
                files
            }
        }
    else
        local sources, tests = conv.sources_and_tests(files)

        local libname = "lib" .. pkgname
        StaticLibrary {
            Name = libname,
            Depends = {
                package_deps[libname]
            },
            Sources = {
                sources
            }
        }

        for _, test in ipairs(tests) do
            local testname = pkgname .. "_" .. test[1]
            Program {
                Name = testname,
                Depends = {
                    "libtest",
                    "win32", -- Windows specific???
                    libname, -- implicit
                    package_deps[libname] -- most be last in depends
                },
                Sources = {
                    test[2]
                }
            }
        end
    end
end

Default "libcommon" -- must be something
