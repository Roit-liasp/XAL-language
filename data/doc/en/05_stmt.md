Chapter 5. Statements

5.1 Let and assignment

A new variable is born with the word let:

  let age = 30;
  let name; # no value - will be 0

A repeated let with the same name is not an error - it just puts a new
value into the same variable. Assigning to an existing variable goes
without let:

  age = 31;

Short forms exist: += -= *= /=. The line "x += 2" means "x = x + 2".
But there is no short form for remainder or gluing - write those in full.

A list item changes by index:

  l[0] = "new";

There can be as many indexes as you need - nested lists change the same
way:

  m[0][1] = "new";
  l[0] += 2;          // short forms work by index too

When the index runs past the end of the list, the list grows, and the new
holes fill with zeroes. Assigning by index into a string is silently
ignored: strings change only as a whole.

5.2 If and else

The condition is any expression, parens are optional:

  if age >= 18 {
      print("access granted");
  } else {
      print("come back later");
  }

Chains go through else if - as many as you like:

  if score > 90 { print("great"); }
  else if score > 70 { print("good"); }
  else { print("could be better"); }

Truth and falsehood for conditions are defined in topic 3.2: zero, empty
string and empty list count as false.

5.3 Case

Case is a tidy pick from several options with no ladder of ifs:

  case weekday {
      1, 2, 3, 4, 5 { print("workdays"); }
      6, 7 { print("weekend"); }
      default { print("no such day"); }
  }

The expression after case compares with each value through ==.
One branch may list several values separated by commas.
The default branch is optional - with no default and no match, simply
nothing happens.

5.4 While

The "as long as" loop:

  let i = 0;
  while i < 5 {
      print(i);
      i += 1;
  }

When the condition is false right away - the body never runs. A forgotten
counter gives an endless loop, so always change what you check.

5.5 For

For walks over a list or over a string:

  for name in ["Anna", "Bob", "Kim"] {
      print("hello, " .. name);
  }

  for letter in "cat" {
      print(letter); # "c", then "a", then "t"
  }

The loop variable takes the next item each time. An empty list or an empty
string means the body never runs. To leave early use break, to skip
a round use continue.

5.6 Break, continue, return

Three words to steer the flow:

- break - leave the nearest while or for at once.
- continue - drop the current round and jump to the next one.
- return - go back from a function, maybe with a value.

Example:

  for x in data {
      if x < 0 { continue; }
      if x == 0 { break; }
      print(x);
  }

A bare return gives back 0. Break and continue outside a loop are
a compile error, the compiler will tell you right away.

5.7 Blocks and semicolons

Curly braces group statements into a block:

  {
      let v = 1;
      print(v);
  }

Important: a block does NOT hide variables. Every let inside a function
is visible in the whole function, even when declared deep inside blocks.
Braces just say "this goes together", they are not a new scope.

The semicolon after statements is optional. Write with it or without -
as you like. An empty statement of one semicolon is allowed too and does
nothing.
