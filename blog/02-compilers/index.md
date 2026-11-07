# Building cross-compilers

Building cross-compilers can be a very, very big pain in the ass. But once you know how to do things it becomes slightly easier.

## Some information about cross compiling

Read [the OSDev wiki page about this](https://wiki.osdev.org/Why_do_I_need_a_Cross_Compiler).

You might ask, `I'm not developing an OS, why should I read it?`, because, knowing something that you're not gonna use right now is not a bad idea.

### Build, host, target

This is mentioned on the OSDev wiki, but I'll write it down here.

You will encounter the `./configure` options `--build`, `--host`, and `--target`. But what do these mean?

|Option|Meaning|Typical value|
|-|-|-|
|`--build`|The machine you're building on.|On Linux, AMD64, this may be `x86_64-pc-linux-gnu`|
|`--host`|The machine that the output is going to run on|Usually, in the case of cross-compilers, this is equal to `--build`|
|`--target`|Only used for software that builds other software, this tells them what to output software for|Depends on the cross-compiler|

### ./configure
GNU software almost always uses `autoconf`. No one likes this, but there's not much we can do.

Some options that you, should know:

|Option|Meaning|Note|
|-|-|-|
|`--prefix`|Install location|By default, it is `/usr`. We do not want this|
|`--disable-&lt;feature&gt;`|Disables a feature||
|`--enable-&lt;feature&gt;`|Enables a features||
|`--without-&lt;package&gt;`|Don't use a package||
|`--with-&lt;package&gt;`|Use a package||

## mingw32

The older "brother" to `mingw-w64` (`x86_64-w64-mingw32` or `i686-w64-mingw32`), this is very useful for cross-compiling things to older versions of Windows (like 9x) that aren't console apps.

The triple for this is `i686-pc-mingw32` or just `mingw32`.

This configuration should be compatible with the last version of `mingw32` released for Windows, but I'm not too sure.

### Requirements

|Thing|Download|SHA256 sum|
|-|-|-|
|gmp 6.1.2|[Download](https://ftp.gnu.org/gnu/gmp/gmp-6.1.2.tar.xz)|`5275bb04f4863a13516b2f39392ac5e272f5e1bb8057b18aec1c9b79d73d8fb2`|
|mpc 1.0.3|[Download](https://ftp.gnu.org/gnu/mpc/mpc-1.0.3.tar.gz)|`617decc6ea09889fb08ede330917a00b16809b8db88c29c31bfbb49cbf88ecc3`|
|mpfr 3.1.5|[Download](https://ftp.gnu.org/gnu/mpfr/mpfr-3.1.5.tar.xz)|`015fde82b3979fbe5f83501986d328331ba8ddf008c1ff3da3c238f49ca062bc`|
|gcc 6.3.0|[Download](https://ftp.gnu.org/gnu/gcc/gcc-6.3.0/gcc-6.3.0.tar.bz2)|`f06ae7f3f790fbf0f018f6d40e844451e6bc3b7bc96e128e63b09825c1f8b29f`|
|binutils 2.27|[Download](https://ftp.gnu.org/gnu/binutils/binutils-2.27.tar.bz2) |`369737ce51587f92466041a97ab7d2358c6d9e1b6490b3940eb09fb0a9a6ac88`|
|w32api 3.18.2|[Download](/blog/02-compilers/w32api-3.18.2-mingw32-dev.tar.xz)|`c47f41c1956e90f4e8b060190953a606a37533bb4b9b647565326f62dcb14208`|
|mingwrt 3.22.4|[Download](/blog/02-compilers/mingwrt-3.22.4-mingw32-dev.tar.xz)|`1665e70082f47da9eda99c026e9a693f10fb666cc487bdcda5866cde4c018af5`|

### Building

#### Setup
```sh
export MINGW_TARGET=i686-pc-mingw32 # or mingw32
export MINGW_PREFIX=/opt/mingw32 # or anything you'd like, this is what I used
export PATH=$MINGW_PREFIX/bin:$PATH
```

#### Binutils

Very straightforward.

```sh
$ tar xpf binutils-2.27.tar.bz2
$ mkdir build-binutils && cd build-binutils
$ ../binutils-2.27/configure --target=$MINGW_TARGET --prefix=$MINGW_PREFIX --disable-nls --disable-multilib MAKEINFO=true
$ make -j$(nproc) && make install
```

### Runtime
```sh
$ mkdir -pv $MINGW_PREFIX/$MINGW_TARGET
$ tar -C $PREFIX/$MINGW_TARGET -xpf mingwrt-3.22.4-mingw32-dev.tar.xz
$ tar -C $PREFIX/$MINGW_TARGET -xpf w32api-3.18.2-mingw32-dev.tar.xz
$ find $MINGW_PREFIX/$MINGW_TARGET/include $MINGW_PREFIX/$MINGW_TARGET/lib -name "crt2.o" -or -name "libmingw32.a" -or -name "libmingwex.a" # check that these exist
```

The last command is to make sure that those files exist

### GCC and G++

First, extract `gcc`, `gmp`, `mpfr`, and `mpc`:
```sh
$ tar xpf gcc-6.3.0.tar.bz2 
$ tar xpf gmp-6.1.2.tar.bz2 && mv gmp-6.1.2 gcc-6.3.0/gmp
$ tar xpf mpfr-3.1.5.tar.xz && mv mpfr-3.1.5 gcc-6.3.0/mpfr
$ tar xpf mpc-1.0.3.tar.gz && mv mpc-1.0.3 gcc-6.3.0/mpc
```

Then, we need to patch GCC's `ubsan.c`:
```sh
$ sed -i "s/xloc.file == '\\\\0'/xloc.file == NULL/" gcc-6.3.0/gcc/ubsan.c
$ sed -n '1474p' gcc-6.3.0/gcc/ubsan.c # check
```

After that we can build GCC:
```sh
$ mkdir build-gcc && cd build-gcc
$ ../gcc-6.3.0/configure --target=$MINGW_TARGET --prefix=$PREFIX --enable-languages=c,c++ --enable-threads=win32 --disable-multilib --disable-nls --disable-win32-registry --without-isl --with-headers=$PREFIX/$MINGW_TARGET/include MAKEINFO=true
$ make -j$(nproc) all-gcc && make install-gcc
$ make -j$(nproc) && make install
```

### Testing
First, 4 small test programs testing the compiler and the standard C library:
```c
// test0.c
// i686-pc-mingw32-gcc -o test0.exe test0.c
int main(void)
{
    return 0;
}
```

```c
// test1.c
// i686-pc-mingw32-gcc -o test1.exe test1.c
#include <stdio.h>

int main(void)
{
    printf("Hello, World!\n");
    return 0;
}
```

```c
// test2.c
// i686-pc-mingw32-gcc -o test2.exe test2.c
#include <stdio.h>

int main(void)
{
    int a = 69;
    printf("a = %d\n", a);
    return 0;
}
```

```c
// test3.c
// i686-pc-mingw32-gcc -o test3.exe test3.c
#include <stdio.h>

int main(void)
{
    int a = 69;
    printf("a = %d\n", a);
    int b = 420;
    int c = 80085;
    printf("b = %d\n", b);
    printf("c = %d\n", c);
    return 0;
}
```

After that, a program testing the Win32 API:
```c
// test4.c
// i686-pc-mingw32-gcc -o test4.exe test4.c
#include <windows.h>

int main(void)
{
    MessageBoxA(NULL, "test", "Test", MB_OK);
    return 0;
}
```

Finally, testing the C++ compiler:
```cpp
// test5.cpp
// i686-pc-mingw32-g++ -o test5.exe test5.cpp
#include <iostream>

int main(void)
{
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
```

## Other compilers

When I:
 - need a cross-compiler,
 - and can figure out how to build it,

I will write them down here.

<small>Written on <time datetime="2026-11-07T00:00:00.000Z">2026-11-07</time></small><br>
