#include "sentry_batcher.h"
#include "sentry_alloc.h"
#include "sentry_cpu_relax.h"
#include "sentry_options.h"
#include "sentry_utils.h"

// The batcher thread sleeps for this interval between flush cycles.
// When the timer fires and there are items in the buffer, they are flushed
// regardless of how recently they were enqueued.
#define SENTRY_BATCHER_FLUSH_INTERVAL_MS 5000
#define SENTRY_BATCHER_THREAD_NAME "sentry-batcher"

#ifdef SENTRY_UNITTEST
#    ifdef SENTRY_PLATFORM_WINDOWS
#        include <windows.h>
#        define sleep_ms(MILLISECONDS) Sleep(MILLISECONDS)
#    else
#        include <unistd.h>
#        define sleep_ms(MILLISECONDS) usleep(MILLISECONDS * 1000)
#    endif
#endif

sentry_batcher_t *
sentry__batcher_new(size_t num_queues, sentry_threadpool_t *threadpool)
{
    if (num_queues == 0
        || num_queues > SIZE_MAX / sizeof(sentry_batcher_queue_t)) {
        return NULL;
    }
    sentry_batcher_t *batcher = SENTRY_MAKE(sentry_batcher_t);
    if (!batcher) {
        return NULL;
    }
    batcher->queues
        = sentry__calloc(num_queues, sizeof(sentry_batcher_queue_t));
    if (!batcher->queues) {
        sentry_free(batcher);
        return NULL;
    }
    batcher->refcount = 1;
    batcher->thread_state = (long)SENTRY_BATCHER_THREAD_STOPPED;
    batcher->threadpool = threadpool;
    batcher->num_queues = num_queues;
    sentry__waitable_flag_init(&batcher->request_flush);
    for (size_t i = 0; i < num_queues; i++) {
        sentry_batcher_queue_t *queue = &batcher->queues[i];
        queue->batcher = batcher;
        queue->data_category = SENTRY_DATA_CATEGORY_MAX;
        sentry__mutex_init(&queue->task_wait_mutex);
        sentry__cond_init(&queue->task_wait_cond);
    }
    sentry__thread_init(&batcher->batching_thread);
    return batcher;
}

/**
 * Releases any items left in the buffers that were enqueued after the final
 * flush (e.g. by a producer that acquired a ref before shutdown).
 */
static void
buffer_drain(sentry_batcher_buffer_t *buf)
{
    const long n = MIN(buf->index, SENTRY_BATCHER_QUEUE_LENGTH);
    for (long i = 0; i < n; i++) {
        sentry_value_decref(buf->items[i]);
    }
    buf->index = 0;
}

void
sentry__batcher_release(sentry_batcher_t *batcher)
{
    if (!batcher || sentry__atomic_fetch_and_add(&batcher->refcount, -1) != 1) {
        return;
    }
    for (size_t i = 0; i < batcher->num_queues; i++) {
        sentry_batcher_queue_t *queue = &batcher->queues[i];
        for (long j = 0; j < SENTRY_BATCHER_BUFFER_COUNT; j++) {
            buffer_drain(&queue->buffers[j]);
        }
        sentry__cond_free(&queue->task_wait_cond);
        sentry__mutex_free(&queue->task_wait_mutex);
    }
    sentry__dsn_decref(batcher->dsn);
    sentry__thread_free(&batcher->batching_thread);
    sentry_free(batcher->queues);
    sentry_free(batcher);
}

void
sentry__batcher_set_queue(sentry_batcher_t *batcher, size_t index,
    sentry_data_category_t data_category, sentry_batch_func_t batch_func)
{
    if (!batcher) {
        return;
    }
    sentry_batcher_queue_t *queue = &batcher->queues[index];
    queue->data_category = data_category;
    queue->batch_func = batch_func;
}

static inline void
lock_ref(sentry_batcher_ref_t *ref)
{
    while (!sentry__atomic_compare_swap(&ref->lock, 0, 1)) {
        sentry__cpu_relax();
    }
}

static inline void
unlock_ref(sentry_batcher_ref_t *ref)
{
    sentry__atomic_store(&ref->lock, 0);
}

static sentry_batcher_t *
swap_batcher(sentry_batcher_ref_t *ref, sentry_batcher_t *batcher)
{
#ifdef SENTRY_PLATFORM_WINDOWS
    return (sentry_batcher_t *)InterlockedExchangePointer(
        (volatile PVOID *)&ref->ptr, batcher);
#else
    return __atomic_exchange_n(&ref->ptr, batcher, __ATOMIC_SEQ_CST);
#endif
}

