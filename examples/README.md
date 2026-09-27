Examples of programs in XAL

РУССКИЙ

- 01_hello - самая простая программа, "Hello World".
- 02_interactive - интерактивный эхо-бот, общается до слова выхода.
- 03_ветвление - ветвления: лесница if/else и выбор случая.
- 04_files - работа с фаилами: пишет, читает, переименовывает, удаляет.
- 05_modules - программа из нескольких модулей: main+calc+text.
- 06_library - библиотека strlib и demo, которое она использует.

Как запустить (из корня проекта, после make all): 

  build/xalc -o /tmp/hello.xb example/01_hello/hello.xa 
  build/xvm/tmp/hello.xb

Точно так же собираются остальные, в начале каждого .ха фаила
написаны точные команды. Для 03_разветвления нужные аргументы,
пример: build/xvm /tmp/check.xb 85 6. Primer 04_files создаёт
папку /tmp/xal_files_demo и сам убирает за собой фейл.

ENGLISH

- 01_hello - the simplest program, "hello, world".
- 02_interactive - an interactive echo bot, chats until the word exit.
- 03_branching - branching: an if/else ladder and a case choice.
- 04_files - file work: writes, reads, renames, deletes.
- 05_modules - a program of several modules: main + calc + text.
- 06_library - the strlib library and a demo that uses it.

How to run (from the project root, after make all):

  build/xalc -o /tmp/hello.xb examples/01_hello/hello.xa
  build/xvm /tmp/hello.xb

The rest build the same way: exact commands sit at the top of every .xa
file. Program 03_branching needs arguments, for example:
build/xvm /tmp/check.xb 85 6. Example 04_files creates
the /tmp/xal_files_demo folder and cleans its files up by itself.
