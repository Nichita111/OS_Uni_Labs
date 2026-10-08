# Lab 1 · The OS as a Resource Manager

**Nichita Rusnac, FAF-242** · UTM, FCIM · Operating Systems · 5 October 2026

All terminal output below is copied from my own session, in the order I ran the commands. `...` marks places where I cut long output.

## Setup

I did the lab on an Ubuntu 24.04 machine on AWS, connected over SSH.

```
$ mkdir -p ~/os-lab1 && cd ~/os-lab1
$ whoami
ubuntu
$ uname -a
Linux ip-172-31-11-189 7.0.0-1011-aws #11~24.04.1-Ubuntu SMP PREEMPT Mon Aug 10 15:07:40 UTC 2026 aarch64 aarch64 aarch64 GNU/Linux
$ uptime
 07:22:29 up 18 days, 16:29,  1 user,  load average: 0.00, 0.00, 0.00
$ pwd
/home/ubuntu/os-lab1
```

- I'm logged in as `ubuntu`
- the kernel is Linux `7.0.0-1011-aws`, on a 64-bit ARM CPU (`aarch64`)
- the OS had been running for 18 days with load average `0.00`, so the machine was basically idle

## Part 1. Files and directories

```
$ ls -la
total 12
drwxrwxr-x  3 ubuntu ubuntu 4096 Sep 28 16:40 .
drwxr-x--- 12 ubuntu ubuntu 4096 Sep 28 16:35 ..
drwxrwxr-x  2 ubuntu ubuntu 4096 Sep 28 16:40 demo
```

```
$ ls -la ~
total 80
drwxr-x--- 12 ubuntu ubuntu 4096 Sep 28 16:35 .
drwxr-xr-x  4 root   root   4096 Sep 28 15:45 ..
-rw-------  1 ubuntu ubuntu 5199 Sep 28 19:49 .bash_history
-rw-r--r--  1 ubuntu ubuntu  220 Mar 31  2024 .bash_logout
-rw-r--r--  1 ubuntu ubuntu 3859 Jul 22 01:36 .bashrc
drwx------  6 ubuntu ubuntu 4096 Jul 22 01:42 .cache
drwxrwxr-x  4 ubuntu ubuntu 4096 Sep 28 16:33 .config
drwxrwxr-x  3 ubuntu ubuntu 4096 Jul 22 01:52 .cua-driver
-rw-rw-r--  1 ubuntu ubuntu   42 Jul 22 02:59 .gitconfig
drwx------  3 ubuntu ubuntu 4096 Jul 23 23:05 .gnupg
drwx------ 21 ubuntu ubuntu 4096 Oct  5 07:22 .hermes
drwxrwxr-x  5 ubuntu ubuntu 4096 Jul 22 02:13 .local
drwxrwxr-x  4 ubuntu ubuntu 4096 Jul 22 01:35 .npm
-rw-r--r--  1 ubuntu ubuntu  807 Mar 31  2024 .profile
-rw-rw-r--  1 ubuntu ubuntu   66 Jul 22 02:59 .selected_editor
drwx------  2 ubuntu ubuntu 4096 Jul 22 02:59 .ssh
-rw-r--r--  1 ubuntu ubuntu    0 Jul 22 01:34 .sudo_as_admin_successful
drwxrwxr-x  3 ubuntu ubuntu 4096 Sep 28 16:40 os-lab1
drwxrwxr-x  7 ubuntu ubuntu 4096 Oct  3 19:00 vault
-rw-rw-r--  1 ubuntu ubuntu  190 Sep 16 23:50 vault-sync-fail.log
```

```
$ cd /etc && ls | head
ModemManager
PackageKit
X11
acpi
adduser.conf
alternatives
apache2
apparmor
apparmor.d
apport
```

```
$ cd ~/os-lab1/
$ mmkdir demo && cd demo
Command 'mmkdir' not found, but can be installed with:
sudo apt install mblaze
$ mkdir demo && cd demo
mkdir: cannot create directory ‘demo’: File exists
$ rm demo
rm: cannot remove 'demo': Is a directory
$ ls
demo
$ rm -r demo
$ mkdir demo && cd demo
```

```
$ echo "hello operating systems" > note.txt
$ cp note.txt copy.txt
$ mv copy.txt renamed.txt
$ ls -l
total 8
-rw-rw-r-- 1 ubuntu ubuntu 24 Oct  5 07:24 note.txt
-rw-rw-r-- 1 ubuntu ubuntu 24 Oct  5 07:25 renamed.txt
$ cat renamed.txt
hello operating systems
```

```
$ rm renamed.txt
$ ls -l note.txt
-rw-rw-r-- 1 ubuntu ubuntu 24 Oct  5 07:24 note.txt
```

