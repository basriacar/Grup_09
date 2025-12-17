#include "scheduler.h"
#include <string.h>
#include <unistd.h> // sleep() için

/*
 * ---------------------------------------------------------------------------
 * GRUP 09 - FREE RTOS SIMULASYONU
 * ---------------------------------------------------------------------------
 * Dosya: scheduler.c
 * Aciklama:
 * Bu modul, sistemdeki process'lerin (gorevlerin) hangi sirayla
 * islemciyi (CPU) kullanacagini belirleyen zamanlayici algoritmasini icerir.
 *
 * Algoritma Detayi:
 * 1. Gercek Zamanli Gorevler (Priority 0):
 * - FCFS (First-Come-First-Served) mantigi ile kesintisiz calisir.
 * - Gelen gorev hemen islemciye alinir ve bitene kadar (veya limiti dolana kadar) calisir.
 *
 * 2. Kullanici Gorevleri (Priority 1-3):
 * - MLFQ (Multi-Level Feedback Queue) kullanilir.
 * - 3 farkli oncelik kuyrugu vardir (user_q[0], user_q[1], user_q[2]).
 * - Her gorev 1 sn (tick) calisir, bitmezse onceligi dusurulur ve kuyruk degistirir.
 * - Round-Robin mantigi en dusuk seviyede devreye girer.
 * ---------------------------------------------------------------------------
 */

/* --- AYARLAR (CONSTANTS) --- */
/* Kodun icindeki sihirli sayilari (magic numbers) buraya tasidik */
#define MAX_CPU_LIMIT  20  // Maksimum CPU kullanim suresi (saniye)
#define SIM_TICK_DELAY 1   // Her dongudeki bekleme suresi (saniye)


/* -------------------------------------------------------------------
 * GİRİŞ DOSYASI OKUMA (giris.txt)
 * -------------------------------------------------------------------
 */
int load_task_list(const char *filename, SimTask tasks[], int max_tasks)
{
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("giris.txt açılamadı");
        return -1;
    }

    int arrival, priority, burst;
    int count = 0;

    while (fscanf(f, " %d , %d , %d", &arrival, &priority, &burst) == 3) {
        if (count >= max_tasks) {
            fprintf(stderr, "Maksimum görev sayısı aşıldı (%d)\n", max_tasks);
            break;
        }

        SimTask *t = &tasks[count];
        t->id             = count + 1;
        t->arrival_time   = arrival;
        t->priority       = priority;
        t->burst_time     = burst;
        t->remaining_time = burst;
        t->cpu_time_used  = 0;
        t->state          = TASK_STATE_WAITING;

        count++;
    }

    fclose(f);
    return count;
}

/* -------------------------------------------------------------------
 * KUYRUK YAPISI
 * -------------------------------------------------------------------
 */
typedef struct {
    int items[MAX_TASKS];
    int head;
    int tail;
    int size;
} IntQueue;

static void queue_init(IntQueue *q)
{
    q->head = q->tail = q->size = 0;
}

static bool queue_is_empty(const IntQueue *q)
{
    return q->size == 0;
}

static bool queue_enqueue(IntQueue *q, int value)
{
    if (q->size >= MAX_TASKS) return false;
    q->items[q->tail] = value;
    q->tail = (q->tail + 1) % MAX_TASKS;
    q->size++;
    return true;
}

static bool queue_dequeue(IntQueue *q, int *value)
{
    if (q->size == 0) return false;
    *value = q->items[q->head];
    q->head = (q->head + 1) % MAX_TASKS;
    q->size--;
    return true;
}

/* -------------------------------------------------------------------
 * YARDIMCI FONKSİYONLAR
 * -------------------------------------------------------------------
 */

/* Görev belirlenen limiti aştı mı? */
static bool task_reached_cpu_limit(const SimTask *t)
{
    /* Elle yazilan 20 yerine sabiti kullaniyoruz */
    return t->cpu_time_used >= MAX_CPU_LIMIT;
}

static const char *get_user_task_start_event(const SimTask *t)
{
    if (t->remaining_time == t->burst_time) {
        return "Kullanıcı görevi BAŞLADI";
    } else {
        return "Kullanıcı görevi DEVAM EDİYOR";
    }
}

/* -------------------------------------------------------------------
 * ZAMANLAYICI (SCHEDULER)
 * -------------------------------------------------------------------
 */
