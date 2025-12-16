# FreeRTOS PC Scheduler Simülasyonu

##  Proje Açıklaması
Bu proje, İşletim Sistemleri dersi kapsamında FreeRTOS mantığı ile uyumlu,
4 seviyeli öncelikli görev zamanlayıcı (scheduler) simülasyonu geliştirmeyi amaçlamaktadır.

Simülasyon, FreeRTOS'taki görevlendirme yaklaşımını modelleyerek:

- Gerçek zamanlı (RT) görev yürütme
- Çok seviyeli geri beslemeli kuyruk (MLFQ)
- Zaman dilimi (quantum) mantığı
- Görev önceliği ve kesme (preemption)
- Kuyruk yapıları

gibi temel kavramları göstermektedir.

## Proje Dosya Yapısı
```
FreeRTOS_PC_Scheduler/
├── src/
│   ├── main.c
│   ├── scheduler.c
│   ├── scheduler.h
│   └── tasks.c
├── giris.txt
├── Makefile
├── README.md
```

##  Geliştirme Ortamı
Proje **Ubuntu WSL üzerinde GCC ve Make kullanılarak** derlenir.

Gerekli paketleri yüklemek için:
```bash
sudo apt update
sudo apt install -y build-essential gcc make
```

##  Derleme
```bash
cd FreeRTOS_PC_Scheduler
make
```

##  Çalıştırma
```bash
./freertos_sim giris.txt
```

veya:

```bash
make run
```

##  giris.txt Formatı
Her satır şu şekildedir:

```
<varis_zamani>, <oncelik>, <gorev_suresi>
```

Öncelik değerleri:
- `0` → Gerçek zamanlı görev
- `1` → Yüksek öncelikli kullanıcı
- `2` → Orta öncelikli kullanıcı
- `3` → Düşük öncelikli kullanıcı

Örnek:
```
0, 0, 3
1, 1, 5
3, 3, 2
```

##  Scheduler Mantığı
### 1️ Gerçek Zamanlı Görevler (priority = 0)
- Her zaman en yüksek önceliğe sahiptir.
- FCFS (İlk Gelen İlk Çalışır) algoritması uygulanır.
- Başladıktan sonra **kesilmez**, bitene kadar çalışır.
- Kullanıcı görevlerini preempt eder.

### 2️ Kullanıcı Görevleri (priority = 1, 2, 3)
- Üç seviyeli geri beslemeli kuyruğa (MLFQ) yerleştirilir.
- Quantum = **1 saniye**
- Bitmezse:
  - Bir alt öncelik seviyesine düşürülür.
  - Tekrar kuyruğa eklenir.
- En alt seviye (3) round-robin mantığıyla çalışır.

### 3️ Görev Çıktıları
Program her görev için renkli olarak:
- BAŞLADI  
- ÇALIŞIYOR  
- ASKIDA  
- BİTTİ  

mesajlarını üretir.

##  Çıktı Kaydetme
Rapor için simülasyon çıktısını kaydetmek için:

```bash
./freertos_sim giris.txt | tee cikti.txt
```

##  ZIP İçeriği
SABIS’e yüklenecek proje dosyası içinde şunlar bulunmalıdır:

- `src/` altındaki tüm `.c` ve `.h` dosyaları  
- `giris.txt`  
- `cikti.txt`  
- `README.md`  
- **PDF formatında proje raporu**

##  Grup Üyeleri
(
 B221210104 Hasan Basri AÇAR
 B221210105 Numan AK
 B201210003 Emir Çağlar DEMİRCİ
 B221210048 Mahmut KAHRAMAN 
 )
