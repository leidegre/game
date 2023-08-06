-- convention based build utilities
module("conv", package.seeall)

local native = require "tundra.native"
local path = require "tundra.path"
local glob = require "tundra.syntax.glob"

function flatten_package_deps(package_deps)
    local function visit_package_deps(deps, visited)
        -- if there's a cycle here that's wrong. we should detect that...
        for _, dep in ipairs(deps) do
            if not visited[dep] then
                visited[dep] = true
                local deps2 = package_deps[dep]
                if deps2 then
                    visit_package_deps(deps2, visited)
                end
            end
        end
    end
    local package_deps2 = {}
    for package, deps in pairs(package_deps) do
        local visited = {}
        visit_package_deps(deps, visited)
        if visited[package] then
            print("warning: circular package dependency!")
        end
        local flattened = {}
        for k, _ in pairs(visited) do
            flattened[#flattened + 1] = k
        end
        package_deps2[package] = flattened
    end
    return package_deps2
end

function glob_cxx(root)
    local files = glob.Glob {
        Dir = root,
        Extensions = {
            ".cc",
            ".c"
        }
    }

    local groups = {}

    for _, file in ipairs(files) do
        local dir = path.get_filename_dir(file)

        local group = groups[dir]
        if not group then
            group = {}
            groups[dir] = group
        end

        group[#group + 1] = file
    end

    return groups
end

local HOST_WINDOWS = native.host_platform == "windows"
local HOST_LINUX = native.host_platform == "linux"

-- seperate sources and tests; filter out platform specific files
function sources_and_tests(files)
    local sources = {}
    local tests = {}

    for _, file in ipairs(files) do
        local basename = path.get_filename_base(file)
        if basename:find("_windows$") then
            if HOST_WINDOWS then
                sources[#sources + 1] = file
            end
        elseif basename:find("_linux$") then
            if HOST_LINUX then
                sources[#sources + 1] = file
            end
        elseif basename:find("_test$") then
            tests[#tests + 1] = {
                basename,
                file
            }
        else
            sources[#sources + 1] = file
        end
    end

    return sources, tests
end
