# Dale

Dale is a build system.

### Installation

Dale builds using itself. If you don't have a copy of Dale, use your
favourite C compiler to compile everything in `src/` and the relevant file from
`src/host/` for your platform.

After placing the Dale binary on your PATH, a configuration file must be created
before it can be used to build software. An example suitable for most systems is
provided in `global.dale`, which can be placed in the following directories:

POSIX:

- `~/.config/dale/`
- `/etc/dale/`

Windows:

- `%APPDATA%/Dale/`

On platforms that have multiple directories in the search path, the first one
listed above is special as it the only directory searched for files named with
the `-g` option.

### Usage

To build a project using dale, run `dale`. MSVC users should do this from
inside a Tools Command Prompt.

Various command-line options are supported, which are explained in the output
of `dale -?`.

To store settings local to a project, create a `local.dale` file, which will be
executed before `build.dale` (but after `global.dale`) unless the `-l` option
is used to specify a different one.

### Setup

To set up a project for building with dale, create a `build.dale` file where
task definitions are placed. If you are using a VCS, it is suggested to add the
`build/` directory and `local.dale` file to your ignore list.

## Language

Dale uses a simple DSL to describe projects and build processes, which is
documented here.

### Comments

Use the `#` character at the start of a line (or after whitespace) to ignore
anything up to the next newline. Multiline comments and comments midway through
a line are not supported.

### Variables

To set a variable, write

	name = value

A variable can be appended to using

	name += value

For setting a variable only if it has not been set, use

	name ?= value

All of these will expand `value` for variables before `name` is set, according
to the following:

- `$$` expands to a single `$` (for escaping)
- `$var` and `$(var)` expand to the value of `var`
- `$[var ...]` expands to `...` if `var` is equal to `1`
- `$[!var ...]` expands to `...` if `var` is not equal to `1`
- `${cmd ...}` expands to the result of a builtin command

Expansions that use brackets around the contents allow further expansions
inside the brackets, eg. `$($(varname))`.

#### System variables

System variables are variables used internally by Dale, which have their names
prefixed with an underscore. They are used to configure some aspects of the
build:

- `_jobs` - number of processes to spawn for running commands (defaults to the
  number of CPU cores)
- `_bscript` - path to the buildscript (defaults to `build.dale`)

### Tasks

Tasks are the unit in which Dale builds software. For a typical C-style build,
each task will declare either an executable or library to be built. They are
declared with one of the following lines

	type(name)
	build.type(name)

(if the first form is used, the build process defaults to `c`).

Indented lines that follow a task declaration specify taskvars, which are
similar to variable declarations but must be indented, use a colon to separate
the name and value, and repeated definitions append rather than overwriting.

#### Standard task format

*This section describes standard practice, but is not enforced by Dale.*

The build type can be `exe`, `lib`, or `dll`.

The following taskvars are available:

- `src` - source files
- `inc` - directories to add to the compiler's include path
- `def` - preprocessor definitions to pass to the compiler
- `lib` - system libraries to link against
- `req` - required non-system libraries

When setting paths, the `*` and `?` wildcard characters can be used to match
any sequence of characters that fits or a single character respectively.

### Builds

Build processes are specified with a `build` and one or more `rule`
declarations. These take a similar format to tasks:

	build(name)
		# ...

	rule(buildname.Rulename)
		# ...

The contents of a build is a series of statements which could appear at the
top level of a Dale script (i.e. variable assignments and builtin calls).

Each line of a rule is a command which will be executed by the OS's command
processor. It will be expanded for variables before being run, and will have
the extra variables `$out` and `$in` set to the output file and input file(s).

### Builtins

The following builtins are available for use in `${}` variable expansions.

- `${map arr expr}` - expands to the result of expanding `expr` for each item
  in `arr` with `$_` set to the current item
- `${glob pattern}` - expands to all paths matched by `pattern`
- `${stripext path}` - expands to `path` without its file extension

These builtins are invoked outside of variable expansions by prefixing them
with an `@`:

- `@dump var` - prints the value of `var` (for debugging)
- `@do rule out in` - inside a build definition, runs the rule `rule` with `$in`
  and `$out` set to the named variables (if both variables are arrays, it will
  be run for each pair of elements in them)
- `@mkdirs arr` - for each path in `arr`, creates all needed directories to hold
  that file
