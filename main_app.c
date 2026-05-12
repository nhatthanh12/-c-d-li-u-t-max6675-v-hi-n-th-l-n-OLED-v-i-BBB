#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include "oled_font.h"

#define TEMP_THRESHOLD 40.0  // Ngưỡng nhiệt độ bật LED cảnh báo

// Biến toàn cục
int is_running = 0;
pthread_mutex_t lock; 

// --- CÁC HÀM HỖ TRỢ HIỂN THỊ OLED ---
void oled_clear(int fd) {
    if (fd < 0) return;
    unsigned char buf[1024] = {0};
    write(fd, buf, 1024);
}

void oled_print_str(int fd, const char *str) {
    if (fd < 0) return;
    unsigned char buf[6];
    while (*str) {
        if (*str >= ' ' && *str <= 'z') {
            memcpy(buf, font5x8[(int)*str], 5);
        } else {
            memset(buf, 0, 5);
        }
        buf[5] = 0x00; 
        write(fd, buf, 6);
        str++;
    }
}

// --- THREAD 1: XỬ LÝ NÚT BẤM (CÓ TỰ PHỤC HỒI) ---
void* button_thread_func(void* arg) {
    int fd_gpio = -1;
    char btn_state, last_state = '1';

    while (1) {
        // 1. Cơ chế Tự phục hồi: Nếu mất kết nối, tự động mở lại
        if (fd_gpio < 0) {
            fd_gpio = open("/dev/nhatthanh_gpio", O_RDWR);
            if (fd_gpio < 0) {
                printf("Loi GPIO! Dang thu khoi dong lai sau 2s...\n");
                sleep(2);
                continue; // Quay lại vòng lặp để thử mở lại
            } else {
                printf("NHATTHANH: GPIO Driver da duoc khoi phuc!\n");
            }
        }

        // 2. Đọc nút bấm
        lseek(fd_gpio, 0, SEEK_SET);
        if (read(fd_gpio, &btn_state, 1) <= 0) {
            // Đọc lỗi -> Đóng file để vòng lặp sau tự động mở lại
            close(fd_gpio);
            fd_gpio = -1; 
            continue;
        }

        // 3. Xử lý logic bấm
        if (btn_state == '0' && last_state == '1') { 
            pthread_mutex_lock(&lock);
            is_running = !is_running;
            int current_status = is_running;
            pthread_mutex_unlock(&lock);
            
            printf("NHATTHANH: Bam nut! Trang thai do: %s\n", current_status ? "RUNNING" : "STOPPED");
            
            if (!current_status) write(fd_gpio, "0", 1); // Tắt LED khi dừng

            usleep(200000); // Debounce
        }
        last_state = btn_state;
        usleep(50000); 
    }
    return NULL;
}

// --- THREAD 2: CẢM BIẾN, OLED, LED & IOT (CÓ TỰ PHỤC HỒI) ---
void* sensor_thread_func(void* arg) {
    int fd_max = -1, fd_oled = -1, fd_gpio = -1;
    unsigned char buf[2];
    float temp;
    char display_buf[32];
    char curl_cmd[256];

    while (1) {
        // 1. Cơ chế Tự phục hồi Driver
        if (fd_max < 0)  fd_max  = open("/dev/nhatthanh_max6675", O_RDONLY);
        if (fd_oled < 0) fd_oled = open("/dev/nhatthanh_oled", O_WRONLY);
        if (fd_gpio < 0) fd_gpio = open("/dev/nhatthanh_gpio", O_RDWR);

        if (fd_max < 0 || fd_oled < 0 || fd_gpio < 0) {
            printf("Loi SPI/I2C! Dang thu khoi dong lai cac Driver...\n");
            sleep(2);
            continue;
        }

        // 2. Kiểm tra trạng thái chạy
        pthread_mutex_lock(&lock);
        int run_status = is_running;
        pthread_mutex_unlock(&lock);

        if (run_status) {
            int read_bytes = read(fd_max, buf, 2);
            if (read_bytes < 0) {
                // Mất kết nối cảm biến đột ngột -> Đóng để tự phục hồi
                close(fd_max); fd_max = -1;
                continue;
            }

            if (read_bytes == 2) {
                uint16_t raw = (buf[0] << 8) | buf[1];
                
                if (!(raw & 0x04)) { // Cảm biến cắm chặt
                    temp = (raw >> 3) * 0.25;
                    printf("NHATTHANH: Nhiet do: %.2f C\n", temp);

                    sprintf(display_buf, "Temp: %.2f C", temp);
                    oled_clear(fd_oled);
                    oled_print_str(fd_oled, display_buf);

                    // Cảnh báo LED
                    if (temp > TEMP_THRESHOLD) write(fd_gpio, "1", 1);
                    else write(fd_gpio, "0", 1);

                    // THÀNH NHỚ ĐIỀN LINK FIREBASE CỦA BẠN VÀO DÒNG DƯỚI NÀY
                    sprintf(curl_cmd, "curl -k -X PATCH -d '{\"nhiet_do\": %.2f}' https://nhatthanhdhnhung-default-rtdb.asia-southeast1.firebasedatabase.app/data.json > /dev/null 2>&1 &", temp);
                    system(curl_cmd);

                } else { // Rút dây cảm biến
                    printf("NHATTHANH: Loi! Mat ket noi MAX6675\n");
                    oled_clear(fd_oled);
                    oled_print_str(fd_oled, "Sensor Error!");
                }
            }
            sleep(1); 
        } else {
            oled_clear(fd_oled);
            oled_print_str(fd_oled, "Status: STOPPED");
            usleep(500000); 
        }
    }
    return NULL;
}

int main() {
    pthread_t thread1, thread2;

    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("Loi he thong: Khong the khoi tao Mutex!\n");
        return 1;
    }

    printf("--- BTL HE DIEU HANH NHUNG - NHAT THANH ---\n");
    printf("He thong co kha nang tu phuc hoi khi loi!\n");

    pthread_create(&thread1, NULL, button_thread_func, NULL);
    pthread_create(&thread2, NULL, sensor_thread_func, NULL);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    pthread_mutex_destroy(&lock);
    return 0;
}
