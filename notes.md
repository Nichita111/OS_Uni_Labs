# Lab 2 · Meet the OS You Will Build

**Nichita Rusnac, FAF-242** · UTM, FCIM · Operating Systems · 5 October 2026

All terminal output below is copied from my own session, in the order I ran the commands. `...` marks places where I cut long or repeated output. I did the lab on my Ubuntu 24.04 ARM machine on AWS, with a fresh clone of upstream `mit-pdos/xv6-riscv`.

## Part 1. Build it and boot it

On the VM:

```
$ cd ~/xv6-riscv 2>/dev/null || git clone https://github.com/mit-pdos/xv6-riscv ~/xv6-riscv
Cloning into '/home/ubuntu/xv6-riscv'...
remote: Enumerating objects: 8349, done.
remote: Counting objects: 100% (49/49), done.
remote: Compressing objects: 100% (28/28), done.
remote: Total 8349 (delta 29), reused 21 (delta 21), pack-reused 8300 (from 2)
Receiving objects: 100% (8349/8349), 17.68 MiB | 30.59 MiB/s, done.
Resolving deltas: 100% (4594/4594), done.
$ cd ~/xv6-riscv/
$ ls
LICENSE  Makefile  README  kernel  mkfs  test-xv6.py  user
$ make qemu
riscv64-linux-gnu-gcc -march=rv64gc -g -ffile-prefix-map=/home/ubuntu/xv6-riscv=. -c -o kernel/entry.o kernel/entry.S
riscv64-linux-gnu-gcc -Wall -Werror ... -march=rv64gc -std=gnu99 ... -c -o kernel/start.o kernel/start.c
riscv64-linux-gnu-gcc -Wall -Werror ... -march=rv64gc -std=gnu99 ... -c -o kernel/console.o kernel/console.c
riscv64-linux-gnu-gcc -Wall -Werror ... -march=rv64gc -std=gnu99 ... -c -o kernel/printk.o kernel/printk.c
riscv64-linux-gnu-gcc -Wall -Werror ... -march=rv64gc -std=gnu99 ... -c -o kernel/uart.o kernel/uart.c
riscv64-linux-gnu-gcc -Wall -Werror ... -march=rv64gc -std=gnu99 ... -c -o kernel/kalloc.o kernel/kalloc.c
...
```

The kernel and user programs are compiled with `riscv64-linux-gnu-gcc -march=rv64gc`, so my ARM machine is cross-compiling code for a different CPU. QEMU then emulates that RISC-V machine and boots the kernel I just built.

Inside xv6:

```
xv6 kernel is booting

hart 2 starting
hart 1 starting
init: starting sh
$ ls
.              1 1 1024
..             1 1 1024
README         2 2 2441
cat            2 3 35376
echo           2 4 34312
forktest       2 5 17280
grep           2 6 42736
init           2 7 34768
kill           2 8 34240
ln             2 9 34032
ls             2 10 41520
mkdir          2 11 34280
rm             2 12 34264
sh             2 13 56136
stressfs       2 14 35160
usertests      2 15 200056
grind          2 16 50504
wc             2 17 36336
zombie         2 18 33648
logstress      2 19 36184
forphan        2 20 35048
dorphan        2 21 34488
sync           2 22 33712
console        3 23 0
```

