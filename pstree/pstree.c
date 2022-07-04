#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_PROCS 512
#define PROC_PATH "/proc"
#define MAX_PROC_CHILDS_NUM 512 
#define MAX_PATH_LEN 512
#define MESSAGE_MAX_SIZE 512

#define UTF_V "\342\224\202"  /* U+2502, Vertical line drawing char */
#define UTF_VR "\342\224\234" /* U+251C, Vertical and right */
#define UTF_H "\342\224\200"  /* U+2500, Horizontal */
#define UTF_UR "\342\224\224" /* U+2514, Up and right */
#define UTF_HD "\342\224\254" /* U+252C, Horizontal and down */
// UTF_V│UTF_VR├UTF_H─UTF_UR└UTF_HD┬

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

int proc_comp(const void *a, const void *b) {
  return ((process **)a)[0]->pid >= ((process **)b)[0]->pid ? 1 : 0;
}

int proc_comp_default(const void *a, const void *b)
{
  return ((process **)a)[0]->comm[0] >= ((process **)b)[0]->comm[0] ? 1 : 0;
}

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
    fscanf(stat, "%d (%s) %c %d", &procs[proc_num].pid, procs[proc_num].comm,
           &temp, &procs[proc_num].ppid);
    proc_num++;
  }
  closedir(dir);
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
        procs[proc_iter_index].child_num = child_num;
        procs[i].ppid = INT_MIN;
      }
    }

    if (is_all_child_linked) {
      if (is_sort)
        qsort(procs[proc_iter_index].child_procs, procs[proc_iter_index].child_num,
              sizeof(process *), proc_comp);
      else
        qsort(procs[proc_iter_index].child_procs, procs[proc_iter_index].child_num,
              sizeof(process *), proc_comp_default);

      if (procs[proc_iter_index].pid == 1)
        root = &procs[proc_iter_index];
      proc_iter_index++;
      parent_pid = procs[proc_iter_index].pid;
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

int print_node(process *node) {
  if (is_print_pid) {
    char message[MESSAGE_MAX_SIZE];
    sprintf(message, "%s(%d)", node->comm, node->pid);
    printf("%s", message);
    return strlen(message);
  } else {
    printf("%s", node->comm);
    return strlen(node->comm);
  }
}

void print_tree(char *forward, char *forward_end, process *root) {
  static int is_print_forward = 0;
  if (is_print_forward) {
    char forward_[MESSAGE_MAX_SIZE];
    strcpy(forward_, forward);
    forward_[forward_end - forward] = '\0';
    printf("%s", forward_);
    is_print_forward = 0;
  }
  if (root->child_num == 1) {
    printf(UTF_H UTF_H UTF_H);
    int add_space_len = 3 + print_node(root->child_procs[0]);
    forward_end = add_n_space(forward_end, add_space_len);
    print_tree(forward, forward_end, root->child_procs[0]);
  } else if (root->child_num > 1) {
    printf(UTF_H UTF_HD UTF_H);
    char *forward_end_child = forward_end + sprintf(forward_end, " " UTF_V " ");
    int add_space_len = print_node(root->child_procs[0]);
    forward_end_child = add_n_space(forward_end_child, add_space_len);
    print_tree(forward, forward_end_child, root->child_procs[0]);

    for (int i = 1; i < root->child_num - 1; i++) {
      if (is_print_forward) {
        char forward_[MESSAGE_MAX_SIZE];
        strcpy(forward_, forward);
        forward_[forward_end - forward] = '\0';
        printf("%s", forward_);
        is_print_forward = 0;
      }
      printf(" " UTF_VR UTF_H);
      forward_end_child -= add_space_len;
      add_space_len = print_node(root->child_procs[i]);
      forward_end_child = add_n_space(forward_end_child, add_space_len);
      print_tree(forward, forward_end_child, root->child_procs[i]);
    }
    if (is_print_forward) {
      char forward_[MESSAGE_MAX_SIZE];
      strcpy(forward_, forward);
      forward_[forward_end - forward] = '\0';
      printf("%s", forward_);
      is_print_forward = 0;
    }
    printf(" " UTF_UR UTF_H);
    forward_end_child -= add_space_len + strlen(" " UTF_V " ");
    add_space_len = 3 + print_node(root->child_procs[root->child_num - 1]);
    forward_end_child = add_n_space(forward_end_child, add_space_len);
    print_tree(forward, forward_end_child,
               root->child_procs[root->child_num - 1]);
  } else {
    printf("\n");
    is_print_forward = 1;
  }
}

int main(int argc, char *argv[]) {
  for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    if (strcmp(argv[i], "-V") == 0) {
      printf("pstree from os-workbench-2022, version 0.0.1");
      break;
    }
    if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--show-pids") == 0)
      is_print_pid = 1;
    if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--numeric-sort") == 0)
      is_sort = 1;
  }
  assert(!argv[argc]);
  char fst_tree_message[MESSAGE_MAX_SIZE];
  char snd_tree_message[MESSAGE_MAX_SIZE];
  readProcess();
  process *root = createTree();
  char line_buff[MESSAGE_MAX_SIZE] = {0};
  int n = print_node(root);
  char *forward_end = add_n_space(line_buff, n);
  print_tree(line_buff, forward_end, root);
  return 0;
}
