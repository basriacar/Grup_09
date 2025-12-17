#include "scheduler.h"
#include <string.h>
#include <unistd.h> // sleep() için

/*
 * ---------------------------------------------------------------------------
 * GRUP 09 - FREE RTOS SIMULASYONU
 * ---------------------------------------------------------------------------
 * Dosya: scheduler.c
 * * Aciklama:
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

/* -------------------------------------------------------------------
 * GİRİŞ DOSYASI OKUMA (giris.txt)
 * -------------------------------------------------------------------
 *
 * giris.txt formatı:
 * <varış_zamanı>, <öncelik>, <görev_süresi>
 *
 * Örnek:
 * 0, 0, 3
 * 1, 1, 5
 * 3, 3, 2
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

    /* Dosyadaki her satırı okuyup diziye yerleştiriyoruz */
    while (fscanf(f, " %d , %d , %d", &arrival, &priority, &burst) == 3) {
        if (count >= max_tasks) {
            fprintf(stderr, "Maksimum görev sayısı aşıldı (%d)\n", max_tasks);
            break;
        }

        SimTask *t = &tasks[count];
        t->id             = count + 1;     // Görev kimliği (1'den başlat)
        t->arrival_time   = arrival;       // Varış zamanı
        t->priority       = priority;      // Başlangıç önceliği (0..3)
        t->burst_time     = burst;         // Toplam ihtiyaç duyduğu süre
        t->remaining_time = burst;         // Başlangıçta kalan süre = burst
        t->cpu_time_used  = 0;             // Toplam CPU kullanım süresi = 0
        t->state          = TASK_STATE_WAITING; // Daha başlamadı, bekliyor

        count++;
    }

    fclose(f);
    return count;
}

/* -------------------------------------------------------------------
 * KUYRUK YAPISI
 * -------------------------------------------------------------------
 *
 * Kuyruklar sadece görev dizisindeki indeksleri tutar.
 * Görev bilgileri SimTask dizisinde duruyor, kuyrukta sadece
 * hangi görevin sırada olduğunu bilmemiz yeterli.
 */

typedef struct {
    int items[MAX_TASKS];
    int head;  // Kuyruğun başı
    int tail;  // Kuyruğun sonu
    int size;  // Kuyruktaki eleman sayısı
} IntQueue;

/* Kuyruğu başlangıç durumuna getir */
static void queue_init(IntQueue *q)
{
    q->head = q->tail = q->size = 0;
}

/* Kuyruk boş mu? */
static bool queue_is_empty(const IntQueue *q)
{
    return q->size == 0;
}

/* Kuyruğa eleman ekle (enqueue) */
static bool queue_enqueue(IntQueue *q, int value)
{
    if (q->size >= MAX_TASKS) {
        return false; // Kuyruk dolu
    }
    q->items[q->tail] = value;
    q->tail = (q->tail + 1) % MAX_TASKS;
    q->size++;
    return true;
}

/* Kuyruktan eleman çıkar (dequeue) */
static bool queue_dequeue(IntQueue *q, int *value)
{
    if (q->size == 0) {
        return false; // Kuyruk boş
    }
    *value = q->items[q->head];
    q->head = (q->head + 1) % MAX_TASKS;
    q->size--;
    return true;
}

/* -------------------------------------------------------------------
 * YARDIMCI FONKSİYONLAR
 * -------------------------------------------------------------------
 */

/* Görev 20 sn CPU kullandı mı? (PDF'e göre max 20 sn sonra kendiliğinden biter) */
static bool task_reached_cpu_limit(const SimTask *t)
{
    return t->cpu_time_used >= 20;
}

/* Görev başlangıç mı, devam mı? (mesaj için) */
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
 *
 * Genel mantık:
 * - Öncelik 0 görevler: RT kuyruğunda (rt_queue), FCFS, kesintisiz.
 * - Öncelik 1,2,3 görevler: 3 seviyeli geri beslemeli kuyruğa
 * (user_q[0], user_q[1], user_q[2]) atanır.
 *
 * - Zaman birimi (tick) = 1 saniye.
 * - RT kuyruğunda bekleyen görev varken, kullanıcı görevleri çalışmaz.
 * - Kullanıcı görevleri 1 saniye çalışır:
 * * Biterse: FINISHED
 * * Bitmezse: askıya alınır, önceliği bir seviye düşer (max 3).
 * - En düşük seviye kuyruğunda (priority = 3) round-robin davranışı oluşur.
 * - Her görev en fazla 20 sn CPU kullanabilir, sonra kendiliğinden biter.
 */

