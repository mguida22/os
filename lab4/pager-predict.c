#include <stdio.h>
#include <stdlib.h>

#include "simulator.h"

struct node {
  int process;
  int page;
  int timestamp;
};

/* Static vars */
static int initialized = 0;
static int tick = 1; // artificial time
static struct node used[PHYSICALPAGES];
static int used_count = 0;

void pageit(Pentry q[MAXPROCESSES]) {
  int proc;
  int pc;
  int page;
  int old;
  int old_page_time;
  int i;

  /* initialize static vars on first run */
  if (!initialized) {
    for (i = 0; i < PHYSICALPAGES; i++) {
      used[i].process = -1;
      used[i].page = -1;
      used[i].timestamp = 2147483647;
    }

    initialized = 1;
  }

  for (proc = 0; proc < MAXPROCESSES; proc++) {
    // if process is not active, move along
    if (!q[proc].active) {
      continue;
    }

    pc = q[proc].pc;
    page = pc / PAGESIZE;

    // if page is swapped out
    if (!q[proc].pages[page]) {
      // try to swap it in
      if (pagein(proc, page)) {
        used[used_count].process = proc;
        used[used_count].page = page;
        used[used_count].timestamp = tick;

        used_count++;
      } else {
        // if we can't swap in, swap out oldest page
        // tick + 1 is ALWAYS greater than any timestamp
        old_page_time = tick + 1;
        for (i = 0; i < PHYSICALPAGES; i++) {
          // if this is the oldest, see if we can swap out
          if (used[i].timestamp < old_page_time) {
            old_page_time = used[i].timestamp;
            old = i;
          }
        }

        if (!pageout(used[old].process, used[old].page)) {
            printf("out\n");
            exit(1);
        }

        used[old].process = proc;
        used[old].page = page;
        used[old].timestamp = tick;

        if (!pagein(proc, page)) {
            printf("in\n");
            exit(1);
        }
      }
    }
  }

  /* advance time for next pageit iteration */
  tick++;
}
