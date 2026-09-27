Chapter 3. Data types

3.1 Only three types

XAL has no dozens of types to give you a headache. Exactly three:

- int - an integer. Examples: 0, 42, -7.
- str - a string. Examples: "hello", "".
- list - a list. An ordered pack of any values, even
  mixed ones: [1, "two", [3]].

You never declare types - a variable just holds whatever you put into it.
A number today, a string tomorrow, the language does not care.

The words true, false and nil are not separate types. True becomes
1, while false and nil become 0. Handy: you write readable code, while
plain numbers live inside.

3.2 What counts as true

In if and while conditions, and in and, or and not, one simple rule holds:

- number 0 is false, any other number is true;
- empty string "" is false, a non-empty one is true;
- empty list [] is false, a list with items is true.

So you can write like this and it will work as expected:

  if name { print("name given"); }
  while jobs { work(); }

3.3 How values convert

Two built-in functions do it explicitly:

- str(x) - turns anything into a string. A list becomes
  something like "[1, two]".
- int(x) - turns into a number. A string parses as a number ("42" gives 42,
  "garbage" gives 0), a list gives its length.

And there are places where conversion happens on its own. Plus glues
strings when at least one side is a string. Comparisons == and != compare
as text when at least one side is a string. Ordering (<, > and friends)
always compares numbers. Details in chapter 4.
