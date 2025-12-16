#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/*
 * Görev öncelikleri:
 *  0 : Gerçek zamanlı (Real-Time) görev
 *  1 : Yüksek öncelikli kullanıcı görevi
 *  2 : Orta öncelikli kullanıcı görevi
 *  3 : Düşük öncelikli kullanıcı görevi
 *
 * Scheduler; 0 öncelikli görevleri ayrı bir FCFS kuyruğunda,
 * 1–3 öncelikli görevleri ise üç seviyeli geri beslemeli
 * (multi-level feedback queue) yapısında çalıştırır.
 */

/* Görev durumu (basit yaşam döngüsü) */
typedef enum {
    TASK_STATE_WAITING,   // Varış zamanı henüz gelmedi, bekliyor
    TASK_STATE_READY,     // Kuyruğa alınmış, çalışmayı bekliyor
    TASK_STATE_RUNNING,   // O anda CPU'da çalışıyor
    TASK_STATE_FINISHED   // Görevini bitirmiş durumda
} TaskState;

/* Simülasyonda kullanılacak görev yapısı */
typedef struct {
    int   id;             // Görev kimliği (1, 2, 3, ...)
    int   arrival_time;   // Varış zamanı (saniye cinsinden)
    int   priority;       // Öncelik (0..3)
    int   burst_time;     // Toplam çalışma süresi (saniye)
    int   remaining_time; // Kalan çalışma süresi (saniye)

    /* PDF'e göre her görev, scheduler tarafından sonlandırılmazsa
     * en fazla 20 saniye CPU kullanabilir. Bu alan, o ana kadar
     * toplam kaç saniye çalıştığını takip etmek için kullanılır.
     */
    int   cpu_time_used;  // Toplam CPU kullanım süresi (saniye)

    TaskState state;      // Görevin mevcut durumu
} SimTask;

/* Maksimum görev sayısı (giris.txt için üst sınır) */
#define MAX_TASKS 128

/* 
 * giris.txt dosyasını okuyup görevleri diziye yükler.
 * 
 * Parametreler:
 *   filename  : Okunacak dosyanın adı (örn. "giris.txt")
 *   tasks     : Görevlerin yazılacağı dizi
 *   max_tasks : Dizi boyutu (üst sınır)
 *
 * Dönüş:
 *   >= 0 : Okunan görev sayısı
 *   -1   : Hata (dosya açılamadı vb.)
 */
int load_task_list(const char *filename, SimTask tasks[], int max_tasks);

/*
 * Zamanlayıcı simülasyonunu başlatır.
 * 
 * - Gerçek zamanlı (priority = 0) görevler FCFS ile ve kesintisiz yürütülür.
 * - Kullanıcı görevleri (priority = 1,2,3) üç seviyeli geri beslemeli kuyruğa
 *   göre 1 saniyelik zaman dilimleri ile çalıştırılır.
 * - Her görev en fazla 20 saniye CPU kullanabilir, aksi takdirde
 *   otomatik olarak sonlandırılır.
 */
void run_scheduler(SimTask tasks[], int task_count);

/*
 * Renkli çıktı ve olay mesajları (tasks.c içinde tanımlanır)
 */
const char *get_task_color(int task_id);
void print_task_event(const SimTask *task, const char *event, int current_time);
void print_task_tick(const SimTask *task, int current_time);

#endif // SCHEDULER_H