```
$ chmod 600 note.txt
$ ls -l note.txt
-rw------- 1 ubuntu ubuntu 24 Oct  5 07:24 note.txt
$ chmod 644 note.txt
$ ls -l note.txt
-rw-r--r-- 1 ubuntu ubuntu 24 Oct  5 07:24 note.txt
```

**1. Who owns the files I created, and which group?**

User `ubuntu`, group `ubuntu` (UID and GID 1000). Every file I create gets my user and my primary group automatically.

**2. What do the ten characters mean for note.txt after `chmod 600`?**

It showed `-rw-------`:

- `-` it's a regular file (`d` would be a directory)
- `rw-` the owner can read and write, but not execute
- `---` the group gets no access
- `---` everyone else gets no access

So only I can open the file. Before that it was `-rw-rw-r--` (664, because my umask is `0002`), and after `chmod 644` it became `-rw-r--r--`, readable by everyone but writable only by me.

**3. Two directories directly under `/`**

- `/etc` holds system-wide config files (I saw `apache2`, `apparmor`, `adduser.conf` in it)
- `/home` holds the users' personal folders (mine is `/home/ubuntu`)

**Interesting output**

My first `mkdir demo` failed with `File exists` because a `demo` folder was left over from an earlier try (dated Sep 28). Then `rm demo` refused with `Is a directory`, so the OS won't delete a folder by accident, you have to ask for it with `rm -r`. Also in my home, `.ssh` and `.gnupg` are `drwx------`, so only my user can even enter them, which makes sense since they hold keys.

## Part 2. Processes

```
$ ps aux | head
USER         PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND
root           1  0.0  0.6  22800 12672 ?        Ss   Sep16   1:26 /usr/lib/systemd/systemd --system --deserialize=81
root           2  0.0  0.0      0     0 ?        S    Sep16   0:00 [kthreadd]
root           3  0.0  0.0      0     0 ?        S    Sep16   0:00 [pool_workqueue_release]
...
```

`top` (the screen I saw before quitting with `q`):

```
top - 07:27:15 up 18 days, 16:33,  1 user,  load average: 0.04, 0.01, 0.00
Tasks: 143 total,   1 running, 142 sleeping,   0 stopped,   0 zombie
%Cpu(s):  0.2 us,  0.0 sy,  0.0 ni, 99.4 id,  0.0 wa,  0.0 hi,  0.2 si,  0.2 st
MiB Mem :   1835.0 total,    165.3 free,    820.3 used,   1034.9 buff/cache
MiB Swap:      0.0 total,      0.0 free,      0.0 used.   1014.7 avail Mem

    PID USER      PR  NI    VIRT    RES    SHR S  %CPU  %MEM     TIME+ COMMAND
    800 ubuntu    20   0 1375492 417252  26884 S   0.4  22.2  53:21.10 hermes
 152900 ubuntu    20   0   12344   5340   3124 R   0.4   0.3   0:00.07 top
      1 root      20   0   22800  12672   8260 S   0.0   0.7   1:26.82 systemd
      2 root      20   0       0      0      0 S   0.0   0.0   0:00.35 kthreadd
      3 root      20   0       0      0      0 S   0.0   0.0   0:00.00 pool_workqueue_release
      4 root       0 -20       0      0      0 I   0.0   0.0   0:00.00 kworker/R-rcu_gp
      5 root       0 -20       0      0      0 I   0.0   0.0   0:00.00 kworker/R-sync_wq
      6 root       0 -20       0      0      0 I   0.0   0.0   0:00.00 kworker/R-kvfree_rcu_reclaim
      7 root       0 -20       0      0      0 I   0.0   0.0   0:00.00 kworker/R-slub_flushwq
      8 root       0 -20       0      0      0 I   0.0   0.0   0:00.00 kworker/R-netns
...
     34 root      20   0       0      0      0 S   0.0   0.0   0:00.00 oom_reaper
...
```

```
$ sleep 300 &
[1] 152903
$ jobs
[1]+  Running                 sleep 300 &
$ ps -ef | grep sleep
ubuntu    152903  152798  0 07:27 pts/0    00:00:00 sleep 300
ubuntu    152905  152798  0 07:28 pts/0    00:00:00 grep --color=auto sleep
```

```
$ kil 152903
Command 'kil' not found, did you mean:
  command 'kig' from snap kig (26.04.3)
  command 'kig' from deb kig (4:23.08.4-0ubuntu2)
  command 'uil' from deb uil (2.3.8-3)
  command 'kill' from deb procps (2:4.0.4-4ubuntu3.2)
  command 'kile' from deb kile (4:2.9.93-2)
  command 'kic' from deb kic (2.4c-1)
  command 'pil' from deb picolisp (23.12-1)
  command 'cil' from deb cil (0.07.00-12)
See 'snap info <snapname>' for additional versions.
$ kill 152903
$ jobs
[1]+  Terminated              sleep 300
```

