#include "scheduler.h"

/*
 * Her görev kimliği için basit bir renk paleti tanımlar.
 * 
 * Amaç: Terminal çıktısında her görevin yazı rengini farklı
 * yaparak, zaman dilimlerini ve görevler arası geçişleri
 * görsel olarak daha kolay ayırt etmek.
 *
 * Not: Renkler ANSI escape kodları ile verilmiştir,
 * WSL/Ubuntu terminalinde çalışır.
 */
const char *get_task_color(int task_id)
{
    static const char *colors[] = {
        "\033[31m", // kırmızı
        "\033[32m", // yeşil
        "\033[33m", // sarı
        "\033[34m", // mavi
        "\033[35m", // mor
        "\033[36m"  // camgöbeği
    };

    int n = (int)(sizeof(colors) / sizeof(colors[0]));

    /* Görev kimliği renk sayısından büyük olabilir.
       Bu yüzden basit bir mod işlemi ile renk atıyoruz. */
    return colors[(task_id - 1) % n];
}

/* Rengi eski haline döndürmek için kullanılan kod */
static const char *color_reset(void)
{
    return "\033[0m";
}

/*
 * Görev ile ilgili bir olayı ekrana yazar.
 * Örnek olaylar:
 *  - "BAŞLADI"
 *  - "BİTTİ"
 *  - "ASKIDA" vb.
 */
void print_task_event(const SimTask *task, const char *event, int current_time)
{
    printf("%s[Zaman %2d] Görev %d (öncelik=%d, kalan=%d sn): %s%s\n",
           get_task_color(task->id),
           current_time,
           task->id,
           task->priority,
           task->remaining_time,
           event,
           color_reset());
}

/*
 * Görev her zaman diliminde (tick) çalıştığında çağrılır.
 * Görevin o anda çalıştığını ve kalan süresini gösterir.
 */
void print_task_tick(const SimTask *task, int current_time)
{
    printf("%s[Zaman %2d] Görev %d çalışıyor... (kalan=%d sn)%s\n",
           get_task_color(task->id),
           current_time,
           task->id,
           task->remaining_time,
           color_reset());
}
