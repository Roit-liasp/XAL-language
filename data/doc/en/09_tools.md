Chapter 9. Tools - XALC and XVM

9.1 XALC - the compiler

The xal compiler turns a .xa source into .xb bytecode. Three modes:

  xalc -o program.xb program.xa  # build
  xalc program.xa                # same, the .xb name is picked on its own
  xalc -d program.xb             # show the disassembler
  xalc -v                        # show the version

With no -o the output name comes from the input one by swapping .xa
for .xb. Compile errors print as "xal: file:line: what happened"
and the build stops. Common causes: no main function, a call to
a missing function, two functions with one name, a module not found,
forgotten brackets or quotes.

9.2 XVM - the virtual machine

The xvm machine runs a ready .xb:

  xvm program.xb arg1 arg2

Arguments after the .xb name reach the program through argv and argc.
The --no-xmc flag turns the garbage collector off - rarely needed, mostly
for debugging and measuring.

There is a tricky mode: when xvm runs under a different name (through
a symlink, for example), it finds a file with that name and the .xb
extension in XAL_LIB or /lib/xal by itself and runs it. That is how scripts
become commands called by name.

9.3 Exit codes and error messages

What main returned - cut down to a byte - becomes the exit code of
the whole program. Returned 3 means code 3. Runtime errors print as
"xvm: what happened" and end the run. For example:

  xvm: spawn_thread: no such function

A broken or truncated .xb file never runs: the machine says "bad bytecode
image" with a reason and exits. It will not silently run garbage.

9.4 Bare metal and embedding

XVM can work even where there is no operating system. The target:

  make bare

builds the xvm_bare_demo example: bytecode bakes right into the binary
together with the machine. In this mode there are no files, folders,
processes, terminal or threads - only computing, memory and output.

For your own projects the mechanics go like this: the tests/embed.sh script
turns .xb into a C array of bytes, and the xvm_run_mem function runs
bytecode straight from memory. Handy when you need a script engine inside
a big C program with no extra dependencies.
