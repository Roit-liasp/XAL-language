Chapter 2. Lexis - what program text is made of

2.1 Comments

Comments come in two flavors - a hash or a double slash.
Everything to the end of the line is ignored:

  # this is a comment
  // this is a comment too
  print("hi"); # comment after code

There are no nested or multi-line comments. Not needed - it is fine
as it is.

2.2 Names

A name starts with a letter or an underscore, then letters, digits and
underscores may follow. Examples of fine names: counter, x2, _tmp,
MY_LIST. Case matters: counter and Counter are two different names.

Module names join with a dot, for example lib.answer. A dot inside
a name is the only exception, modules are covered in chapter 7.

2.3 Keywords

Here is the full list of words taken by the language that cannot be names:

  fn let if else while for return break continue
  true false nil and or not use in
  module import pub case default

Twenty two in total, easy to memorize, because almost all of them are
control structures you already know from other languages.

2.4 Numbers

Numbers in XAL are integers only. Decimal numbers look normal, and
hexadecimal ones take the 0x prefix:

  let a = 42;
  let b = 0xFF; # same as 255

No floating point. Negatives go through a minus in front: -5. That is
not part of the number but a negation operation, yet it works exactly
how everyone expects.

2.5 Strings

Strings take double or single quotes - no difference:

  let s1 = "hello";
  let s2 = 'hello to you too';

Escapes work inside: \n newline, \t tab, \r carriage return, \0 zero
byte, \\ the slash itself, \" and \' quotes. Any other letter after
a slash means itself: \d is just d.

A string may span several text lines - the newline simply becomes part
of it. But a forgotten closing quote will not be forgiven: you get
an "unterminated string literal" error.

2.6 Punctuation in code

There are eleven two-character operators in total:

  == != <= >= && || += -= *= /= ..

Everything else is single characters: + - * / % < > = ! ( ) { } [ ] , ; .
and so on. Spaces, tabs and newlines matter only as separators:
you can write everything on a single line or stretch it out over a hundred; it’s a matter of taste.