static sentry_batcher_t *
load_batcher(sentry_batcher_ref_t *ref)
{
#ifdef SENTRY_PLATFORM_WINDOWS
    return (sentry_batcher_t *)InterlockedCompareExchangePointer(
        (volatile PVOID *)&ref->ptr, NULL, NULL);
#else
    return __atomic_load_n(&ref->ptr, __ATOMIC_SEQ_CST);
#endif
}

sentry_batcher_t *
sentry__batcher_acquire(sentry_batcher_ref_t *ref)
{
    lock_ref(ref);
    sentry_batcher_t *batcher = load_batcher(ref);
    if (batcher) {
        sentry__atomic_fetch_and_add(&batcher->refcount, 1);
    }
    unlock_ref(ref);
    return batcher;
}

sentry_batcher_t *
sentry__batcher_incref(sentry_batcher_t *batcher)
{
    if (batcher) {
        sentry__atomic_fetch_and_add(&batcher->refcount, 1);
    }
    return batcher;
}

sentry_batcher_t *
sentry__batcher_pin(sentry_batcher_ref_t *ref)
{
    sentry__atomic_fetch_and_add(&ref->pins, 1);
    sentry_batcher_t *batcher = load_batcher(ref);
    if (!batcher) {
        sentry__atomic_fetch_and_add(&ref->pins, -1);
    }
    return batcher;
}

void
sentry__batcher_unpin(sentry_batcher_ref_t *ref)
{
    sentry__atomic_fetch_and_add(&ref->pins, -1);
}

sentry_batcher_t *
sentry__batcher_swap(sentry_batcher_ref_t *ref, sentry_batcher_t *batcher)
{
    lock_ref(ref);
    sentry_batcher_t *old = swap_batcher(ref, batcher);
    unlock_ref(ref);
    while (old && sentry__atomic_fetch(&ref->pins) > 0) {
        sentry__cpu_relax();
    }
    return old;
}

// Use a sleep spinner around a monotonic timer so we don't syscall sleep from
// a signal handler. While this is strictly needed only there, there is no
// reason not to use the same implementation across platforms.
static void
crash_safe_sleep_ms(uint64_t delay_ms)
{
    const uint64_t start = sentry__monotonic_time();
    const uint64_t end = start + delay_ms;
    while (sentry__monotonic_time() < end) {
        for (int i = 0; i < 64; i++) {
            sentry__cpu_relax();
        }
    }
}

static bool
crash_safe_spin_wait(int attempts, void *UNUSED(data))
{
    if (attempts > 200) {
        return false;
    }
    // backoff max-wait with max_attempts = 200 based sleep slots:
    // 9ms + 450ms + 1010ms = 1500ish ms
    const uint32_t sleep_time = (attempts < 10) ? 1 : (attempts < 100) ? 5 : 10;
    crash_safe_sleep_ms(sleep_time);
    return true;
}

// Rotate the active buffer so producers can continue while the consumer is
// busy. Producers and the consumer may both rotate, so use CAS.
static bool
rotate_buffer(sentry_batcher_queue_t *queue, long old_idx)
{
    const long new_idx = (old_idx + 1) % SENTRY_BATCHER_BUFFER_COUNT;
    sentry_batcher_buffer_t *old_buf = &queue->buffers[old_idx];
    sentry_batcher_buffer_t *new_buf = &queue->buffers[new_idx];

    // The consumer resets a buffer before clearing `sealed`.
    if (sentry__atomic_fetch(&new_buf->sealed) != 0
        || sentry__atomic_fetch(&new_buf->index) != 0
        || sentry__atomic_fetch(&new_buf->adding) != 0) {
        return sentry__atomic_fetch(&queue->active_idx) != old_idx;
    }

    // Seal the old buffer before publishing the new active buffer.
    sentry__atomic_store(&old_buf->sealed, 1);

    // Make the next buffer active (after this we're good to go producer side).
    if (!sentry__atomic_compare_swap(&queue->active_idx, old_idx, new_idx)) {
        return sentry__atomic_fetch(&queue->active_idx) != old_idx;
    }
    return true;
}

typedef enum {
    SENTRY_BATCH_TASK_PENDING = 0,
    SENTRY_BATCH_TASK_RUNNING = 1,
    SENTRY_BATCH_TASK_READY = 2,
    SENTRY_BATCH_TASK_COMPLETING = 3,
    SENTRY_BATCH_TASK_DUMPED = 4,
} sentry_batch_task_state_t;

