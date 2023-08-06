# A game engine for experimenting with ECS and DX12

I use `tundra2` to build based on the following conventions.

Each folder in the `src` directory is a module. A module is compiled into it's own static library and will have 1 precompiled header. If the module name starts with `cmd_` then it's an executable. If the module name starts with `test_` it's a test. A test is just a bunch of functions to test some function or system.

## Structure

- `src` source files
- `scripts` utilities written in JavaScript, use Node.js to run.
- `vendor` external source files (and/or external libraries)

## How to build

I don't check-in the vendor source files because most of them are available online. You will need to download them and place them in the vendor directory.

You will need to download and install [Tundra](https://github.com/deplinenoise/tundra).

## How to dev

Will update with instructions on how to run and debug this project.
