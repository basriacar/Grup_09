#include <stdio.h>
#include <stdlib.h>
#include "scheduler.h" // Eğer scheduler.h aynı klasördeyse "scheduler.h", üstteyse "../src/scheduler.h"

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