typedef struct sentry_batch_task_s {
    struct sentry_batch_task_s *next;
    sentry_batcher_queue_t *queue;
    sentry_envelope_t *envelope;
    sentry_value_t items;
    long state;
} sentry_batch_task_t;

static void
lock_tasks(sentry_batcher_queue_t *queue)
{
    while (!sentry__atomic_compare_swap(&queue->task_lock, 0, 1)) {
        sentry__cpu_relax();
    }
}

static bool
lock_tasks_crash_safe(sentry_batcher_queue_t *queue)
{
    int attempts = 0;
    while (!sentry__atomic_compare_swap(&queue->task_lock, 0, 1)) {
        if (!crash_safe_spin_wait(++attempts, NULL)) {
            return false;
        }
    }
    return true;
}

static void
unlock_tasks(sentry_batcher_queue_t *queue)
{
    sentry__atomic_store(&queue->task_lock, 0);
}

static void
batch_task_link(sentry_batch_task_t *task)
{
    sentry_batcher_queue_t *queue = task->queue;
    lock_tasks(queue);
    task->next = queue->tasks;
    queue->tasks = task;
    unlock_tasks(queue);
}

static bool
batch_task_unlink_locked(sentry_batch_task_t *task)
{
    sentry_batch_task_t *prev = NULL;
    sentry_batcher_queue_t *queue = task->queue;
    sentry_batch_task_t *cur = queue->tasks;
    while (cur) {
        if (cur == task) {
            if (prev) {
                prev->next = cur->next;
            } else {
                queue->tasks = cur->next;
            }
            task->next = NULL;
            return queue->tasks == NULL;
        }
        prev = cur;
        cur = cur->next;
    }
    return false;
}

static void
batch_task_unlink(sentry_batch_task_t *task)
{
    sentry_batcher_queue_t *queue = task->queue;
    sentry__mutex_lock(&queue->task_wait_mutex);
    lock_tasks(queue);
    const bool drained = batch_task_unlink_locked(task);
    unlock_tasks(queue);
    if (drained) {
        sentry__cond_wake_all(&queue->task_wait_cond);
    }
    sentry__mutex_unlock(&queue->task_wait_mutex);
}

static void
batch_task_wait_all(sentry_batcher_queue_t *queue)
{
    sentry__mutex_lock(&queue->task_wait_mutex);
    while (true) {
        lock_tasks(queue);
        const bool done = queue->tasks == NULL;
        unlock_tasks(queue);
        if (done) {
            break;
        }
        sentry__cond_wait(&queue->task_wait_cond, &queue->task_wait_mutex);
    }
    sentry__mutex_unlock(&queue->task_wait_mutex);
}

static void
process_batch_sync(sentry_batcher_queue_t *queue, sentry_envelope_t *envelope,
    sentry_value_t items, bool crash_safe)
{
    queue->batch_func(envelope, items);
    sentry_value_decref(items);

    if (crash_safe || sentry__atomic_fetch(&queue->batcher->crash_flush)) {
        // Write directly to disk to avoid transport queuing during
        // crash
        sentry__run_write_envelope(queue->batcher->run, envelope);
        sentry_envelope_free(envelope);
    } else if (!sentry__run_should_skip_upload(queue->batcher->run)) {
        // Normal operation: use transport for HTTP transmission
        sentry__transport_send_envelope(queue->batcher->transport, envelope);
    } else {
        sentry_envelope_free(envelope);
    }
}

static void
process_batch_sync_ordered(sentry_batcher_queue_t *queue,
    sentry_envelope_t *envelope, sentry_value_t items)
{
    // Preserve FIFO order when falling back after earlier batches were
    // accepted by the serialization pool.
    batch_task_wait_all(queue);
    process_batch_sync(queue, envelope, items, false);
}

static void
batch_task_exec(void *task_data)
{
    sentry_batch_task_t *task = task_data;
    sentry_batcher_queue_t *queue = task->queue;

    lock_tasks(queue);
    // Crash flushing may have claimed and dumped this task before its worker
    // started, leaving nothing to execute.
    if (!sentry__atomic_compare_swap(&task->state, SENTRY_BATCH_TASK_PENDING,
            SENTRY_BATCH_TASK_RUNNING)) {
        unlock_tasks(queue);
        return;
    }
    unlock_tasks(queue);

    queue->batch_func(task->envelope, task->items);
    sentry_value_decref(task->items);
    task->items = sentry_value_new_null();
    lock_tasks(queue);
    sentry__atomic_store(&task->state, SENTRY_BATCH_TASK_READY);
    unlock_tasks(queue);
}

