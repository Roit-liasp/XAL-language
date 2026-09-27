Chapter 10. Bytecode

10.1 What is inside .xb

A .xb file is a memory image for the machine. It starts with the four-byte
magic "XALB", then a version number (1 right now), then four sections:

- the string table - all string constants of the program;
- the function table - for each one: entry point, parameter count,
  local frame size and name;
- the header - global count, numbers of the main function and
  the service init function, code size;
- the code itself - a run of instructions, each with an operation code
  and one argument.

Limits sew into the format: no more than 65535 functions, no more than 32
call arguments, code and strings not endless. The machine checks the image
on load and rejects everything suspicious instead of crashing.

10.2 Instructions

Instructions are simple, almost all of them work on the stack:

- pushi, pushs - put a number or a string on the stack.
- load, store - take or put a local variable.
- gload, gstore - take or put a global variable.
- add, sub, mul, div, mod, neg - math and minus.
- eq, ne, lt, le, gt, ge, not - comparisons and negation.
- jmp, jz, jnz - jumps: always, on false, on true.
- call, ncall - call a plain or a built-in function.
- ret, pop, dup - return a value, drop the top, double the top.
- newlist, append, index, setidx, len - lists and strings work.
- concat - gluing into a string.
- nop, halt - do nothing and stop.

With call and ncall the argument packs tricky: the low 16 bits hold
the function number, the high 16 bits hold the argument count. That is why
there cannot be more than 65535 functions.

10.3 Disassembler

The -d flag shows what the compiler put into .xb:

  xalc -d program.xb

First goes a header line: version, how many strings, functions,
globals and instructions, numbers of main and init. Then the function list
with parameters, locals and entry points. And then the code itself: number
of the instruction, operation name and argument. With pushs you see
the string itself, with call and ncall - the function name and count
of arguments.

The disassembler is your best friend when the program misbehaves: you see
right away what code the compiler made and where the logic went wrong.
