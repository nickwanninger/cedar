/*
 * MIT License
 *
 * Copyright (c) 2018 Nick Wanninger
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include <cedar/channel.h>
#include <cedar/event_loop.h>
#include <cedar/globals.h>
#include <cedar/modules.h>
#include <cedar/object/fiber.h>
#include <cedar/object/lambda.h>
#include <cedar/objtype.h>
#include <cedar/scheduler.h>
#include <cedar/thread.h>
#include <cedar/types.h>
#include <unistd.h>
#include <uv.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <flat_hash_map.hpp>
#include <mutex>


using namespace cedar;


/**
 * The number of jobs that have yet to be completed. This number is used
 * to track if the main thread should exit or not, and if a certain thread
 * should wait or not. When a job is pushed to the scheduler, this value is
 * incremented, and is decremented when a job is deemed completed.
 *
 * This number is an unsigned 64 bit integer, because cedar will eventually
 * be able to run a huge number of these jobs concurrently. :)
 */
static std::atomic<u64> pending_job_count;

/**
 * ncpus is how many worker threads to maintain at any given time.
 */
static unsigned ncpus = 8;



/**
 * the listing of all worker threads and a fast lookup map to the same
 * information
 * TODO: more docs
 */
static std::mutex worker_thread_mutex;
static std::vector<worker_thread *> worker_threads;
static ska::flat_hash_map<std::thread::id, worker_thread *> worker_thread_map;



/**
 * time_ms returns the number of milliseconds from the unix epoch. This is
 * useful for timing fibers
 * TODO: remove the need for this entirely by using libuv
 */
static u64 time_ms(void);


static std::mutex jid_mutex;
int next_jid = 0;


static worker_thread *lookup_or_create_worker(std::thread::id tid) {
  std::unique_lock<std::mutex> lock(worker_thread_mutex);
  worker_thread *w = worker_thread_map[tid];
  if (w == nullptr) {
    // create w
    w = new worker_thread();
    w->tid = tid;
    worker_thread_map[tid] = w;
    worker_threads.push_back(w);
  }
  return w;
}


void cedar::add_job(fiber *f) {
  worker_thread_mutex.lock();
  int ind = rand() % worker_threads.size();
  worker_threads[ind]->local_queue.push(f);
  worker_thread_mutex.unlock();
}



/**
 * construct and start a worker thread by creating it and pushing it to
 * the worker pool atomically
 *
 * A worker thread will sit and wait for jobs in the following order:
 *     A) A job is waiting in the local job queue
 *     B) a job to be accessable in the global queue
 *     C) POSSIBLY, steal work from other worker threads.
 */
static std::thread spawn_worker_thread(void) {
  return std::thread([](void) -> void {
    register_thread();
    worker_thread *my_thread =
        lookup_or_create_worker(std::this_thread::get_id());
    while (true) volunteer(my_thread);
  });
}



/**
 * schedule a single job on the caller thread, it's up to the caller to manage
 * where the job goes after the job yields
 */
static bool schedule_job(fiber *proc) {
  static int sched_time = 2;
  static bool read_env = false;
  static const char *SCHED_TIME_ENV = getenv("CDRTIMESLICE");
  if (SCHED_TIME_ENV != nullptr && !read_env) {
    sched_time = atol(SCHED_TIME_ENV);
    if (sched_time < 2)
      throw cedar::make_exception("$CDRTIMESLICE must be larger than 2ms");
  }
  read_env = true;

  if (proc == nullptr) return false;

  bool done = false;
  u64 time = time_ms();
  /* check if this process needs to be run or not */
  if (time >= (proc->last_ran + proc->sleep)) {
    run_context ctx;
    /* "context switch" into the job for sched_time milliseconds
     * and return back here, with a modified ctx */
    proc->run(&ctx, sched_time);
    /* store the last time of execution in the job. This is an
     * approximation as getting the ms from epoch takes too long */
    proc->last_ran = time + sched_time;
    /* store how long the process should sleep for.
     * TODO: move this into libuv once that is implenented */
    proc->sleep = ctx.sleep_for;


    /* and increment the ticks for this job */
    proc->ticks++;
    done = proc->done;


    return !done;
  }
  return true;
}