static void
batch_task_complete(void *task_data)
{
    sentry_batch_task_t *task = task_data;
    sentry_batcher_queue_t *queue = task->queue;

    lock_tasks(queue);
    const long state = sentry__atomic_fetch(&task->state);
    if (state == SENTRY_BATCH_TASK_DUMPED) {
        unlock_tasks(queue);
        batch_task_unlink(task);
        return;
    }
    sentry__atomic_store(&task->state, SENTRY_BATCH_TASK_COMPLETING);
    unlock_tasks(queue);

    if (sentry__atomic_fetch(&queue->batcher->crash_flush)) {
        sentry__run_write_envelope(queue->batcher->run, task->envelope);
        sentry_envelope_free(task->envelope);
    } else if (!sentry__run_should_skip_upload(queue->batcher->run)) {
        // Normal operation: use transport for HTTP transmission
        sentry__transport_send_envelope(
            queue->batcher->transport, task->envelope);
    } else {
        sentry_envelope_free(task->envelope);
    }
    task->envelope = NULL;

    batch_task_unlink(task);
}

static void
batch_task_free(sentry_batch_task_t *task)
{
    sentry_value_decref(task->items);
    sentry_envelope_free(task->envelope);
    sentry_free(task);
}

static void
batch_task_complete_and_cleanup(void *task_data)
{
    sentry_batch_task_t *task = task_data;
    batch_task_complete(task);
    batch_task_free(task);
}

typedef struct {
    sentry_envelope_t *envelope;
    sentry_value_t items;
    bool serialize;
} sentry_batch_dump_t;

static bool
batch_task_claim_dump(sentry_batch_task_t *task, sentry_batch_dump_t *dump)
{
    const long state = sentry__atomic_fetch(&task->state);
    if (state == SENTRY_BATCH_TASK_PENDING) {
        dump->items = task->items;
        task->items = sentry_value_new_null();
        dump->serialize = true;
    } else if (state == SENTRY_BATCH_TASK_READY) {
        dump->serialize = false;
    } else {
        return false;
    }

    // Completion may free the task as soon as the lock is released.
    dump->envelope = task->envelope;
    task->envelope = NULL;
    sentry__atomic_store(&task->state, SENTRY_BATCH_TASK_DUMPED);
    return true;
}

static void
batch_task_dump(sentry_batcher_queue_t *queue, sentry_batch_dump_t *dump)
{
    if (dump->serialize) {
        queue->batch_func(dump->envelope, dump->items);
        sentry_value_decref(dump->items);
    }
    sentry__run_write_envelope(queue->batcher->run, dump->envelope);
    sentry_envelope_free(dump->envelope);
}

static void
batch_task_dump_pending_all(sentry_batcher_queue_t *queue)
{
    int attempts = 0;
    while (true) {
        bool has_busy = false;
        bool claimed = false;
        sentry_batch_dump_t dump = { 0 };
        if (!lock_tasks_crash_safe(queue)) {
            return;
        }
        for (sentry_batch_task_t *task = queue->tasks; task;
            task = task->next) {
            if (batch_task_claim_dump(task, &dump)) {
                claimed = true;
                break;
            }
            const long state = sentry__atomic_fetch(&task->state);
            has_busy = has_busy || state == SENTRY_BATCH_TASK_RUNNING
                || state == SENTRY_BATCH_TASK_COMPLETING;
        }
        unlock_tasks(queue);

        if (claimed) {
            batch_task_dump(queue, &dump);
            attempts = 0;
            continue;
        }
        if (!has_busy) {
            return;
        }

        const int max_attempts = 200;
        if (++attempts > max_attempts) {
            return;
        }
        const uint32_t sleep_time = (attempts < 10) ? 1
            : (attempts < 100)                      ? 5
                                                    : 10;
        crash_safe_sleep_ms(sleep_time);
    }
}

