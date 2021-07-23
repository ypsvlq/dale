# Dale

Dale is a build system for C.

## Building

Dale builds using itself. If you don't have an installation of Dale, use your
favourite C compiler to compile everything in `src/` and the relevant file from
`src/host/` for your platform.

On POSIX systems that use [pkg-config][1], install the `extra/dalereq` script
along with dale to allow automatically discovering dependencies.

[1]: https://www.freedesktop.org/wiki/Software/pkg-config/

## Using

To build a project using dale, run `dale`. MSVC users should do this from
inside a Tools Command Prompt.

### Setup

To set up a project for building with dale, create a `build.dale` file where
task definitions are placed. For storing settings local to your computer, use
`local.dale` (it is recommended to add this and the `build/` directory to your
VCS's ignore list).

### Variables

To declare a variable, write

	name = value

An existing variable can be appended to using

	name += value

Both of these will expand `value` for variables before `name` is set, according
to the following rules:

- `$$` expands to a single `$` (for escaping)
- `$var` and `$(var)` expand to the value of `var`
- `$[var text]` expands to `text` if `var` is equal to `1`
- `$[!var text]` expands to `text` if `var` is not equal to `1`

Expansions that use brackets around the contents allow further expansions
inside the brackets, eg. `$($(varname))`.

### Tasks

Each task declares an executable or library to be built. They are declared
with a line of the form

	type(name)

where `type` can be `exe`, `lib`, or `dll`.

Indented lines that follow a task declaration specify taskvars, which are
similar to variable declarations but must be indented with at least one tab or
space, use a colon to separate the name and value instead of an equals sign,
and repeated declarations append rather than overwriting.

The following taskvars are available:

- `src` - source files
- `inc` - directories to add to the compiler's include path
- `def` - preprocessor definitions to pass to the compiler
- `lib` - system libraries to link against
- `req` - required non-system libraries

When setting paths, the `*` and `?` wildcard characters can be used to match
any sequence of characters that fits or a single character respectively.

Libraries specified in the `req` taskvar will be searched for via `dalereq` if
it exists, otherwise the user can manually specify which compiler flags and
libraries to pass via the `name_CFLAGS` and `name_LIBS` variables.

### Comments

Use the `#` character at the start of a line (or after whitespace) to ignore
anything up to the next newline. Multiline comments and comments midway through
a line are not supported.
