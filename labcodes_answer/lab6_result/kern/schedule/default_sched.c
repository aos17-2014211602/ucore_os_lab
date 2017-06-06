#include <defs.h>
#include <list.h>
#include <proc.h>
#include <assert.h>
#include <default_sched.h>

#define USE_SKEW_HEAP 0

/* You should define the BigStride constant here*/
/* LAB6: YOUR CODE */
//#define BIG_STRIDE    0x7FFFFFFF /* ??? */
#define WARPL    10
#define WARPU    20
/* The compare function for two skew_heap_node_t's and the
 * corresponding procs*/
static int
proc_bvt_comp_f(void *a, void *b)
{
     struct proc_struct *p = le2proc(a, lab6_run_pool);
     struct proc_struct *q = le2proc(b, lab6_run_pool);
     int32_t c = p->bvt_evt - q->bvt_evt;
     if (c > 0) return 1;
     else if (c == 0) return 0;
     else return -1;
}

/*
 * bvt_init initializes the run-queue rq with correct assignment for
 * member variables, including:
 *
 *   - run_list: should be a empty list after initialization.
 *   - lab6_run_pool: NULL
 *   - proc_num: 0
 *   - max_time_slice: no need here, the variable would be assigned by the caller.
 *
 * hint: see proj13.1/libs/list.h for routines of the list structures.
 */
static void
bvt_init(struct run_queue *rq) {
     /* LAB6: YOUR CODE */
     list_init(&(rq->run_list));
     rq->lab6_run_pool = NULL;
     rq->proc_num = 0;
}

/*
 * bvt_enqueue inserts the process ``proc'' into the run-queue
 * ``rq''. The procedure should verify/initialize the relevant members
 * of ``proc'', and then put the ``lab6_run_pool'' node into the
 * queue(since we use priority queue here). The procedure should also
 * update the meta date in ``rq'' structure.
 *
 * proc->time_slice denotes the time slices allocation for the
 * process, which should set to rq->max_time_slice.
 * 
 * hint: see proj13.1/libs/skew_heap.h for routines of the priority
 * queue structures.
 */
static void
bvt_enqueue(struct run_queue *rq, struct proc_struct *proc) {
     /* LAB6: YOUR CODE */
#if USE_SKEW_HEAP
     rq->lab6_run_pool =
          skew_heap_insert(rq->lab6_run_pool, &(proc->lab6_run_pool), proc_bvt_comp_f);
#else
     assert(list_empty(&(proc->run_link)));
     list_add_before(&(rq->run_list), &(proc->run_link));
#endif
     if (proc->time_slice == 0 || proc->time_slice > rq->max_time_slice) {
          proc->time_slice = rq->max_time_slice;
     }
     proc->rq = rq;
     rq->proc_num ++;
}

/*
 * bvt_dequeue removes the process ``proc'' from the run-queue
 * ``rq'', the operation would be finished by the skew_heap_remove
 * operations. Remember to update the ``rq'' structure.
 *
 * hint: see proj13.1/libs/skew_heap.h for routines of the priority
 * queue structures.
 */
static void
bvt_dequeue(struct run_queue *rq, struct proc_struct *proc) {
     /* LAB6: YOUR CODE */
#if USE_SKEW_HEAP
     rq->lab6_run_pool =
          skew_heap_remove(rq->lab6_run_pool, &(proc->lab6_run_pool), proc_bvt_comp_f);
#else
     assert(!list_empty(&(proc->run_link)) && proc->rq == rq);
     list_del_init(&(proc->run_link));
#endif
     rq->proc_num --;
}
/*
 * bvt_pick_next pick the element from the ``run-queue'', with the
 * minimum value of bvt, and returns the corresponding process
 * pointer. The process pointer would be calculated by macro le2proc,
 * see proj13.1/kern/process/proc.h for definition. Return NULL if
 * there is no process in the queue.
 *
 * When one proc structure is selected, remember to update the bvt
 * property of the proc. (bvt += BIG_STRIDE / priority)
 *
 * hint: see proj13.1/libs/skew_heap.h for routines of the priority
 * queue structures.
 */
static struct proc_struct *
bvt_pick_next(struct run_queue *rq) {
     /* LAB6: YOUR CODE */
#if USE_SKEW_HEAP
     if (rq->lab6_run_pool == NULL) return NULL;
     struct proc_struct *p = le2proc(rq->lab6_run_pool, lab6_run_pool);
#else
     list_entry_t *le = list_next(&(rq->run_list));

     if (le == &rq->run_list)
          return NULL;
     
     struct proc_struct *p = le2proc(le, run_link);
     le = list_next(le);
     while (le != &rq->run_list)
     {
          struct proc_struct *q = le2proc(le, run_link);
          if ((int32_t)(p->bvt_evt - q->bvt_evt) > 0 && q->bvt_warp_timer < WARPL)
               p = q;
          le = list_next(le);
     }
#endif
     p->bvt_avt += 1.0 / p->lab6_priority;
     p->bvt_evt = p->bvt_avt - p->bvt_warp;
     if(p->bvt_warp) p->bvt_unwarp_timer = 0;
     return p;
}

/*
 * bvt_proc_tick works with the tick event of current process. You
 * should check whether the time slices for current process is
 * exhausted and update the proc struct ``proc''. proc->time_slice
 * denotes the time slices left for current
 * process. proc->need_resched is the flag variable for process
 * switching.
 */
static void
bvt_proc_tick(struct run_queue *rq, struct proc_struct *proc) {
     /* LAB6: YOUR CODE */
     if (proc->time_slice > 0) {
          proc->time_slice --;
          if(!proc->bvt_warp){proc->bvt_warp_timer ++ ;}
          list_entry_t *le = list_next(&(rq->run_list));

         if (le == &rq->run_list)
         {}
         
         //struct proc_struct *p = le2proc(le, run_link);
         le = list_next(le);
         while (le != &rq->run_list)
         {
              struct proc_struct *q = le2proc(le, run_link);
              if (!q->bvt_warp)
                   q->bvt_unwarp_timer ++;
              if (q->bvt_unwarp_timer > WARPU)
                  q->bvt_warp_timer = 0;
         }
     }
     if (proc->time_slice == 0) {
          proc->need_resched = 1;
     }
}

struct sched_class default_sched_class = {
     .name = "bvt_scheduler",
     .init = bvt_init,
     .enqueue = bvt_enqueue,
     .dequeue = bvt_dequeue,
     .pick_next = bvt_pick_next,
     .proc_tick = bvt_proc_tick,
};