`hart 2 starting` and `hart 1 starting` are the extra CPU cores coming up ("hart" is RISC-V's name for a hardware thread; QEMU runs with `-smp 3`). Then `init` starts the shell and I get the `$` prompt.

## Part 2. Use xv6 as the Unix it is

Inside xv6:

```
$ cat README
exec catME failed
$ cat README
xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
Version 6 (v6).  xv6 loosely follows the structure and style of v6,
but is implemented for a modern RISC-V multiprocessor using ANSI C.
...
$ echo hello xv6
hello xv6
$ ls | grep c
cat            2 3 35376
echo           2 4 34312
wc             2 17 36336
sync           2 22 33712
console        3 23 0
$ wc README
48 336 2441 README
$ usertest -q
exec usertest failed
$ usertests -q
usertests starting
test copyin: OK
test copyout: OK
test copyinstr1: OK
test copyinstr2: OK
test copyinstr3: OK
test rwsbrk: OK
...
test lazy_copy: OK
test lazy_copyinstr: OK
test lazy_sbrk: OK
test partial_write: OK
test unlinkcwd: OK
ALL TESTS PASSED
$ QEMU: Terminated
```

**1. Three programs xv6 ships with**

`ls`, `cat` and `grep` (also `echo`, `wc`, `mkdir`, `rm`, `kill`, `sh` and the test programs like `usertests`).

**2. Which two OS features must exist for a pipe to work?**

- **Processes.** The shell has to create two processes with `fork()` and load `ls` and `grep` into them with `exec()`, so both run at the same time.
- **Communication between them.** `pipe()` creates a kernel buffer with a read end and a write end (two file descriptors). The shell connects `ls`'s output to the write end and `grep`'s input to the read end, so data flows from one process to the other through the kernel.

**3. xv6 shell vs the Linux shell from Lab 1**

It works the same way at its core (run a program, pipes, `<` `>` redirection, `&`, `;`), but it's much more basic. It has no history, tab completion, variables or wildcards, and errors are short, like `exec usertest failed` instead of Linux's "command not found" with suggestions.

**Interesting output**

xv6's `ls` prints name, type, inode number and size. Type `1` is a directory, `2` a file and `3` a device, so `console 3 23 0` is the terminal shown as a file, just like `/dev/console` in Lab 1. `wc README` gave `48 336 2441` (lines, words, bytes), and 2441 matches the README size in `ls`. `usertests -q` ended with `ALL TESTS PASSED`, so the kernel I built works correctly.

## Part 3. Read the source

After quitting QEMU (Ctrl-A, then X), on the VM:

```
$ sed -n "1,40p" user/ls.c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

char *
fmtname(char *path)
{
  static char buf[DIRSIZ + 1];
  char *p;

  // Find first character after last slash.
  for (p = path + strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if (strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf + strlen(p), ' ', DIRSIZ - strlen(p));
  buf[sizeof(buf) - 1] = '\0';
  return buf;
}

void
ls(char *path)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if ((fd = open(path, O_RDONLY)) < 0) {
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
```

```
$ sed -n "1,30p" user/cat.c
#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

char buf[512];

void
cat(int fd)
{
  int n;

  while ((n = read(fd, buf, sizeof(buf))) > 0) {
    if (write(1, buf, n) != n) {
      fprintf(2, "cat: write error\n");
      exit(1);
    }
  }
  if (n < 0) {
    fprintf(2, "cat: read error\n");
    exit(1);
  }
}

int
main(int argc, char *argv[])
{
  int fd, i;

  if (argc <= 1) {
    cat(0);
```

```
$ cat user/user.h
#define SBRK_ERROR ((char *)-1)

struct stat;

// system calls
int fork(void);
int exit(int) __attribute__((noreturn));
int wait(int *);
int pipe(int *);
int write(int, const void *, int);
int read(int, void *, int);
int close(int);
int kill(int);
int exec(const char *, char **);
int open(const char *, int);
int mknod(const char *, short, short);
int unlink(const char *);
int fstat(int fd, struct stat *);
int link(const char *, const char *);
int mkdir(const char *);
int chdir(const char *);
int dup(int);
int getpid(void);
char *sys_sbrk(int, int);
int pause(int);
int uptime(void);
int sync(void);
...
```

```
$ grep -n "sys_read\|sys_write" kernel/sysfile.c | head
69:sys_read(void)
83:sys_write(void)
```

**1. Which system calls does `user/cat.c` use, and what does each ask the kernel?**

- `read(fd, buf, sizeof(buf))` asks the kernel to copy up to 512 bytes from an open file into `buf`. It returns how many bytes it read, `0` at the end of the file.
- `write(1, buf, n)` asks the kernel to send `n` bytes to file descriptor 1, the standard output (the console).
- `exit(1)` asks the kernel to end the process and hand the status to its parent.
- `open()` and `close()` are used later in `main` to open each file given as an argument and release it afterwards.

`fprintf(2, ...)` is not a system call. It's a library function from `printf.c` that ends up calling `write()` on fd 2 (standard error).

**2. Where is `sys_read` implemented?**

In `kernel/sysfile.c`, line 69 (`sys_write` is on line 83).

**3. Difference between `kernel/` and `user/`**

`kernel/` is the OS itself, running in privileged mode with full access to memory and devices. `user/` holds normal programs that run in unprivileged user mode and can only get things done by asking the kernel through the system calls listed in `user/user.h`.

**Interesting output**

`user/user.h` shows that the whole interface between programs and the kernel is just 22 system calls, while Linux has hundreds. It also showed me that there is no `sleep()` in this version, only `pause(int)`, which mattered in Part 4.

## Part 4. My first xv6 program

```
$ cat > user/sleep.c << 'EOF'
#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if(argc != 2){
    fprintf(2, "usage: sleep <ticks>\n");
    exit(1);
  }
  sleep(atoi(argv[1]));   // system call into the kernel
  exit(0);
}
EOF
```

```
$ vim Makefile
$ make qemu
riscv64-linux-gnu-gcc -Wall -Werror ... -march=rv64gc -std=gnu99 ... -c -o user/sleep.o user/sleep.c
user/sleep.c: In function ‘main’:
user/sleep.c:11:3: error: implicit declaration of function ‘sleep’ [-Werror=implicit-function-declaration]
   11 |   sleep(atoi(argv[1]));   // system call into the kernel
      |   ^~~~~
At top level:
cc1: note: unrecognized command-line option ‘-Wno-unknown-attributes’ may have been intended to silence earlier diagnostics
cc1: all warnings being treated as errors
make: *** [<builtin>: user/sleep.o] Error 1
```

```
$ sed -i 's/sleep(atoi(argv\[1\]))/pause(atoi(argv[1]))/' user/sleep.c
$ grep -n pause user/sleep.c user/user.h
user/sleep.c:11:  pause(atoi(argv[1]));   // system call into the kernel
user/user.h:25:int pause(int);
```

```
$ make qemu
riscv64-linux-gnu-gcc -Wall -Werror ... -march=rv64gc -std=gnu99 ... -c -o user/sleep.o user/sleep.c
riscv64-linux-gnu-ld -z max-page-size=4096 -T user/user.ld -o user/_sleep user/sleep.o user/ulib.o user/usys.o user/printf.o user/umalloc.o
riscv64-linux-gnu-objdump -S user/_sleep > user/sleep.asm
riscv64-linux-gnu-objdump -t user/_sleep | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$/d' > user/sleep.sym
mkfs/mkfs fs.img README user/_cat user/_echo user/_sleep user/_forktest user/_grep user/_init user/_kill user/_ln user/_ls user/_mkdir user/_rm user/_sh user/_stressfs user/_usertests user/_grind user/_wc user/_zombie user/_logstress user/_forphan user/_dorphan user/_sync
nmeta 47 (boot, super, log blocks 31, inode blocks 13, bitmap blocks 1) blocks 1953 total 2000
balloc: first 988 blocks have been allocated
balloc: write bitmap block at sector 46
qemu-system-riscv64 -machine virt -bios none -kernel kernel/kernel -m 128M -smp 3 -nographic -global virtio-mmio.force-legacy=false -drive file=fs.img,if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

xv6 kernel is booting

hart 2 starting
hart 1 starting
init: starting sh
$ sleep 10
$ sleep
usage: sleep <ticks>
$
```

Final `user/sleep.c`:

```c
#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if(argc != 2){
    fprintf(2, "usage: sleep <ticks>\n");
    exit(1);
  }
  pause(atoi(argv[1]));   // system call into the kernel
  exit(0);
}
```

In the `Makefile` I added `$U/_sleep\` to `UPROGS`, indented with a tab like the others:

```make
UPROGS=\
	$U/_cat\
	$U/_echo\
	$U/_sleep\
	$U/_forktest\
	...
```

The `mkfs` line in the build output lists `user/_sleep`, so the program was packed into the xv6 disk image.

Inside xv6, `sleep 10` paused and then returned to the prompt. One tick is about 0.1 s in QEMU, so that's around one second. Running `sleep` with no argument printed `usage: sleep <ticks>`, so the usage check works too.

## Problems I solved along the way

- **No RISC-V toolchain.** My first `make qemu` failed, because the VM had no RISC-V compiler or QEMU and its own gcc only targets ARM:

  ```
  *** Error: Couldn't find a riscv64 version of GCC/binutils.
  /bin/sh: 1: qemu-system-riscv64: not found
  cc1: error: unknown value 'rv64gc' for '-march'
  ```

  I installed the packages from the lab's "if a tool is missing" box (`gcc-riscv64-linux-gnu`, `binutils-riscv64-linux-gnu`, `qemu-system-misc`, `gdb-multiarch`) and rebuilt.

- **`echo` broke the C file.** The double quotes inside the code closed the `echo "..."` string early, and bash read `<ticks>` as an input redirect (`-bash: ticks: No such file or directory`). I wrote the file with a heredoc (`cat > user/sleep.c << 'EOF'`), which copies the text exactly.

  ```
  $ touch user/sleep.c
  $ echo "#include "kernel/types.h" #include "user/user.h"  int main(int argc, char *argv[]) { ... }" > user/sleep.c
  -bash: ticks: No such file or directory
  $ cd user
  $ cat sleep.c
  $ echo "#include "kernel/types.h" #include "user/user.h"  int main(int argc, char *argv[]) { ... }" > sleep.c
  -bash: ticks: No such file or directory
  $ cd ..
  ```

- **`sleep()` doesn't exist in upstream xv6.** The build failed with `implicit declaration of function 'sleep'`. The current xv6 renamed the sleep system call to `pause()` (`user/user.h` line 25, implemented as `sys_pause` in `kernel/sysproc.c`). It does the same thing, waiting a number of clock ticks, so I call `pause()` and the program is still called `sleep`.

- **Typos inside xv6.** `usertest` (missing `s`) gave `exec usertest failed`, which is xv6's version of "command not found". One edited line also came through garbled as `catME`. Retyping both fixed it.