void run_scheduler(SimTask tasks[], int task_count)
{
    if (task_count <= 0) {
        printf("Çalıştırılacak görev yok.\n");
        return;
    }

    IntQueue rt_queue;
    IntQueue user_q[3];

    queue_init(&rt_queue);
    for (int i = 0; i < 3; i++) {
        queue_init(&user_q[i]);
    }

    int completed_tasks   = 0;
    int current_time      = 0;
    int rt_running_index  = -1;
    bool all_done = false;

    while (!all_done) {

        /* 1) Gelen görevleri kuyruğa al */
        for (int i = 0; i < task_count; i++) {
            SimTask *t = &tasks[i];

            if (t->state == TASK_STATE_WAITING && t->arrival_time <= current_time) {
                t->state = TASK_STATE_READY;
                if (t->priority == 0) {
                    queue_enqueue(&rt_queue, i);
                } else if (t->priority >= 1 && t->priority <= 3) {
                    queue_enqueue(&user_q[t->priority - 1], i);
                } else {
                    fprintf(stderr, "Geçersiz öncelik: %d\n", t->priority);
                }
            }
        }

        /* 2) RT görev kontrolü */
        if (rt_running_index == -1 && !queue_is_empty(&rt_queue)) {
            if (queue_dequeue(&rt_queue, &rt_running_index)) {
                SimTask *t = &tasks[rt_running_index];
                if (t->state != TASK_STATE_FINISHED) {
                    t->state = TASK_STATE_RUNNING;
                    print_task_event(t, "GERÇEK ZAMANLI görev BAŞLADI", current_time);
                }
            }
        }

        /* 3) Simülasyon adımı */
        if (rt_running_index != -1) {
            /* --- GERÇEK ZAMANLI GÖREV --- */
            SimTask *t = &tasks[rt_running_index];
            if (t->state != TASK_STATE_FINISHED) {
                t->remaining_time--;
                t->cpu_time_used++;
                print_task_tick(t, current_time);
                
                /* Sabit sure kadar bekle */
                sleep(SIM_TICK_DELAY); 

                if (t->remaining_time <= 0 || task_reached_cpu_limit(t)) {
                    t->state = TASK_STATE_FINISHED;
                    if (t->remaining_time < 0) t->remaining_time = 0;

                    if (task_reached_cpu_limit(t) && t->remaining_time > 0) {
                        /* 20 sn limiti doldu */
                        print_task_event(t, "GERÇEK ZAMANLI görev CPU limitine ulaştı, sonlandırıldı", current_time + 1);
                    } else {
                        print_task_event(t, "GERÇEK ZAMANLI görev BİTTİ", current_time + 1);
                    }
                    completed_tasks++;
                    rt_running_index = -1;
                }
            }
        } else {
            /* --- KULLANICI GÖREVLERİ (MLFQ) --- */
            int idx = -1;
            for (int level = 0; level < 3; level++) {
                if (!queue_is_empty(&user_q[level])) {
                    queue_dequeue(&user_q[level], &idx);
                    break;
                }
            }

            if (idx != -1) {
                SimTask *t = &tasks[idx];
                if (t->state != TASK_STATE_FINISHED) {
                    t->state = TASK_STATE_RUNNING;
                    const char *event = get_user_task_start_event(t);
                    print_task_event(t, event, current_time);

                    t->remaining_time--;
                    t->cpu_time_used++;
                    print_task_tick(t, current_time);
                    
                    /* Sabit sure kadar bekle */
                    sleep(SIM_TICK_DELAY);

                    if (t->remaining_time <= 0 || task_reached_cpu_limit(t)) {
                        t->state = TASK_STATE_FINISHED;
                        if (t->remaining_time < 0) t->remaining_time = 0;

                        if (task_reached_cpu_limit(t) && t->remaining_time > 0) {
                            print_task_event(t, "Kullanıcı görevi CPU limitine ulaştı, sonlandırıldı", current_time + 1);
                        } else {
                            print_task_event(t, "Kullanıcı görevi BİTTİ", current_time + 1);
                        }
                        completed_tasks++;
                    } else {
                        if (t->priority < 3) t->priority++;
                        t->state = TASK_STATE_READY;
                        print_task_event(t, "Görev ASKIDA, kuyruk seviyesi düşürüldü", current_time + 1);
                        
                        int new_level = t->priority - 1;
                        queue_enqueue(&user_q[new_level], idx);
                    }
                }
            } else {
                printf("[Zaman %2d] Sistemde çalışacak görev yok, CPU boşta.\n", current_time);
                sleep(SIM_TICK_DELAY);
            }
        }

        if (completed_tasks >= task_count) all_done = true;
        current_time++;
    }

    printf("Tüm görevler tamamlandı. Toplam görev sayısı: %d\n", task_count);
}