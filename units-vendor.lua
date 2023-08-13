StaticLibrary {
    Name = "imgui",
    Sources = {
        "vendor/imgui-1.89.8/imgui.cpp",
        "vendor/imgui-1.89.8/imgui_demo.cpp",
        "vendor/imgui-1.89.8/imgui_draw.cpp",
        "vendor/imgui-1.89.8/imgui_tables.cpp",
        "vendor/imgui-1.89.8/imgui_widgets.cpp",
        "vendor/imgui-1.89.8/backends/imgui_impl_dx12.cpp",
        "vendor/imgui-1.89.8/backends/imgui_impl_win32.cpp"
    },
    Includes = {
        "vendor/imgui-1.89.8",
        "vendor/imgui-1.89.8/backends"
    },
    Propagate = {
        Includes = {
            "vendor/imgui-1.89.8",
            "vendor/imgui-1.89.8/backends"
        }
    }
}

StaticLibrary {
    Name = "zlib",
    Sources = {
        "vendor/zlib-1.2.13/adler32.c",
        "vendor/zlib-1.2.13/compress.c",
        "vendor/zlib-1.2.13/crc32.c",
        "vendor/zlib-1.2.13/deflate.c",
        "vendor/zlib-1.2.13/gzclose.c",
        "vendor/zlib-1.2.13/gzlib.c",
        "vendor/zlib-1.2.13/gzread.c",
        "vendor/zlib-1.2.13/gzwrite.c",
        "vendor/zlib-1.2.13/infback.c",
        "vendor/zlib-1.2.13/inffast.c",
        "vendor/zlib-1.2.13/inflate.c",
        "vendor/zlib-1.2.13/inftrees.c",
        "vendor/zlib-1.2.13/trees.c",
        "vendor/zlib-1.2.13/uncompr.c",
        "vendor/zlib-1.2.13/zutil.c"
    },
    Includes = {
        "vendor/zlib-1.2.13"
    },
    Defines = {
        "PNG_USER_MEM_SUPPORTED"
    },
    Propagate = {
        Includes = {
            "vendor/zlib-1.2.13"
        }
    }
}

StaticLibrary {
    Name = "png",
    Depends = {
        "zlib"
    },
    Sources = {
        "vendor/lpng1639/png.c",
        "vendor/lpng1639/pngerror.c",
        "vendor/lpng1639/pngget.c",
        "vendor/lpng1639/pngmem.c",
        "vendor/lpng1639/pngpread.c",
        "vendor/lpng1639/pngread.c",
        "vendor/lpng1639/pngrio.c",
        "vendor/lpng1639/pngrtran.c",
        "vendor/lpng1639/pngrutil.c",
        "vendor/lpng1639/pngset.c",
        "vendor/lpng1639/pngtest.c",
        "vendor/lpng1639/pngtrans.c",
        "vendor/lpng1639/pngwio.c",
        "vendor/lpng1639/pngwrite.c",
        "vendor/lpng1639/pngwtran.c",
        "vendor/lpng1639/pngwutil.c"
    },
    Includes = {
        "vendor/lpng1639"
    },
    Propagate = {
        Includes = {
            "vendor/lpng1639"
        }
    }
}

ExternalLibrary {
    Name = "xxhash",
    Includes = {
        "vendor/xxHash-v0.8.2"
    },
    Propagate = {
        Includes = {
            "vendor/xxHash-v0.8.2"
        }
    }
}

ExternalLibrary {
    Name = "dx12ext",
    Includes = {},
    Propagate = {
        Includes = {
            "include/directx"
        },
        Libs = {
            "d3d12.lib",
            "dxgi.lib",
            "dxguid.lib",
            "D3DCompiler.lib"
        }
    }
}
