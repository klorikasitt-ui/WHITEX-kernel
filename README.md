
## 📋 Proje Hakkında
WhiteX, bir işletim sisteminin en temel bileşenlerini (bootloader, kernel, temel sürücüler) sıfırdan inşa etmeyi hedefler. Mevcut sürüm (v0.0.5), fonksiyonel bir VGA metin modu arayüzü ve temel bir kabuk (shell) sunmaktadır.
## 🛠️ Teknik Özellikler
 * **Kernel Mimarisi:** Monolitik yapıya sahip minimalist çekirdek tasarımı.
 * **Derleyici & Bağlayıcı:** Geliştirme sürecinde **Clang 21.1.8** ve **LLD 21.1.8** kullanılmaktadır.
 * **Grafik Arayüzü:** 80x25 VGA metin modu desteği (vga_entry, terminal_row).
 * **Giriş Birimleri:** Kesme tabanlı (interrupt-driven) klavye sürücüsü ve US-Shift düzeni desteği.
## 💻 Mevcut Komutlar
Sistem önyüklendikten sonra whitex$ istemi üzerinden aşağıdaki komutları kabul eder:
| Komut | Açıklama |
|---|---|
| help | Sistemdeki tüm kullanılabilir komutları listeler. |
## 🏗️ Dosya Yapısı ve Semboller
 * boot.asm: Sistemin başlangıç noktası ve 32-bit korumalı moda geçiş hazırlığı.
 * Kernel.c: Çekirdek mantığı, terminal yönetimi ve komut ayrıştırıcı.
 * _start: Kernel'ın giriş noktası (entry point).
 * keyboard_handler: Donanım seviyesinde klavye vuruşlarını işleyen fonksiyon.
## 🚀 Kurulum ve Çalıştırma
Sistemi derlemek için Clang ve LLD araç zinciri gereklidir. .bin dosyası, uygun bir emülatör (QEMU vb.) veya gerçek donanım üzerinde boot edilebilir.
**Geliştirici:** Burak Yakub Güçer
**Sürüm:** 0.0.5

