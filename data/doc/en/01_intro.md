Chapter 1. Introduction

1.1 Meet XAL

XAL is a small language designed for embedded systems, utilities, and experimentation.
It features just three data types, familiar `if`, `while`, and `for` constructs, as well as functions, modules, and threads.
XAL code is compiled into bytecode, which is then executed by a virtual machine (similar to Java and the JVM).
The language was intentionally designed to be simple: you can read through its documentation in an evening and start writing code in it after just an hour.

Source files usually end with .xa, and compiled bytecode ends with .xb.

1.2 Three names - XAL, XALC, XVM

This is easy to mix up, so let us sort it out right away:

- XAL - the programming language itself, its syntax and rules.
- XALC - the compiler of that language. The xal program takes a .xa file and turns it into .xb bytecode.
- XVM - the virtual machine. The xvm program takes a .xb file and runs it.

The usual chain looks like this: write .xa, compile to .xb, run it
under xvm and get the result.

1.3 First program

Create a file hello.xa with this text:

  fn main() {
      print("Hello World");
  }

Now compile and run it:

  xalc -o hello.xb hello.xa
  xvm hello.xb

You will see "Hello World" on the screen. The whole program is the main
function, and it is required in every project. Whatever main returns becomes
the exit code of the whole program.

1.4 How to build and run

Building the XAL kit itself is easy.
Two commands are enough:

  make all
  make test

The first builds xal and xvm into the build folder, the second runs the
tests and shows "ok" next to each one. If everything is "ok" - the kit
works. The conf.sh script detects your system and flags on its own,
you usually do not need to touch anything. Ready-made examples live
in the examples folder: from hello world to your own library.
What is where - see examples/README.md.

1.5 License and support

The project ships under the BSD 2-Clause license. The full text lives
in the LICENSE file in the root. Short version: you may use, change and
embed it even in closed products, but keep the copyright notice and the
no-warranty disclaimer.

Long-term support for the language is not guaranteed. This simply means that
development continues and bugs are fixed, but no one
promises a date for the next release or a specific timeframe for fixes and updates. If you find
a bug, report it, and it will be looked into whenever time permits.