```
$ sleep 300 &
[1] 152918
$ PID=$!
$ ls /proc/$PID/
attr        coredump_filter  gid_map            map_files   ns             personality  smaps         task
autogroup   cpuset           io                 maps        numa_maps      projid_map   smaps_rollup  timens_offsets
auxv        cwd              ksm_merging_pages  mem         oom_adj        root         stack         timers
cgroup      environ          ksm_stat           mountinfo   oom_score      sched        stat          timerslack_ns
clear_refs  exe              latency            mounts      oom_score_adj  schedstat    statm         uid_map
cmdline     fd               limits             mountstats  pagemap        sessionid    status        wchan
comm        fdinfo           loginuid           net         patch_state    setgroups    syscall
```

```
$ cat /proc/$PID/status | head -20
Name:	sleep
Umask:	0002
State:	S (sleeping)
Tgid:	152918
Ngid:	0
Pid:	152918
PPid:	152798
TracerPid:	0
Uid:	1000	1000	1000	1000
Gid:	1000	1000	1000	1000
FDSize:	256
Groups:	4 24 27 30 104 1000
NStgid:	152918
NSpid:	152918
NSpgid:	152918
NSsid:	152798
Kthread:	0
VmPeak:	    5696 kB
VmSize:	    5696 kB
VmLck:	       0 kB
$ kill $PID
$ jobs
[1]+  Terminated              sleep 300
```

**1. What is PID 1?**

PID 1 is `systemd` (`/usr/lib/systemd/systemd --system`), owned by root and started on Sep 16, the same day the machine booted. It's the init system, the first user-space process the kernel starts. It starts and supervises all the services and adopts processes whose parent died.

**2. How many processes were running on the idle VM?**

About 143. `top` showed `Tasks: 143 total, 1 running, 142 sleeping`. Most of them are kernel threads (`kworker`, `ksoftirqd`, `migration` ...) sitting idle, and the only one actually running was `top` itself.

**3. What does `State:` say for the sleeping process?**

`State: S (sleeping)`. The `sleep` process just waits for a timer, so it uses no CPU until the kernel wakes it up.

**Interesting output**

In `/proc/$PID/status` the line `PPid: 152798` matches the parent PID column of `ps -ef`. That's my bash shell, which created the `sleep` process. After `kill`, `jobs` changed from `Running` to `Terminated`, since `kill` sends SIGTERM by default. (I also typed `kil` once, and Ubuntu suggested `kill` from the `procps` package.)

## Part 3. Memory

```
$ free -h
               total        used        free      shared  buff/cache   available
Mem:           1.8Gi       821Mi       163Mi       928Ki       1.0Gi       1.0Gi
Swap:             0B          0B          0B
```

```
$ cat /proc/meminfo | head -6
MemTotal:        1879088 kB
MemFree:          167868 kB
MemAvailable:    1038312 kB
Buffers:           84660 kB
Cached:           756000 kB
SwapCached:            0 kB
```

```
$ sleep 300 & PID=$!
[1] 152945
$ grep VmRSS /proc/$PID/status
VmRSS:	    1680 kB
$ kill $PID
```

**1. Total and free RAM**

Total is about 1.8 GiB (`MemTotal: 1879088 kB`). Only 163 MiB is completely free (`MemFree: 167868 kB`), but `available` is 1.0 GiB because around 1 GiB is buffers and cache that the kernel gives back as soon as a program needs it. So the real headroom is about 1 GB.

**2. What is swap, and how much is configured?**

Swap is disk space used as extra memory. When RAM gets full, the kernel moves rarely used pages out to disk. Here swap is `0B`, so none is configured (normal for small cloud images). If RAM runs out, the kernel's OOM killer has to kill a process instead (I even saw the `oom_reaper` thread in `top`).

**3. VmRSS of a bare `sleep`**

`VmRSS: 1680 kB`, so about 1.6 MB. It surprised me a bit for a program that does nothing, but it still needs its own code, the C library, the dynamic loader and a stack loaded into RAM. Its virtual size is bigger (`VmSize: 5696 kB`), and only part of it is actually resident. A lot of those pages are shared libc pages also used by other processes, so the real extra cost is even smaller.

**Interesting output**

`free` says 163Mi free but 1.0Gi available. Linux uses spare RAM as disk cache instead of leaving it empty, so "free" being low doesn't mean the machine is out of memory.

## Part 4. Devices and storage

