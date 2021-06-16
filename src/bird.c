#include "util.h"
#include "vm.h"
#include "b_unistd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if !defined _WIN32 && !defined __CYGWIN__

#include <readline/history.h>
#include <readline/readline.h>

#endif

#include <setjmp.h>
#include <signal.h>
#include <fcntl.h>

#ifdef _WIN32
#include "win32.h"
#endif

static bool continue_repl = true;
sigjmp_buf ctrlc_buf;

void handle_signals(int sig_no) {
  if (sig_no == SIGINT) {
    printf("\n<KeyboardInterrupt>\n");
    continue_repl = false;
    siglongjmp(ctrlc_buf, 1);
  }
}

static void repl(b_vm *vm) {
  if (signal(SIGINT, handle_signals) == SIG_ERR) {
    printf("failed to register interrupts with kernel\n");
  }

  vm->is_repl = true;

  printf("Bird %s (running on BVM %s), REPL/Interactive mode = ON\n",
          BIRD_VERSION_STRING, BVM_VERSION);
  printf("%s, (Build time = %s, %s)\n", COMPILER, __DATE__, __TIME__);
  printf("Type \"exit()\" to quit, \"help()\" or \"credits()\" for more information\n");

  char *source = (char *) malloc(sizeof(char));
  memset(source, 0, sizeof(char));

  int current_line = 0;
  int brace_count = 0, paren_count = 0, bracket_count = 0, single_quote_count = 0, double_quote_count = 0;

  for (;;) {
    while (sigsetjmp(ctrlc_buf, 1) != 0);

    if (!continue_repl) {
      current_line = 0;
      brace_count = 0;
      paren_count = 0;
      bracket_count = 0;
      single_quote_count = 0;
      double_quote_count = 0;

      // reset source...
      memset(source, 0, strlen(source));
      continue_repl = true;
    }

    current_line++;

    const char *cursor = "> ";
    if (brace_count > 0 || bracket_count > 0 || paren_count > 0) {
      cursor = "| ";
    } else if (single_quote_count == 1 || double_quote_count == 1) {
      cursor = "  ";
    }

#if defined _WIN32 || defined __CYGWIN__
    char buffer[1024];
    printf(cursor);
    char *line = fgets(buffer, 1024, stdin);
    int line_length = strcspn(line, "\r\n");
    line[line_length] = 0;
#else
    char *line = readline(cursor);
    int line_length = (int) strlen(line);
#endif // _WIN32

    // terminate early if we receive a terminating command such as exit()
    if (strcmp(line, "exit()") == 0) {
      exit(EXIT_SUCCESS);
    }

#if !defined _WIN32 && !defined __CYGWIN__
    // allow user to navigate through past input in terminal...
    add_history(line);
#endif // !_WIN32

    // find count of { and }, ( and ), [ and ]
    for (int i = 0; i < line_length; i++) {
      // scope openers...
      if (line[i] == '{')
        brace_count++;
      else if (line[i] == '(')
        paren_count++;
      else if (line[i] == '[')
        bracket_count++;

      // quotes
      else if(((i == 0 && line[i] == '\'') || (line[i] == '\'' && line[i - 1] != '\\')) && double_quote_count == 0) {
        if(single_quote_count == 0) 
          single_quote_count = 1; 
        else 
          single_quote_count = 0;
      }
      else if(((i == 0 && line[i] == '"') || (line[i] == '"' && line[i - 1] != '\\')) && single_quote_count == 0){
        if(double_quote_count == 0) 
          double_quote_count = 1; 
        else 
          double_quote_count = 0;
      }

        // scope closers...
      else if (line[i] == '}' && brace_count > 0)
        brace_count--;
      else if (line[i] == ')' && paren_count > 0)
        paren_count--;
      else if (line[i] == ']' && bracket_count > 0)
        bracket_count--;
    }

    source = append_strings(source, line);
    if (line_length > 0) {
      source = append_strings(source, "\n");
    }

#ifndef _WIN32
    free(line);
#endif // !_WIN32

    if (bracket_count == 0 && paren_count == 0 && brace_count == 0 && single_quote_count == 0 && double_quote_count == 0) {

      interpret(vm, source, "<repl>");

      fflush(stdout); // flush all outputs

      // reset source...
      memset(source, 0, strlen(source));
    }
  }
}

static void run_file(b_vm *vm, const char *file) {
  char *source = read_file(file);
  if (source == NULL) {
    fprintf(stderr, "(Bird):\n  Launch aborted for %s\n  Reason: %s\n", file, strerror(errno));
    exit(EXIT_FAILURE);
  }

  b_ptr_result result = interpret(vm, source, file);
  free(source);

  fflush(stdout);

  if (result == PTR_COMPILE_ERR)
    exit(EXIT_COMPILE);
  if (result == PTR_RUNTIME_ERR)
    exit(EXIT_RUNTIME);
}

void show_usage(char *argv[]) {
  fprintf(stderr, "Usage: %s [-[d | j | v | g]] [filename]\n", argv[0]);
  fprintf(stderr, "   -v    Show version string.\n");
  fprintf(stderr, "   -d    Show generated bytecode.\n");
  fprintf(stderr, "   -j    Show stack objects during execution.\n");
  fprintf(stderr, "   -g    Sets the minimum heap size in kilobytes before the GC\n"
                  "         can start. [Default = %d (%dmb)]\n", DEFAULT_GC_START / 1024,
                  DEFAULT_GC_START / (1024 * 1024));
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {

  bool should_debug_stack = false;
  bool should_print_bytecode = false;
  int next_gc_start = DEFAULT_GC_START;

  if(argc > 1) {
    int opt;
    while ((opt = getopt(argc, argv, "djvg:")) != -1) {
      switch (opt) {
        case 'd': should_print_bytecode = true; break;
        case 'j': should_debug_stack = true; break;
        case 'v': {
          printf("Bird " BIRD_VERSION_STRING " (running on BVM " BVM_VERSION ")\n");
          return 0;
        }
        case 'g': {
          int next = (int)strtol(optarg, NULL, 10);
          if(next > 0) {
            next_gc_start = next * 1024; // expected value is in kilobytes
          }
          break;
        }
        default: show_usage(argv); break;
      }
    }
  }

  b_vm *vm = (b_vm *) malloc(sizeof(b_vm));
  if (vm != NULL) {
    memset(vm, 0, sizeof(b_vm));
    init_vm(vm);

    // set vm options...
    vm->should_debug_stack = should_debug_stack;
    vm->should_print_bytecode = should_print_bytecode;
    vm->next_gc = next_gc_start;

    /*// forcing printf buffering for TTYs and terminals
    if (isatty(fileno(stdout))) {
      char buffer[8192];
      setvbuf(stdout, buffer, _IOFBF, sizeof(buffer));
    }*/

    if (argc == 1 || argc <= optind) {
      repl(vm);
    }
    else {
      run_file(vm, argv[optind]);
    }

    free_vm(vm);
    return EXIT_SUCCESS;
  }

  fprintf(stderr, "Device out of memory.");
  exit(EXIT_FAILURE);
}