static void
process_batch(
    sentry_batcher_queue_t *queue, sentry_value_t items, bool crash_safe)
{
    sentry_envelope_t *envelope
        = sentry__envelope_new_with_dsn(queue->batcher->dsn);
    if (crash_safe) {
        process_batch_sync(queue, envelope, items, true);
        return;
    }

    sentry_batch_task_t *task = SENTRY_MAKE(sentry_batch_task_t);
    if (!task) {
        SENTRY_WARN("serializing telemetry batch synchronously: "
                    "serialization task allocation failed");
        process_batch_sync_ordered(queue, envelope, items);
        return;
    }

    task->queue = queue;
    task->envelope = envelope;
    task->items = items;
    task->state = SENTRY_BATCH_TASK_PENDING;
    batch_task_link(task);

    if (!queue->batcher->threadpool) {
        batch_task_exec(task);
        batch_task_complete(task);
        batch_task_free(task);
        return;
    }

    if (sentry__threadpool_start(queue->batcher->threadpool) != 0) {
        SENTRY_WARN("serializing telemetry batch synchronously: "
                    "serialization pool unavailable");
        batch_task_unlink(task);
        process_batch_sync_ordered(queue, envelope, items);
        sentry_free(task);
        return;
    }

    const int result = sentry__threadpool_submit(queue->batcher->threadpool,
        batch_task_exec, batch_task_complete_and_cleanup, NULL, task);
    if (result != 0) {
        batch_task_unlink(task);
        if (result > 0) {
            sentry__client_report_discard(SENTRY_DISCARD_REASON_QUEUE_OVERFLOW,
                queue->data_category,
                (long)sentry_value_get_length(
                    sentry_value_get_by_key(task->items, "items")));
        } else {
            SENTRY_WARN("discarding telemetry batch: "
                        "serialization task submission failed");
        }
        batch_task_free(task);
    }
}

static bool
flush_queue(sentry_batcher_queue_t *queue, bool crash_safe)
{
    if (crash_safe) {
        // In crash-safe mode, spin lock with timeout and backoff
        int attempts = 0;
        while (!sentry__atomic_compare_swap(&queue->flushing, 0, 1)) {
            if (!crash_safe_spin_wait(++attempts, NULL)) {
                SENTRY_SIGNAL_SAFE_LOG(
                    "WARN sentry__batcher_flush: timeout waiting for "
                    "flushing lock in crash-safe mode");
                return false;
            }
        }
    } else {
        // Normal mode: try once and return if already flushing
        const long already_flushing = sentry__atomic_store(&queue->flushing, 1);
        if (already_flushing) {
            return false;
        }
    }
    // Flush through the buffer that was active when this call began. Later
    // partial buffers remain queued until the next flush.
    const long flush_idx = sentry__atomic_fetch(&queue->active_idx);
    while (true) {
        // Prep the oldest buffer first to preserve FIFO order.
        long old_buf_idx = sentry__atomic_fetch(&queue->drain_idx);
        sentry_batcher_buffer_t *old_buf = &queue->buffers[old_buf_idx];

        // Seal the active buffer if this flush owns it or it is already full.
        if (!sentry__atomic_fetch(&old_buf->sealed)) {
            const long active_idx = sentry__atomic_fetch(&queue->active_idx);
            const long count = sentry__atomic_fetch(&old_buf->index);
            if (active_idx == old_buf_idx
                && (count <= 0
                    || (old_buf_idx != flush_idx
                        && count < SENTRY_BATCHER_QUEUE_LENGTH)
                    || !rotate_buffer(queue, active_idx))) {
                break;
            }
        }

        // Wait for all in-flight producers of the old buffer
        while (sentry__atomic_fetch(&old_buf->adding) > 0) {
            sentry__cpu_relax();
        }

        long n = sentry__atomic_store(&old_buf->index, 0);
        if (n > SENTRY_BATCHER_QUEUE_LENGTH) {
            n = SENTRY_BATCHER_QUEUE_LENGTH;
        }

        if (n > 0) {
            // now we can do the actual batching of the old buffer

            sentry_value_t logs = sentry_value_new_object();
            sentry_value_t log_items = sentry_value_new_list();
            int i;
            for (i = 0; i < n; i++) {
                sentry_value_append(log_items, old_buf->items[i]);
            }
            sentry_value_set_by_key(logs, "items", log_items);

            process_batch(queue, logs, crash_safe);
        }

        // Reset the drained buffer...
        sentry__atomic_store(&old_buf->sealed, 0);
        // ...and advance to the next buffer.
        sentry__atomic_store(
            &queue->drain_idx, (old_buf_idx + 1) % SENTRY_BATCHER_BUFFER_COUNT);
    }

    sentry__atomic_store(&queue->flushing, 0);
    return true;
}

bool
sentry__batcher_flush(sentry_batcher_t *batcher, bool crash_safe)
{
    bool flushed = true;
    for (size_t i = 0; i < batcher->num_queues; i++) {
        if (!flush_queue(&batcher->queues[i], crash_safe)) {
            flushed = false;
        }
    }
    return flushed;
}

static void start_thread(sentry_batcher_t *batcher);

