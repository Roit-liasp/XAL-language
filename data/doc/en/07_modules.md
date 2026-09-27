Chapter 7. Modules

7.1 Module, import, use and pub

It is convenient to break a large program into pieces. One file — one module.
At the beginning of the file, you specify its name:

  module(lib)

To use someone else's module, import it by file name without extension:

  import "mod/lib";

The word use does nothing yet - it is reserved for the future.
You may write lines like "use lib.answer;" - the compiler will
silently skip them.

The word pub opens a name to the outside. Without it a function or
a variable shows only in its own file:

  module(lib)
  pub let LV = 100;
  pub fn answer() { return 42; }
  let secret = 1; # visible only here

And in the main file after the import you may write plain LV and answer() -
short names get picked up on their own. Full ones work too: lib.answer().

7.2 Where the compiler looks for modules

The import string is a path to a file. The .xa extension may be skipped,
it gets added on its own. Folders are searched in this order:

- the folder holding the file with the import;
- the folder holding the main file;
- the current folder;
- the folder from the XAL_LIB environment variable, or /lib/xal
  when there is none.

When a module is found nowhere - compilation stops with a clear error.
One module imported twice connects only once - the second time nothing
happens.

7.3 How names are joined

There is one rule: an open name joins from the module name, a dot
and the name itself. Pub function answer in module lib becomes
lib.answer. With no module() in the file, the name stays as is.

When you write a short name, the compiler looks for it in this order:

- a local variable with that name;
- an open name of the current module;
- any open name with such a tail after the dot;
- otherwise - a plain global name.

The practical takeaway: in libraries always write module() at the top
and open only what is needed through pub. Then names never clash
and someone else's file will not break your variables.
