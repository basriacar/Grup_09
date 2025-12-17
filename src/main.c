#include <stdio.h>
#include <stdlib.h>
#include "scheduler.h" // Eğer scheduler.h aynı klasördeyse "scheduler.h", üstteyse "../src/scheduler.h"

/* --- EKSİK OLAN FONKSİYONLAR BURADA TANIMLANIYOR --- */

// scheduler.c'nin olayları ekrana yazdırmak için çağırdığı fonksiyon
void print_task_event(const SimTask *t, const char *event, int time) {
    printf("[Zaman: %3d] [Görev %d] (Öncelik: %d) -> %s\n", 
           time, t->id, t->priority, event);
}

// Her saniye (tick) çağrılan fonksiyon (İsteğe bağlı detay için)
void print_task_tick(const SimTask *t, int time) {
    // Ekranı çok doldurmamak için şimdilik boş bırakabiliriz 
    // veya debug için şunu açabilirsiniz:
    // printf("    Running: Task %d, Rem: %d\n", t->id, t->remaining_time);
}

/* --- ANA PROGRAM --- */
int main() {
    SimTask tasks[100]; // Maksimum görev sayısı
    
    printf("--- GRUP 09 SCHEDULER SIMULATORU ---\n");

    // giris.txt dosyasından görevleri oku
    // Not: main.c src klasöründeyse ve giris.txt ana dizindeyse "../giris.txt" yapmanız gerekebilir.
    // Şimdilik "giris.txt" diyelim.
    int count = load_task_list("giris.txt", tasks, 100);
    
    if (count <= 0) {
        printf("Hata: Görevler okunamadı. 'giris.txt' dosyasının doğru yerde olduğundan emin olun.\n");
        return 1;
    }

    printf("%d adet görev yüklendi. Simülasyon başlıyor...\n\n", count);

    // Zamanlayıcıyı başlat
    run_scheduler(tasks, count);

    return 0;
}