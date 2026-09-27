Chapter 8. Built-in functions

Built-in functions are called natives here. No need to declare or import
them - just call. Below are all of them, sorted onto shelves. Parens hold
arguments, then comes what the function returns.

8.1 Printing and input

- print(a, b, ...) - prints all arguments separated by spaces, with a newline at the end. Returns 0.
- puts(a, b, ...) - prints everything at once, with no spaces and no newline. Returns 0.
- input() - reads a line from the console without the newline. At end of file it raises the eof flag.
- eof() - returns 1 when input already hit end of file.
- read(fd, howmany) - reads a chunk from a descriptor. Defaults are fd 0 (console) and 4096 bytes. Returns a string.
- write(fd, s) - writes a string into a descriptor. Returns the number of bytes written.
- open(path, mode) - opens a file. Mode "r" reads, "w" writes with truncation, "a" appends. Returns a descriptor or a negative.
- close(fd) - closes a descriptor.

8.2 Strings

- len(x) - length of a string or a list. With no arguments gives 0.
- str(x) - turn into a string.
- int(x) - turn into a number.
- split(s, sep) - cuts a string by a separator, a space by default. Returns a list.
- join(list, sep) - glues a list into a string through a separator, a space by default. A non-list gives an empty string.
- substr(s, start, length) - a piece of a string. Borders trim themselves.
- find(s, sub) - position of a substring or -1.
- eq(a, b) - 1 when strings are equal as text.
- ord(s, i) - code of the character at position i or -1.
- chr(n) - a one-character string with that code.
- trim(s) - drops spaces and newlines from both ends.
- upper(s), lower(s) - upper and lower case, English letters only.
- replace(s, what, with) - replaces every occurrence. An empty "what" changes nothing.
- startswith(s, head), endswith(s, tail) - edge checks, 1 or 0.
- repeat(s, n) - repeats a string n times.

8.3 Lists

- sort(list) - returns a NEW sorted list, the old one is untouched. Sorts as strings. A non-list comes back as is.
- copy(list) - a shallow copy of a list. A non-list gives [].
- insert(list, where, what) - inserts into place, changing the list itself. Returns the same list.
- remove(list, where) - drops an item, changing the list itself.

8.4 Numbers and time

- abs(x) - absolute value.
- min(a, b), max(a, b) - the smaller and the larger.
- now() - current time in seconds.
- sleep(ms) - a pause in milliseconds. Returns 0.

8.5 Files and folders

- exists(path) - 1 when the path exists.
- isdir(path) - 1 when it is a folder.
- isexec(path) - 1 when it is a runnable file.
- mtime(path) - change time in seconds or -1.
- mkdirp(path) - creates a folder with parents. 0 means it worked.
- unlink(path) - deletes a file. 0 means it worked.
- rename(old, new) - renames. 0 means it worked.
- readdir(folder) - names inside a folder. Current one by default.
- readfile(path) - the whole file as one string, or "" when unreadable.
- writefile(path, data) - writes a string into a file. Returns the byte count or -1.

8.6 Environment and paths

- getenv(key) - value of an environment variable or "".
- setenv(key, value) - sets a variable.
- getcwd() - the current folder.
- chdir(path) - moves into a folder. 0 means it worked.
- which(name) - looks for a program in PATH. Returns the full path or an empty string.
- argv(i) - the i-th command line argument, argv(0) is the program itself.
- argc() - how many arguments in total.

8.7 Processes