bool
sentry__batcher_enqueue(sentry_batcher_t *batcher,
    sentry_data_category_t data_category, sentry_value_t item)
{
    size_t index = 0;
    while (index < batcher->num_queues
        && batcher->queues[index].data_category != data_category) {
        index++;
    }
    if (index == batcher->num_queues || !batcher->queues[index].batch_func) {
        return false;
    }
    start_thread(batcher);

    sentry_batcher_queue_t *queue = &batcher->queues[index];
    while (true) {
        // retrieve the active buffer
        const long active_idx = sentry__atomic_fetch(&queue->active_idx);
        sentry_batcher_buffer_t *active = &queue->buffers[active_idx];

        // if the buffer is already sealed, retry with the new active buffer.
        if (sentry__atomic_fetch(&active->sealed) != 0) {
            continue;
        }

        // `adding` is our boundary for this buffer since it keeps the flusher
        // blocked. We have to recheck that the flusher hasn't already switched
        // the active buffer or sealed the one this thread is on. If either is
        // true we have to unblock the flusher and retry the item.
        sentry__atomic_fetch_and_add(&active->adding, 1);
        const long active_idx_check = sentry__atomic_fetch(&queue->active_idx);
        const long sealed_check = sentry__atomic_fetch(&active->sealed);
        if (active_idx != active_idx_check) {
            sentry__atomic_fetch_and_add(&active->adding, -1);
            continue;
        }
        if (sealed_check) {
            sentry__atomic_fetch_and_add(&active->adding, -1);
            continue;
        }

        // Now we can finally request a slot and check if the item fits in this
        // buffer.
        const long item_idx = sentry__atomic_fetch_and_add(&active->index, 1);
        if (item_idx < SENTRY_BATCHER_QUEUE_LENGTH) {
            // got a slot, write item while keeping the flusher blocked through
            // a possible rotation
            active->items[item_idx] = item;

            // Check if active buffer is now full and trigger flush.
            if (item_idx == SENTRY_BATCHER_QUEUE_LENGTH - 1) {
                rotate_buffer(queue, active_idx);
                sentry__atomic_store(&queue->request_flush, 1);
                sentry__waitable_flag_set(&batcher->request_flush);
            }
            sentry__atomic_fetch_and_add(&active->adding, -1);
            return true;
        }
        const bool rotated = rotate_buffer(queue, active_idx);
        // ping the batching thread to flush, since we could miss the flag set
        // on adding the last item
        sentry__atomic_store(&queue->request_flush, 1);
        sentry__waitable_flag_set(&batcher->request_flush);
        // Buffer is already full, roll back our increments and retry or drop.
        sentry__atomic_fetch_and_add(&active->adding, -1);
        if (rotated || sentry__atomic_fetch(&queue->active_idx) != active_idx) {
            continue;
        }
        sentry__client_report_discard(
            SENTRY_DISCARD_REASON_QUEUE_OVERFLOW, queue->data_category, 1);
        return false;
    }
}

