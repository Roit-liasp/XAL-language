Chapter 4. Expressions

4.1 Order of operations

XAL operators go from weakest to strongest binding in this order:

1. or: ||, or
2. and: &&, and
3. comparisons: == != < <= > >=
4. addition and gluing: + - ..
5. multiplication: * / %
6. primary: numbers, strings, parens, calls, indexes, not, minus

You may write and, or and not as words - for those who dislike
symbols. Parens work as everywhere and save you when the order is unclear:

  let x = (a + b) * c;
  if (age > 18) and pass { enter(); }

4.2 Arithmetic and gluing strings

Five operations: + - * / %. Division is integer: 7 / 2 gives 3.
Remainder: 7 % 2 gives 1. Unary minus flips the sign: -x.

Two important surprises, both pleasant:

- Division and remainder by zero do not crash the program, they give 0.
  Do not ask why - the authors decided so, to keep scripts from falling.
- Plus can glue strings: "cat" + "dog" gives "catdog". When at least
  one side is a string, the other quietly becomes a string too.

And the ".." dots are honest gluing, always: they turn both sides into
strings and join them. When you want gluing and not math, write two dots:

  let s = "answer: " .. 42; # "answer: 42"

4.3 Comparisons

All six as usual: == != < <= > >=. The result is always 1 or 0.

A subtle point: == and != look at types. When at least one side is
a string, the comparison goes by text. So "42" == 42 is true, because
the number 42 turns into the string "42". Ordering - <, >, <=, >= - works
the same way: when at least one side is a string, the two are compared as
strings, so "10" < "9" is true (alphabetically, not numerically). Numbers
against numbers compare as numbers.

  if answer == "yes" { print("agreed"); }
  if height > 170 { print("tall"); }

4.4 Logic - and, or, not

Three operations on truth and falsehood:

- a and b (or a && b) - true when both are true.
- a or b (or a || b) - true when at least one is true.
- not a - a flip: turns truth into falsehood and back.

And and or are lazy: the second side runs only when the first one was not
enough for the answer. So this is safe to write:

  if items and items[0] == "start" { print("go"); }

Here items[0] is never touched when the list is empty - no error.

An honest warning for the curious: as values, and and or do not always
return 1 and 0. And returns the second side when the first is true, else 0.
Or returns 1 when the first is true, else the second side. In conditions
you never notice it, but in arithmetic it is better not to rely on it.

One more thing: not and unary minus bind very tightly, only to the next
value. The line "not a == b" reads as "(not a) == b", not as
"not (a == b)". To negate the whole comparison - add parens.

4.5 Calls inside expressions

A call is a name plus parens with arguments:

  let d = length("hello");
  print(add(2, 3), "done");

From zero to 32 arguments allowed. Calls may nest as deep as you like.
Your own functions look just like built-in ones - and that is good,
less to memorize.

4.6 Lists and square brackets

A list is born in square brackets:

  let empty = [];
  let mixed = [1, "two", [3, 4]];

Indexes start at zero. On lists they return an item, on strings -
a character as a one-character string:

  let l = ["a", "b", "c"];
  print(l[0], l[2]); # "a" "c"
  let s = "hello";
  print(s[1]); # "e" - the second letter

Brackets may chain: matrix[1][0] takes item zero of list one. Going out
of bounds does not crash: lists give 0, strings give an empty string.
Handy for scripts, where emptiness beats a crash.
