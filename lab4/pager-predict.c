#include <stdio.h>
#include <stdlib.h>

#include "simulator.h"

/* Static vars */
static int initialized = 0;
static int tick = 1; // artificial time
static int timestamps[MAXPROCESSES][MAXPROCPAGES];

void swapLRU(int proc, int page, int tick, Pentry q[MAXPROCESSES],
             int timestamps[MAXPROCESSES][MAXPROCPAGES]) {

  int curr_page;
  int old_page;
  // if we can't swap in, swap out oldest page
  // tick + 1 is ALWAYS greater than any timestamp
  int old_page_time = tick + 1;
  for (curr_page = 0; curr_page < q[proc].npages; curr_page++) {
    // if this is the oldest, see if we can swap out
    if (timestamps[proc][curr_page] < old_page_time) {
      // if this page isn't in memory, no reason to swap out
      if (q[proc].pages[curr_page]) {
        old_page_time = timestamps[proc][curr_page];
        old_page = curr_page;
      }
    }
  }

  pageout(proc, old_page);
  pagein(proc, page);
}

void pageit(Pentry q[MAXPROCESSES]) {
  /* Local vars */
  int proctmp;
  int pagetmp;

  /* initialize static vars on first run */
  if(!initialized){
    for(proctmp=0; proctmp < MAXPROCESSES; proctmp++){
      for(pagetmp=0; pagetmp < MAXPROCPAGES; pagetmp++){
        timestamps[proctmp][pagetmp] = 0;
      }
    }
    initialized = 1;
  }

  int proc;
  int pc;
  int page;
  int i;

  for (proc = 0; proc < MAXPROCESSES; proc++) {
    // if process is not active, move along
    if (!q[proc].active) {
      continue;
    }

    for (i = 0; i < 5; i++) {
      pc = q[proc].pc + i;
      page = pc / PAGESIZE;

      // set the timestamp of when we used this
      timestamps[proc][page] = tick;

      // if page is swapped out
      if (!q[proc].pages[page]) {
        // try to swap it in
        if (!pagein(proc, page)) {
          swapLRU(proc, page, tick, q, timestamps);
        }
      }
    }
  }

  /* advance time for next pageit iteration */
  tick++;
}