void run_scheduler(SimTask tasks[], int task_count)
{
    if (task_count <= 0) {
        printf("Çalıştırılacak görev yok.\n");
        return;
    }

    /* rt_queue   : öncelik 0 (gerçek zamanlı) */
    IntQueue rt_queue;
    /* user_q[0] : öncelik 1 (yüksek)
       user_q[1] : öncelik 2 (orta)
       user_q[2] : öncelik 3 (düşük) */
    IntQueue user_q[3];

    queue_init(&rt_queue);
    for (int i = 0; i < 3; i++) {
        queue_init(&user_q[i]);
    }

    int completed_tasks   = 0;  // Biten görev sayısı
    int current_time      = 0;  // Simülasyon zamanı (saniye)
    int rt_running_index  = -1; // O anda çalışan RT görevin indeksini tutar

    bool all_done = false;

    /* Tüm görevler bitene kadar simülasyonu döndür */
    while (!all_done) {

        /* -----------------------------------------------------------
         * 1) Bu zaman adımında "gelen" görevleri uygun kuyruğa aktar
         * -----------------------------------------------------------
         */
        for (int i = 0; i < task_count; i++) {
            SimTask *t = &tasks[i];

            /* WAITING durumundaki ve varış zamanı gelmiş görevleri READY yap */
            if (t->state == TASK_STATE_WAITING &&
                t->arrival_time <= current_time) {

                t->state = TASK_STATE_READY;

                if (t->priority == 0) {
                    /* Gerçek zamanlı görevler RT kuyruğuna */
                    queue_enqueue(&rt_queue, i);
                } else if (t->priority >= 1 && t->priority <= 3) {
                    /* Kullanıcı görevleri ilgili öncelik kuyruğuna */
                    queue_enqueue(&user_q[t->priority - 1], i);
                } else {
                    fprintf(stderr, "Geçersiz öncelik: %d\n", t->priority);
                }
            }
        }

        /* -----------------------------------------------------------
         * 2) Eğer çalışan bir RT görev yoksa ve RT kuyruğu boş değilse
         * sıradaki RT görevi başlat.
         * -----------------------------------------------------------
         */
        if (rt_running_index == -1 && !queue_is_empty(&rt_queue)) {
            if (queue_dequeue(&rt_queue, &rt_running_index)) {
                SimTask *t = &tasks[rt_running_index];
                if (t->state != TASK_STATE_FINISHED) {
                    t->state = TASK_STATE_RUNNING;
                    print_task_event(t,
                                     "GERÇEK ZAMANLI görev BAŞLADI",
                                     current_time);
                }
            }
        }

        /* -----------------------------------------------------------
         * 3) 1 saniyelik zaman dilimini simüle et
         * -----------------------------------------------------------
         */
        if (rt_running_index != -1) {
            /* --------- GERÇEK ZAMANLI GÖREV ÇALIŞIYOR --------- */
            SimTask *t = &tasks[rt_running_index];

            if (t->state != TASK_STATE_FINISHED) {
                /* 1 sn çalıştır */
                t->remaining_time--;
                t->cpu_time_used++;  // Toplam CPU süresini takip et
                print_task_tick(t, current_time);
                sleep(1);

                if (t->remaining_time <= 0 || task_reached_cpu_limit(t)) {
                    /* Görev süresini tamamladı veya 20 sn CPU limitine ulaştı */
                    t->state = TASK_STATE_FINISHED;
                    if (t->remaining_time < 0) {
                        t->remaining_time = 0;
                    }

                    if (task_reached_cpu_limit(t) && t->remaining_time > 0) {
                        print_task_event(
                            t,
                            "GERÇEK ZAMANLI görev 20 sn CPU limitine ulaştı, sonlandırıldı",
                            current_time + 1
                        );
                    } else {
                        print_task_event(
                            t,
                            "GERÇEK ZAMANLI görev BİTTİ",
                            current_time + 1
                        );
                    }

                    completed_tasks++;
                    rt_running_index = -1; // Artık RT görev yok
                }
            }
        } else {
            /* --------- KULLANICI GÖREVLERİ (MLFQ) --------- */
            int idx = -1;

            /* En yüksek öncelikli dolu kuyruğu bul (1 -> 2 -> 3) */
            for (int level = 0; level < 3; level++) {
                if (!queue_is_empty(&user_q[level])) {
                    queue_dequeue(&user_q[level], &idx);
                    break;
                }
            }

            if (idx != -1) {
                /* Çalıştırılacak bir kullanıcı görevi bulundu */
                SimTask *t = &tasks[idx];

                if (t->state != TASK_STATE_FINISHED) {
                    t->state = TASK_STATE_RUNNING;

                    /* İlk kez mi başlıyor, yoksa devam mı ediyor? */
                    const char *event = get_user_task_start_event(t);
                    print_task_event(t, event, current_time);

                    /* 1 saniyelik zaman dilimi */
                    t->remaining_time--;
                    t->cpu_time_used++;
                    print_task_tick(t, current_time);
                    sleep(1);

                    if (t->remaining_time <= 0 || task_reached_cpu_limit(t)) {
                        /* Görev tamamlandı veya 20 sn CPU limitine ulaştı */
                        t->state = TASK_STATE_FINISHED;
                        if (t->remaining_time < 0) {
                            t->remaining_time = 0;
                        }

                        if (task_reached_cpu_limit(t) && t->remaining_time > 0) {
                            print_task_event(
                                t,
                                "Kullanıcı görevi 20 sn CPU limitine ulaştı, sonlandırıldı",
                                current_time + 1
                            );
                        } else {
                            print_task_event(
                                t,
                                "Kullanıcı görevi BİTTİ",
                                current_time + 1
                            );
                        }

                        completed_tasks++;
                    } else {
                        /* Görev bitmedi -> askıya al ve önceliğini düşür */
                        if (t->priority < 3) {
                            t->priority++;  // Bir alt öncelik seviyesine düş
                        }
                        t->state = TASK_STATE_READY;
                        print_task_event(
                            t,
                            "Görev ASKIDA, kuyruk seviyesi düşürüldü",
                            current_time + 1
                        );

                        /* Güncel önceliğine uygun kuyruğa tekrar ekle */
                        int new_level = t->priority - 1;
                        queue_enqueue(&user_q[new_level], idx);
                    }
                }
            } else {
                /* Bu zaman anında çalışacak hiçbir görev yok */
                printf("[Zaman %2d] Sistemde çalışacak görev yok, CPU boşta.\n",
                       current_time);
                sleep(1);
            }
        }

        /* -----------------------------------------------------------
         * 4) Döngü sonunda tüm görevler bitmiş mi kontrol et
         * -----------------------------------------------------------
         */
        if (completed_tasks >= task_count) {
            all_done = true;
        }

        /* Zamanı bir saniye ilerlet */
        current_time++;
    }

    printf("Tüm görevler tamamlandı. Toplam görev sayısı: %d\n", task_count);
}