```
$ df -h
Filesystem       Size  Used Avail Use% Mounted on
/dev/root         15G  6.9G  7.6G  48% /
tmpfs            918M     0  918M   0% /dev/shm
tmpfs            368M  912K  367M   1% /run
tmpfs            5.0M     0  5.0M   0% /run/lock
efivarfs         128K  3.3K  125K   3% /sys/firmware/efi/efivars
/dev/nvme0n1p16  891M  182M  647M  22% /boot
/dev/nvme0n1p15   98M  6.4M   92M   7% /boot/efi
tmpfs            184M   16K  184M   1% /run/user/1000
[1]+  Terminated              sleep 300
```

```
$ lsblk
NAME         MAJ:MIN RM  SIZE RO TYPE MOUNTPOINTS
loop0          7:0    0 24.2M  1 loop /snap/amazon-ssm-agent/13008
loop1          7:1    0 24.3M  1 loop /snap/amazon-ssm-agent/13348
loop2          7:2    0   69M  1 loop /snap/core22/2412
loop3          7:3    0   69M  1 loop /snap/core22/2956
loop5          7:5    0 43.5M  1 loop /snap/snapd/27740
loop6          7:6    0 38.6M  1 loop /snap/snapd/28255
nvme0n1      259:0    0   16G  0 disk
├─nvme0n1p1  259:1    0   15G  0 part /
├─nvme0n1p15 259:2    0   99M  0 part /boot/efi
└─nvme0n1p16 259:3    0  923M  0 part /boot
```

```
$ du -sh ~/os-lab1
12K	/home/ubuntu/os-lab1
```

```
$ ls -l /dev | head
total 0
crw-r--r--  1 root root     10, 235 Sep 16 14:53 autofs
drwxr-xr-x  2 root root         280 Oct  5 07:19 block
crw-------  1 root root     10, 234 Sep 16 14:53 btrfs-control
drwxr-xr-x  2 root root        3140 Sep 16 14:53 char
crw--w----  1 root tty       5,   1 Sep 16 14:53 console
lrwxrwxrwx  1 root root          11 Sep 16 14:53 core -> /proc/kcore
crw-------  1 root root     10, 260 Sep 16 14:53 cpu_dma_latency
crw-------  1 root root     10, 261 Sep 16 14:53 cpu_wakeup_latency
crw-------  1 root root     10, 203 Sep 16 14:53 cuse
```

```
$ mount | head
/dev/nvme0n1p1 on / type ext4 (rw,relatime,discard,errors=remount-ro,commit=30)
devtmpfs on /dev type devtmpfs (rw,nosuid,noexec,relatime,size=786648k,nr_inodes=196662,mode=755,inode64)
proc on /proc type proc (rw,nosuid,nodev,noexec,relatime)
sysfs on /sys type sysfs (rw,nosuid,nodev,noexec,relatime)
securityfs on /sys/kernel/security type securityfs (rw,nosuid,nodev,noexec,relatime)
tmpfs on /dev/shm type tmpfs (rw,nosuid,nodev,inode64)
devpts on /dev/pts type devpts (rw,nosuid,noexec,relatime,gid=5,mode=620,ptmxmode=000)
tmpfs on /run type tmpfs (rw,nosuid,nodev,size=375820k,nr_inodes=819200,mode=755,inode64)
tmpfs on /run/lock type tmpfs (rw,nosuid,nodev,noexec,relatime,size=5120k,inode64)
cgroup2 on /sys/fs/cgroup type cgroup2 (rw,nosuid,nodev,noexec,relatime,nsdelegate,memory_recursiveprot)
```

**1. Which device is `/` mounted on?**

`/dev/nvme0n1p1`, the first partition of the NVMe disk `nvme0n1` (16G), formatted as ext4. `df` shows it under the alias `/dev/root`, while `lsblk` and `mount` show the real name. It's 15G with 6.9G used (48%).

**2. One entry from `/dev`**

`/dev/console` (`crw--w---- root tty 5, 1`) is the system console terminal. The `c` means it's a character device, and `5, 1` are the major and minor numbers the kernel uses to find the right driver.

**3. What does "everything is a file" mean?**

Disks (`/dev/nvme0n1`), terminals (`/dev/console`), process info (`/proc/$PID/status`) and even kernel memory (`/dev/core -> /proc/kcore`) all appear as paths in one tree, so I could look at them with the same `ls`, `cat` and `grep` I use on normal files.

**Interesting output**

`lsblk` shows read-only `loop` devices mounted under `/snap`. Each snap package is a compressed image file mounted as if it were a small disk. In `mount`, `/proc`, `/sys` and `/dev` are not real disks either (`proc`, `sysfs`, `devtmpfs`), the kernel builds them in memory. And my whole lab folder takes only `12K`.

## Wrap-up

Files are organized by the OS into one tree with owners and permissions, which I saw with `ls -l`. Processes are created, scheduled and stopped by the OS, which `ps aux` showed, and memory is shared out between them, which `free -h` showed. Devices like the NVMe disk are hidden behind file-like interfaces, which I saw with `lsblk`.
