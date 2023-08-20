# A game engine using ECS and DX12

> ⚠️🚧🏗️ WORK IN PROGRESS ⚠️🚧🏗️
>
> I'm actively developing this for my own personal use. It is in a very early stage and many things are incomplete and will change.
>
> ⚠️🚧🏗️ WORK IN PROGRESS ⚠️🚧🏗️

## What is ECS?

ECS is an acronym for Entity-Component-System.

In short, it is just a memory allocator with a bigger API surface. Instead of `malloc` you have `EntityManger::CreateEntity` and instead of `free` you have `EntityManger::DestroyEntity`. Instead of a static `GameObject` type you have a runtime `Archetype` that group entities of the same component types together.

When you create an entity you create a chunk (a block of memory) to hold a number of entities. When you set component data for an entity, you are setting a value in an array of component data of the same type (SOA) inside some chunk.

To make things happen you write systems to coordinate with other systems and read/write component data in chunks. These systems can then be made to process those chunks in parallel.

All of this is important because CPUs are not really getting faster they are getting wider (they can do more things in parallel). Memory access is comparatively slow with respect to CPU throughput and the only way to offset this is to do speculative execution. Straightforward code and linear memory access is preferable because it is easier for the CPU to predict.

## What is DX12?

DX12 is a modern low-level graphics API. It is not cross platform like Vulkan but DX12 can run on Linux using Valve's Proton and on macOS using Apple's Game Porting Toolkit.

## How the project is organized

- `src` source files C and C++ (mostly C++)
- `data` asset files (including HLSL shader sources)
- `scripts` utilities written in JavaScript, use Node.js to run
- `vendor` external source files (and/or external libraries)

## How to build

I don't check-in the vendor source files because most of them are available online. You will need to download them and place them in the vendor directory.

You will need to download and install the VS 2019 Build Tools and [Tundra](https://github.com/deplinenoise/tundra). Tundra is a high-performance code build system with excellent incremental build times.

If you use VS Code you can access the build and launch tasks after generating the tasks.

Run `node .\scripts\generate-tasks.mjs` to generate `tasks.json`.

To build from command line you invoke tundra with a build target.

To list all build targets:

```
tundra2 -t
```

To build a build target `game`:

```
tundra2 game
```

By default it will build a debug target. If you want to build a release target you need to include the desired configuration first.

```
tundra2 win64-msvc-release game
```

## How to dev

You need a C/C++ toolchain, I use the older VS 2019 Build Tools (with Clang for diagnostics) since it has served me well and I have not found a reason to upgrade.

> If you want to use the companion VS Code extension `leidegre.leidegre-clang-diagnostics` for additional Clang diagnostics you need to launch VS Code with `set-env.cmd` to setup the environment.
>
> The `leidegre.leidegre-clang-diagnostics` extension will run the Clang frontend on your code with various options and report source level errors. Clang has in my opinion provided better error messages for common issues that Visual C++ has never supported.

It is also possible to build and run on WSL (Tundra is a cross platform build system). This has been useful for running address sanitizer and thread sanitizer.

## Some tools you might want to use

- RenderDoc (graphics debugger), https://renderdoc.org/
- Superluminal (profiler), https://superluminal.eu/
- RemedyBG (debugger), https://remedybg.itch.io/remedybg
