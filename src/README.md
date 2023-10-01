# Coding Guidelines

## Packages

Each folder is a package.

Each package typically has a common header `<package-name>.hh` but you can include any header from a package.

Public APIs use `PascalCase`

Package private APIs use `_PascalCase`, that is pascal case with a leading `_` (underscore).

Packages are not allowed to have cyclic dependencies. That is, when analyzing the dependencies of a package it should form a directed acyclic graph (DAG).

## Naming Convention

Type names, member functions and functions use `PascalCase`.

Builtin/primitive types like `int32_t` (abbreviated `i32`) use lower case `snake_case`.

Local variables and parameters use `snake_case`.

Data members use `snake_case_`, that is each word is delimited by `_` (underscore) and have an additional trailing `_` (underscore) to distinguish them from local variables and parameters. We rarely use `this` to qualify data member access.

## Plain Old Data (POD)

We mostly use blittable, trivially copyable, structs. The memory for these data types can be allocated from anywhere. We use `Create(...)` to initialize and `Destroy()` to cleanup.

We don't use access specifiers. All data members should be public but don't go access data members directly from outside a package without going tough a public API.
