Chapter 6. Functions

6.1 How to declare a function

A function starts with the word fn, then a name, parens with parameters
and a body in curly braces:

  fn add(a, b) {
      return a + b;
  }

There may be no parameters at all:

  fn greet() {
      print("hi");
  }

The word pub before fn is for modules - it makes the function visible
from the outside. Chapter 7 covers that in detail.

6.2 Arguments and return value

A call passes arguments in order:

  let s = add(2, 3); # 5

The language does not complain about an incorrect number of arguments.
Arguments that are omitted default to zero, while extra arguments are silently discarded.
This is convenient for scripts but can sometimes mask typos, so be careful.
There is a limit of 32 arguments per call; the compiler will not allow more.

Return hands a value back. When a function reaches the end with no
return - it gives back 0. So every function always returns something,
at least a zero.

You may call a function before it is declared in the file - the compiler
collects all names first and checks later. But a call to a function that
does not exist will be found, and the program will not build. Two functions
with one name are an error too.

6.3 Main and code outside functions

The main function is the entry point, and it is required. Without it
the compiler says "no main function" and stops.

Code written plainly in the file outside functions is not lost either: it
is gathered into a hidden service function init and runs before
main. A cozy place for starting values:

  let factor = 10; # runs before main

  fn main() {
      print(5 * factor);
  }

Global lets may stand before or after functions - order does not matter,
all of them run before main starts, in written order.

6.4 Where variables are visible

The rules are simple:

- A variable declared inside a function is local. It lives only in that function and shows in its whole body, even inside nested blocks.
- A variable outside functions is global. All functions see it.
- Assigning inside a function to a name that exists among globals changes that global. When the name exists nowhere - a new local is born.

And the main trap for newcomers: reading a missing variable is not
an error. The language quietly creates a global with the value 0
and moves on. A typo in a name gives zero instead of a scolding. When
the program acts weird - check the names first.
