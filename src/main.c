#include "scheduler.h"

/*
 * Programın giriş noktası.
 *
 * Komut satırı kullanımı:
 *   ./freertos_sim giris.txt
 *
 * 1) giris.txt dosyasından görev listesini okur.
 * 2) Görevleri diziye yükler.
 * 3) run_scheduler() fonksiyonu ile zamanlayıcı simülasyonunu başlatır.
 */
int main(int argc, char *argv[])
{
    /* Komut satırı argüman kontrolü */
    if (argc != 2) {
        fprintf(stderr, "Kullanım: %s giris.txt\n", argv[0]);
        return 1;
    }

    const char *input_file = argv[1];

    /* Görevleri tutacağımız dizi */
    SimTask tasks[MAX_TASKS];

    /* giris.txt dosyasını okuyup görevleri yükle */
    int task_count = load_task_list(input_file, tasks, MAX_TASKS);

    if (task_count < 0) {
        fprintf(stderr, "Görev listesi yüklenemedi.\n");
        return 1;
    }

    printf("Toplam %d görev yüklendi. Zamanlayıcı başlatılıyor...\n",
           task_count);

    /* Asıl zamanlayıcı fonksiyonunu çağır */
    run_scheduler(tasks, task_count);

    return 0;
}