SENTRY_THREAD_FN
batcher_thread_func(void *data)
{
    sentry_batcher_t *batcher = data;
    sentry__thread_setname(
        sentry__current_thread(), SENTRY_BATCHER_THREAD_NAME);
    SENTRY_DEBUGF("Starting %s thread", SENTRY_BATCHER_THREAD_NAME);

    while (sentry__atomic_fetch(&batcher->thread_state)
            == SENTRY_BATCHER_THREAD_SPAWNING
        && !sentry__atomic_fetch(&batcher->crash_flush)) {
        sentry__cpu_relax();
    }

    // Transition from STARTING to RUNNING using compare-and-swap
    // CAS ensures atomic state verification: only succeeds if state is STARTING
    // If CAS fails, shutdown already set state to STOPPED, so exit immediately
    // Uses sequential consistency to ensure all thread initialization is
    // visible
    if (sentry__atomic_fetch(&batcher->crash_flush)
        || !sentry__atomic_compare_swap(&batcher->thread_state,
            (long)SENTRY_BATCHER_THREAD_STARTING,
            (long)SENTRY_BATCHER_THREAD_RUNNING)) {
        SENTRY_DEBUG(
            "batcher thread detected shutdown during startup, exiting");
        return 0;
    }

    uint64_t next_flush
        = sentry__monotonic_time() + SENTRY_BATCHER_FLUSH_INTERVAL_MS;
    // Main loop: run while state is RUNNING
    //
    // Flush triggers:
    //  1. Buffer full → enqueue wakes us via request_flush (immediate flush)
    //  2. Timeout → partial buffer flushed after
    //  SENTRY_BATCHER_FLUSH_INTERVAL_MS
    //  3. Shutdown → thread state change or cond_wake
    while (sentry__atomic_fetch(&batcher->thread_state)
        == SENTRY_BATCHER_THREAD_RUNNING) {
        // Sleep until the next periodic flush or request_flush is set
        const uint64_t now = sentry__monotonic_time();
        const uint64_t timeout = now < next_flush ? next_flush - now : 0;
        sentry__waitable_flag_wait(&batcher->request_flush, timeout);

        if (sentry__atomic_fetch(&batcher->thread_state)
            != SENTRY_BATCHER_THREAD_RUNNING) {
            break;
        }

        const uint64_t after_wait = sentry__monotonic_time();
        const bool timed_out = after_wait >= next_flush;
        if (timed_out) {
            next_flush = after_wait + SENTRY_BATCHER_FLUSH_INTERVAL_MS;
        }

        for (size_t i = 0; i < batcher->num_queues; i++) {
            sentry_batcher_queue_t *queue = &batcher->queues[i];
            const bool requested
                = sentry__atomic_store(&queue->request_flush, 0) != 0;
            if (!timed_out && !requested) {
                continue;
            }

            // Use the buffer state as the source of truth once a flush is due:
            // flush if there's data, skip otherwise.
            const long drain_idx = sentry__atomic_fetch(&queue->drain_idx);
            const bool sealed
                = sentry__atomic_fetch(&queue->buffers[drain_idx].sealed) != 0;
            const long active_idx = sentry__atomic_fetch(&queue->active_idx);
            sentry_batcher_buffer_t *buf = &queue->buffers[active_idx];
            const long count = sentry__atomic_fetch(&buf->index);
            if (!sealed && count <= 0) {
                continue;
            }

            if (sealed || count >= SENTRY_BATCHER_QUEUE_LENGTH) {
                SENTRY_TRACE("Batcher flushed by filled buffer");
            } else {
                SENTRY_TRACE("Batcher flushed by timeout");
            }

            flush_queue(queue, false);
        }
    }

    SENTRY_DEBUG("batching thread exiting");
    return 0;
}

void
sentry__batcher_startup(
    sentry_batcher_t *batcher, const sentry_options_t *options)
{
    // dsn is incref'd because release() decref's it and may outlive options.
    batcher->dsn = sentry__dsn_incref(options->dsn);
    // transport and run are non-owning refs, safe because they
    // are only accessed in flush() which is bound by the options lifetime.
    batcher->transport = options->transport;
    batcher->run = options->run;

    // Defer thread creation until there's work; IDLE allows the first enqueue
    // to start it, while STOPPED prevents startup after shutdown.
    sentry__atomic_store(
        &batcher->thread_state, (long)SENTRY_BATCHER_THREAD_IDLE);
}

static void
start_thread(sentry_batcher_t *batcher)
{
    // Mark thread as spawning before actually spawning so thread can transition
    // to RUNNING. This prevents shutdown from thinking the thread was never
    // started if it races with the thread's initialization.
    if (!sentry__atomic_compare_swap(&batcher->thread_state,
            (long)SENTRY_BATCHER_THREAD_IDLE,
            (long)SENTRY_BATCHER_THREAD_SPAWNING)) {
        return;
    }

    int spawn_result = sentry__thread_spawn(
        &batcher->batching_thread, batcher_thread_func, batcher);

    if (spawn_result != 0) {
        SENTRY_ERROR("Failed to start batching thread");
        // Failed to spawn, reset to STOPPED
        sentry__atomic_store(
            &batcher->thread_state, (long)SENTRY_BATCHER_THREAD_STOPPED);
    } else if (!sentry__atomic_compare_swap(&batcher->thread_state,
                   (long)SENTRY_BATCHER_THREAD_SPAWNING,
                   (long)SENTRY_BATCHER_THREAD_STARTING)) {
        // Crash shutdown canceled startup; the producer still holds a reference
        sentry__thread_join(batcher->batching_thread);
    }
}

