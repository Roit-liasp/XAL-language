## English

<img src="data/images/logo_1.png" alt="XAL" width="360">

# XAL

XAL is a simple language designed for embedded systems, utilities, and experimentation.
Its operating principle is similar to that of the Java JVM,
as the code is compiled into bytecode that is subsequently executed by the XVM virtual machine.

Three names that are easily confused:
- XAL: the language itself.
- XALC: the compiler.
- XVM: the virtual machine.

---

A simple program looks like this:
```XAL
fn main() {
    print("Hello World"); 
    return 0;
}
```
---
To build the project and verify it works:
```SH
make all
make test
```
The first command builds xalc and xvm; the second runs the tests.
The `conf.sh` script automatically detects the system and necessary flags, so manual intervention is usually not required.
There is also a `make bare` target for building a bare-metal version, where the bytecode is embedded directly into the binary.

Now you can compile and run something:
```SH
./build/xalc -o /tmp/hello.xb examples/01_hello/hello.xa
./build/xvm /tmp/hello.xb
```

---

Repository contents:
- src/xalc - compiler.
- src/xvm - virtual machine.
- src/libxos - system layer: POSIX, Windows, bare metal.
- src/libxc, src/libxcore - lightweight runtime that powers everything.
- include - public headers.
- examples - 6 examples, ranging from "Hello World" to creating a custom library.
- tests - tests.
- data/doc/ru - documentation (ten chapters). Start with `01_intro.md`.

---

No long-term support is promised:
development is ongoing and bugs are being fixed, but there are no release dates.
If you find a bug, please report it, and it will be looked into.
Contributors are listed in [CONTRIBUTORS.md](CONTRIBUTORS.md).
---

## License
BSD 2-Clause license; see [LICENSE.md](LICENSE.md).

## Русский

<img src="data/images/logo_1.png" alt="XAL" width="360">

# XAL

XAL это простой язык для встраиваемых систем, утилит и экспериментов.
По принципу работы он похож на java JVM.
Так как код компилируеться в байткод который потом исполняеться виртуальной машиной XVM.

Три имени, которые легко перепутать:
- XAL это сам язык.
- XALC это компилятор.
- XVM это виртуальная машина.

---

Простейшая программа выглядит так:
```XAL
fn main() {
    print("Hello World");
    return 0;
}
```
---
Собрать и проверить, что он живой:
```SH
make all
make test
```
Первая команда собирает xalc и xvm, вторая запускает тесты.
Скрипт conf.sh сам определяет систему и нужные флаги, вмешиваться обычно не приходится.
Есть ещё make bare для сборки безоперационной системы, когда байткод зашивается прямо в бинарник.

Теперь можно что нибудь скомпилировать и запустить:
```SH
./build/xalc -o /tmp/hello.xb examples/01_hello/hello.xa
./build/xvm /tmp/hello.xb
```
---

Что лежит в репозитории:
- src/xalc - компилятор.
- src/xvm - виртуальная машина.
- src/libxos - системный слой: posix, windows, bare metal.
- src/libxc, src/libxcore - маленький рантайм, на котором всё держится.
- include - публичные заголовки.
- examples - 6 примеров, от hello world до своей библиотеки.
- tests - тесты.
- data/doc/ru - документация, десять глав. Начинать с 01_intro.md.

---
Долгой поддержки никто не обещает:
разработка идёт, ошибки чинятся, но дат релизов нет.
Нашли баг, сообщите, его посмотрят.
Кто участвовал в проекте, записано в [CONTRIBUTORS.md](CONTRIBUTORS.md).
---

## Лицензия
BSD 2-Clause license. см [LICENSE.md](LICENSE.md).