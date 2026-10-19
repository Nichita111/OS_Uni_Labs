#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

int showino;   // set by the optional -n flag: print the inode number too

// Return a pointer to the last component of path (after the last '/').
char *
lastname(char *path)
{
  char *p;

  for (p = path + strlen(path); p >= path && *p != '/'; p--)
    ;
  return p + 1;
}

void
find(char *path, char *name)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if ((fd = open(path, O_RDONLY)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }
  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  // The path itself matches?
  if (strcmp(lastname(path), name) == 0) {
    if (showino)
      printf("%s %d\n", path, st.ino);
    else
      printf("%s\n", path);
  }

  if (st.type != T_DIR) {
    close(fd);
    return;
  }

  if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
    fprintf(2, "find: path too long\n");
    close(fd);
    return;
  }
  strcpy(buf, path);
  p = buf + strlen(buf);
  *p++ = '/';
  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    if (de.inum == 0)
      continue;   // free slot
    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = 0;
    if (strcmp(p, ".") == 0 || strcmp(p, "..") == 0)
      continue;   // never recurse into . and ..
    find(buf, name);
  }
  close(fd);   // only 16 open files per process
}

int
main(int argc, char *argv[])
{
  if (argc < 3 || argc > 4 || (argc == 4 && strcmp(argv[3], "-n") != 0)) {
    fprintf(2, "usage: find <dir> <name> [-n]\n");
    exit(1);
  }
  showino = (argc == 4);
  find(argv[1], argv[2]);
  exit(0);
}