void
sentry__batcher_shutdown(sentry_batcher_t *batcher, uint64_t timeout)
{
    if (!batcher) {
        return;
    }
    (void)timeout;

    // Atomically transition to STOPPED and get the previous state
    // This handles the race where thread might be in STARTING state:
    // - If thread's CAS hasn't run yet: CAS will fail, thread exits cleanly
    // - If thread already transitioned to RUNNING: normal shutdown path
    long old_state;
    while (true) {
        old_state = sentry__atomic_fetch(&batcher->thread_state);
        if (old_state == SENTRY_BATCHER_THREAD_SPAWNING
            && !sentry__atomic_fetch(&batcher->crash_flush)) {
            sentry__cpu_relax();
            continue;
        }
        if (sentry__atomic_compare_swap(&batcher->thread_state, old_state,
                (long)SENTRY_BATCHER_THREAD_STOPPED)) {
            break;
        }
    }

    // The creator joins a canceled spawn after thread creation returns
    if (old_state <= SENTRY_BATCHER_THREAD_SPAWNING) {
        SENTRY_DEBUG("batcher thread is not joinable, skipping thread join");
    } else {
        // Thread was started (STARTING, RUNNING, or STOPPING), signal it to
        // stop
        sentry__waitable_flag_set(&batcher->request_flush);

        // Always join the thread to avoid leaks
        sentry__thread_join(batcher->batching_thread);
        sentry__atomic_store(
            &batcher->thread_state, (long)SENTRY_BATCHER_THREAD_STOPPED);
    }

    // Perform final flush to ensure any remaining items are sent
    sentry__batcher_flush(batcher, false);
    for (size_t i = 0; i < batcher->num_queues; i++) {
        batch_task_wait_all(&batcher->queues[i]);
    }
}

void
sentry__batcher_flush_crash_safe(sentry_batcher_t *batcher)
{
    // Check if batcher is initialized
    sentry__atomic_store(&batcher->crash_flush, 1);
    sentry__atomic_compare_swap(&batcher->thread_state,
        (long)SENTRY_BATCHER_THREAD_IDLE, (long)SENTRY_BATCHER_THREAD_STOPPED);
    const long state = sentry__atomic_fetch(&batcher->thread_state);
    if (state <= SENTRY_BATCHER_THREAD_SPAWNING) {
        sentry__batcher_flush(batcher, true);
        for (size_t i = 0; i < batcher->num_queues; i++) {
            batch_task_dump_pending_all(&batcher->queues[i]);
        }
        return;
    }

    // Signal the thread to stop but don't wait, since the crash-safe flush
    // will spin-lock on flushing anyway.
    sentry__atomic_compare_swap(&batcher->thread_state,
        (long)SENTRY_BATCHER_THREAD_STARTING,
        (long)SENTRY_BATCHER_THREAD_STOPPING);
    sentry__atomic_compare_swap(&batcher->thread_state,
        (long)SENTRY_BATCHER_THREAD_RUNNING,
        (long)SENTRY_BATCHER_THREAD_STOPPING);

    // Perform crash-safe flush directly to disk to avoid transport queuing
    // This is safe because we're in a crash scenario and the main thread
    // is likely dead or dying anyway
    sentry__batcher_flush(batcher, true);
    for (size_t i = 0; i < batcher->num_queues; i++) {
        batch_task_dump_pending_all(&batcher->queues[i]);
    }
}

void
sentry__batcher_force_flush(sentry_batcher_t *batcher)
{
    for (size_t i = 0; i < batcher->num_queues; i++) {
        sentry_batcher_queue_t *queue = &batcher->queues[i];
        do {
            // wait for in-progress flush to complete
            while (sentry__atomic_fetch(&queue->flushing)) {
                sentry__cpu_relax();
            }
            // retry if the batcher thread wins the race
        } while (!flush_queue(queue, false));
    }
    sentry__threadpool_flush(batcher->threadpool);
}

#ifdef SENTRY_UNITTEST
/**
 * Wait for the batching thread to be ready.
 * Returns immediately if the thread has not been requested yet.
 * This is a test-only helper to avoid race conditions in tests.
 */
void
sentry__batcher_wait_for_thread_startup(sentry_batcher_t *batcher)
{
    const int max_wait_ms = 1000;
    const int check_interval_ms = 10;
    const int max_attempts = max_wait_ms / check_interval_ms;

    for (int i = 0; i < max_attempts; i++) {
        const long state = sentry__atomic_fetch(&batcher->thread_state);
        if (state <= SENTRY_BATCHER_THREAD_IDLE) {
            return;
        }
        if (state == SENTRY_BATCHER_THREAD_RUNNING) {
            SENTRY_DEBUGF(
                "batcher thread ready after %d ms", i * check_interval_ms);
            return;
        }
        sleep_ms(check_interval_ms);
    }

    SENTRY_WARNF(
        "batcher thread failed to start within %d ms timeout", max_wait_ms);
}
#endif
