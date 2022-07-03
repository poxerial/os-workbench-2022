#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_PROCS 512
#define PROC_PATH "/proc"
#define MAX_PROC_CHILDS_NUM 32
#define MAX_PATH_LEN 512
#define MESSAGE_MAX_SIZE 256

#define UTF_V "\342\224\202"  /* U+2502, Vertical line drawing char */
#define UTF_VR "\342\224\234" /* U+251C, Vertical and right */
#define UTF_H "\342\224\200"  /* U+2500, Horizontal */
#define UTF_UR "\342\224\224" /* U+2514, Up and right */
#define UTF_HD "\342\224\254" /* U+252C, Horizontal and down */

typedef struct process_t process;
struct process_t {
  int pid;
  char comm[256];
  int ppid;
  process *child_procs[MAX_PROC_CHILDS_NUM];
  int child_num;
};

const char usage[] =
    "usage:\n-p, --show-pids: 打印每个进程的进程号。\n"
    "-n --numeric-sort: 按照pid的数值从小到大顺序输出一个进程的直接孩子。\n"
    "-V --version: 打印版本信息\n";

int is_sort;
int is_print_pid;
process procs[MAX_PROCS];
size_t proc_num;

int proc_comp(const void *a, const void *b) { return ((process **)a)[0]->pid <= ((process **)b)[0]->pid; }

void readProcess() {
  struct dirent *file = NULL;
  DIR *dir = opendir(PROC_PATH);
  procs[proc_num].ppid = INT_MIN;
  strcpy(procs[proc_num++].comm, "root");
  while (NULL != (file = readdir(dir))) {
    if (file->d_name[0] < '0' || file->d_name[0] > '9')
      continue;
    char path[MAX_PATH_LEN];
    sprintf(path, "%s/%s/stat", PROC_PATH, file->d_name);
    FILE *stat = fopen(path, "r");
    char temp;
    fscanf(stat, "%d %s %c %d", &procs[proc_num].pid, procs[proc_num].comm,
           &temp, &procs[proc_num].ppid);
    proc_num++;
  }
}

process *createTree() {
  process *root = NULL;
  int parent_pid = 0;
  for (int proc_iter_index = 0; proc_iter_index < proc_num;) {
    assert(procs[proc_iter_index].child_num < MAX_PROC_CHILDS_NUM);
    int is_all_child_linked = 1;
    for (int i = 0; i < proc_num; i++) {
      if (procs[i].ppid == parent_pid) {
        is_all_child_linked = 0;
        size_t child_num = procs[proc_iter_index].child_num;
        procs[proc_iter_index].child_procs[child_num++] = &procs[i];
        procs[i].ppid = INT_MIN;
      }
    }

    if (is_all_child_linked) {
      if (is_sort)
        qsort(procs[proc_iter_index].child_procs, sizeof(process *),
              procs[proc_iter_index].child_num * sizeof(process *), proc_comp);
      if (procs[proc_iter_index].pid == 1)
        root = &procs[proc_iter_index];
      parent_pid = procs[proc_iter_index].pid;
      proc_iter_index++;
    }
  }
  return root;
}

char *add_n_space(char *dest, int n) {
  char *end = dest + n;
  for (; dest != end; dest++)
    *dest = ' ';
  return end;
}

void print_node(process *node) {
  if (is_print_pid)
    printf("%s(%d)", node->comm, node->pid);
  else
    printf("%s", node->comm);
}

void print_tree(char *forward, char *forward_end, process *root) {
  printf("%s", forward);
  print_node(root);
  if (root->child_num != 0) {
    printf("   ");
    forward_end = add_n_space(forward_end, strlen(root->comm + 3));
    print_tree(forward, forward_end, root->child_procs[0]);
    for (int i = 1; i < root->child_num; i++)
    {
      print_tree(forward, forward_end, root->child_procs[i]);
    }
  }
}

int main(int argc, char *argv[]) {
  for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    if (strcmp(argv[i], "-V") == 0) {
      printf("pstree from os-workbench-2022, version 0.0.1");
      break;
    }
    if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--show-pids"))
      is_print_pid = 1;
    if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--numeric-sort"))
      is_sort = 1;
  }
  assert(!argv[argc]);
  char fst_tree_message[MESSAGE_MAX_SIZE];
  char snd_tree_message[MESSAGE_MAX_SIZE];
  readProcess();
  process *root = createTree();
  char line_buff[MESSAGE_MAX_SIZE];
  print_tree(line_buff, line_buff, root);
  return 0;
}