- exec(list) - runs a program and waits for the end. The first item is the command, the rest are arguments. Returns the exit code. A string instead of a list means a command with no arguments. A name with no slash is looked up in PATH.
- spawn(list) - runs and does not wait. Returns the process number.
- wait(pid) - waits for a process and returns its code.
- waitpid(pid, nohang) - the same with a flag: pass 1 to not wait and get -2 while the process is still running. -1 means there is no such process (or the host cannot tell).
- getpid() - number of the current process, -1 when there are none.
- kill(pid, signal) - send a signal (15, SIGTERM, by default). 0 on success, -1 on failure.
- fork() - a copy of the process: 0 in the child, the child number in the parent, -1 where fork does not exist (Windows, bare metal, kernels without fork). Right after fork the child must call execv: two live copies of the virtual machine is a breakage, not multitasking.
- pipe() - a channel: a list [read, write]. An empty list on failure.
- dup2(old, new) - replace a descriptor. 0 on success, -1 on failure.
- signal(sig, mode) - 0 (XOS_SIG_DFL, default) or 1 (XOS_SIG_IGN, ignore). -1 when the host has no signals.
- sys(command) - runs a string through the shell. Returns the code.
- execv(list) - replaces the current process with a new program. On success it never comes back.
- exit(code) - ends the program with that code. When the program was started nested (run_xb), only that nested image ends, not the whole machine.

8.8 Terminal

- rawmode(on) - turns raw terminal mode on or off. On by default.
- getkey() - reads one byte from the console or -1.
- termsize() - a list of two numbers: terminal width and height.
- isatty(fd) - 1 when the descriptor is a terminal. Fd 0 by default.

8.9 Threads and channels

- spawn_thread(function, arguments) - runs a function in a new thread. The function is named by a string with its name or by a number. Arguments go as a list. Returns the thread number or -1. With no such function it scolds into the console and returns -1.
- join_thread(id) - waits for a thread and returns what the function gave.
- thread_id() - number of the current thread.
- thread_cpu(), set_affinity() - stubs: 0 and -1.
- nproc(), cpu_count() - how many CPUs are seen. Both give the same.
- chan_new() - a new channel, returns its number.
- chan_send(channel, value) - puts a value into a channel. 0 is success.
- chan_recv(channel) - takes a value, waits when empty.
- chan_tryrecv(channel) - takes with no waiting, gives 0 when empty.
- chan_close(channel) - closes a channel.
- parallel_map(function, list) - applies a function to every item, working in several threads. Returns a new list.

8.10 Memory and garbage collector

Strings and lists live under the watch of a garbage collector: memory frees
itself when values become unreachable. You usually need no meddling,
but three handles exist:

- gc_collect() - collect garbage forcibly, right now.
- gc_count() - how many collections happened already.
- gc_stat() - a list of three numbers: is the collector on, how many collections happened, how many bytes are busy now.

8.11 Running images and host capabilities

- run_xb(path, arguments) - runs a .xb image and waits for the end.
  Arguments are a list of strings, the first one being the program
  name (as in exec); omit it and the path itself is passed as argv[0].
  Returns the exit code (0..255), -1 when it could not start, 126 when
  the image needs something this host cannot do.
  The image runs nested, inside the same virtual machine - which is
  why it works both on Unix and inside a kernel with no fork/exec.
  When a nested run is impossible (depth exhausted, forbidden by the
  host) but processes can be spawned, xvm hands the image to an
  external interpreter: its own argv[0] or whatever XAL_VM points at.
  Nesting depth is limited (XVM_NEST_MAX, 8 by default).
- caps() - a list of the capabilities this host has.
- has_cap(name) - 1 when the capability is there. Names: io, fs, dir,
  env, cwd, proc, kill, fork, pipe, signal, term, threads, time, xb.

xvm gets embedded anywhere: into a Unix process, into a kernel with no
fork/exec, onto bare metal. Every native function declares the
capabilities it needs; before running, xvm looks at what the image
requires and refuses to start it (code 126 and a readable message)
instead of crashing halfway. An image can be checked without running:

    xvm --caps program.xb

To run an image in the background, use the process tools: `fork()`, and
in the child `run_xb()` with `exit()` (right after fork the child must
start the image and leave: two live copies of the virtual machine is a
breakage):

    let pid = fork();
    if pid == 0 {
        exit(run_xb("/bin/util.xb", ["util", "arg"]));
    }
    let code = wait(pid);

A kernel that embeds xvm declares its own capabilities: either at
build time (-DXVM_CAPS_DEFAULT=...) or at runtime with
xvm_caps_disable() for everything it certainly lacks. XVM_CAPS_STRICT=0
turns the refusal into a warning; unsupported functions then return -1.