/**
 * return if all work has been completed or not
 */
bool cedar::all_work_done(void) {
  // if there are no pending jobs, we must be done with all work.
  return pending_job_count == 0;
}



static void init_scheduler(void) {
  // the number of worker threads is the number of cpus the host machine
  // has, minus one as the main thread does work
  ncpus = std::thread::hardware_concurrency();
  static const char *CDRMAXPROC = getenv("CDRMAXPROC");
  if (CDRMAXPROC != nullptr) ncpus = atol(CDRMAXPROC);

  // spawn 'ncpus' worker threads
  for (unsigned i = 0; i < ncpus; i++) spawn_worker_thread().detach();
}




void cedar::volunteer(worker_thread *worker) {
  if (worker == nullptr) {
    worker = lookup_or_create_worker(std::this_thread::get_id());
  }
  size_t pool_size = 0;
  int i1, i2;
  worker_thread *w1, *w2;

  /**
   * now that we have the worker_thread, we need to find work to do
   */


  // work is the thing that will eventually be done, while looking for work,
  // it will be set and checked for equality to nullptr. If at any point it
  // is not nullptr, it will immediately be scheduled.
  fiber *work = nullptr;



  // first check the local queue
  work = worker->local_queue.steal();
  if (work != nullptr) goto SCHEDULE;



  worker_thread_mutex.lock();
  // now look through other threads in the thread pool for work to steal
  pool_size = worker_threads.size();

  i1 = rand() % pool_size;
  i2 = rand() % pool_size;
  w1 = worker_threads[i1];
  w2 = worker_threads[i2];
  worker_thread_mutex.unlock();

  if (w1->local_queue.size() > w2->local_queue.size()) {
    work = w1->local_queue.steal();
  } else {
    work = w2->local_queue.steal();
  }

  if (work != nullptr) {
    goto SCHEDULE;
  }
  // sleep for 1 ms
  usleep(10);
  return;

SCHEDULE:


  if (work == nullptr) {
    return;
    throw std::logic_error("work is nullptr");
  }

  // switch into the work for a time slice and return here when done.
  bool replace = schedule_job(work);
  worker->ticks++;



  // since we did some work, we should put it back in the local queue
  // but only if it isn't done.
  if (work != nullptr && replace) {
    worker->local_queue.push(work);
  }
}




// forward declare this function
namespace cedar {
  void bind_stdlib(void);
};
void init_binding(cedar::vm::machine *m);



/**
 * this is the primary entry point for cedar, this function
 * should be called before ANY code is run.
 */
void cedar::init(void) {
  init_scheduler();
  // init_ev();
  type_init();
  init_binding(nullptr);
  bind_stdlib();
  core_mod = require("core");
}


/**
 * eval_lambda is an 'async' evaluator. This means it will add the
 * job to the thread pool with a future-like concept attached, then
 * run the scheduler until the fiber has completed it's work, then
 * return to the caller in C++. It works in this way so that calling
 * cedar functions from within C++ won't fully block the event loop
 * when someone tries to get a mutex lock or something from within
 * such a call
 */
ref cedar::eval_lambda(lambda *fn) {
  fiber f(fn);
  worker_thread *my_worker =
      lookup_or_create_worker(std::this_thread::get_id());
  my_worker->local_queue.push(&f);
  while (!f.done) {
    volunteer(my_worker);
  }
  return f.return_value;
}



ref cedar::call_function(lambda *fn, int argc, ref *argv, call_context *ctx) {
  if (fn->code_type == lambda::function_binding_type) {
    return fn->function_binding(argc, argv, ctx);
  }
  fn = fn->copy();
  fn->prime_args(argc, argv);
  return eval_lambda(fn);
}



/**
 * get the time since epoch in ms
 */
static u64 time_ms(void) {
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch());
  return ms.count();
}
