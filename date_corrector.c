#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <zlib.h>
#include <tar.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <utime.h>
#include <archive.h>
#include <archive_entry.h>
#include <stdbool.h>

#define PATH_MAX 4096

typedef struct posix_header
{                              /* byte offset */
    char name[100];               /*   0 */
    char mode[8];                 /* 100 */
    char uid[8];                  /* 108 */
    char gid[8];                  /* 116 */
    char size[12];
    char mtime[12];               /* 136 */
    char chksum[8];               /* 148 */
    char typeflag;                /* 156 */
    char linkname[100];           /* 157 */
    char magic[6];                /* 257 */
    char version[2];              /* 263 */
    char uname[32];               /* 265 */
    char gname[32];               /* 297 */
    char devmajor[8];             /* 329 */
    char devminor[8];             /* 337 */
    char prefix[155];             /* 345 */
    /* 500 */
} posix_header;

int next_file_in_tar(gzFile* archive, posix_header* header_out) {
    static char buffer[512];
    posix_header* header = (posix_header*)buffer;

    int num_read = gzread(*archive, buffer, 512);

    char size_str[13] = { '\0' };
    memcpy(size_str, header->size, sizeof(header->size));
    uint64_t size = strtol(size_str, NULL, 8);

    if (size % 512 != 0) {
        size += 512 - (size % 512);
    }

    // Пропускаем содержимое файла
    gzseek(*archive, size, SEEK_CUR);

    if (header->name[0] != '\0' && num_read == 512) {
        *header_out = *header;
        return num_read + size;
    }
    else {
        return 0;
    }
}

void view_arcive_contents(gzFile* archive) {
    posix_header header;
    int num_read = 0;

    // Читаем содержимое архива
    while ((num_read = next_file_in_tar(archive, &header)) != 0) {
        // Дата последней модификации
        char mtime_str[13] = { '\0' };
        memcpy(mtime_str, header.mtime, sizeof(header.mtime));
        time_t mtime = strtol(mtime_str, NULL, 8);

        // Размер файла
        char size_str[13] = { '\0' };
        memcpy(size_str, header.size, sizeof(header.size));
        uint64_t size = strtol(size_str, NULL, 8);
        printf("Файл: %s: %llu байт: %s", header.name, (unsigned long long)size, ctime(&mtime));
    }
}

// Функция для просмотра содержимого текстовых файлов в архиве с датами
void view_archive_contents_with_dates(const char *archive_path) {
    gzFile archive = gzopen(archive_path, "rb");
    if (!archive) {
        perror("Ошибка при открытии архива");
        return;
    }

    printf("Содержимое архива %s:\n", archive_path);

    view_arcive_contents(&archive);

    gzclose(archive);
}

void print_checksum(posix_header* header) {
    char sum_str[9] = { '\0' };
    memcpy(sum_str, header->chksum, sizeof(header->chksum));
    uint64_t checksum = strtol(sum_str, NULL, 8);

    printf("Checksum: %u\n", checksum);

    uint64_t sum = 0;
    for (int i = 0; i < sizeof(posix_header); ++i) {
        sum += ((const char*)header)[i];
    }

    uint64_t other = 0;
    for (int i = 0; i < sizeof(header->chksum); ++i) {
        other += header->chksum[i];
    }

    printf("Sum: %u : %u : %u\n", sum, other, sum - other);
}

void copy_archive(gzFile* src, FILE* dst, posix_header* header) {
    gzseek(*src, 0, SEEK_SET);

    static char buffer[512] = { '\0' };
    int num_read = 0;

    while ((num_read = gzread(*src, buffer, 512)) > 0) {
        posix_header* curr_file = (posix_header*)buffer;

        if (memcmp(curr_file->name, header->name, sizeof(header->name)) == 0) {
            memcpy(curr_file->mtime, header->mtime, sizeof(header->mtime));
        }

        fwrite(buffer, 512, 1, dst);
    }

    return;
}

void archivate(FILE* file, const char* path) {
    gzFile arch = gzopen(path, "wb");

    fseek(file, 0, SEEK_END);
    off_t size = ftell(file);

    if (size % 512 != 0)
        size += 512 - (size % 512);
    
    size /= 512;

    char buffer[512] = { '\0' };

    fseek(file, 0, SEEK_SET);

    for (int i = 0; i < size; ++i) {
        fread(buffer, 512, 1, file);
        gzwrite(arch, buffer, 512);
    }

    gzclose(arch);
}

// Функция для корректировки дат файлов указанного типа в архиве
void correct_file_dates(const char *archive_path) {
    char choice;
    // Открываем архив

    char temp_path[256] = { '\0' };

    strcat(temp_path, archive_path);
    strcat(temp_path, ".temp.tar");
    
    printf("temp file path: %s\n", temp_path);

    FILE* temp_arch = fopen(temp_path, "w");
    gzFile archive = gzopen(archive_path, "rb");
    if (!archive || !temp_arch) {
        perror("Ошибка при открытии архива");
        return;
    }

    printf("Выберите файл для корректировки дат:\n");

    posix_header header;
    int num_read = 0;

    {
        int file_num = 0;

        // Читаем содержимое архива
        while ((num_read = next_file_in_tar(&archive, &header)) != 0) {
            file_num++;
            printf("%u. %s\n", file_num, header.name);
        }

        scanf("%u", &file_num);
        gzseek(archive, 0, SEEK_SET);

        for (int i = 0; i < file_num; ++i) {
            num_read = next_file_in_tar(&archive, &header);
        }
    }

    char mtime_str[13] = { '\0' };
    memcpy(mtime_str, header.mtime, sizeof(header.mtime));
    time_t mtime = strtol(mtime_str, NULL, 8);
    printf("Дата последней модификации: %s", ctime(&mtime));

    time_t current_time = time(NULL);

    sprintf(header.mtime, "%12o", current_time);
    printf("Новое время: %s, %012o", ctime(&current_time), current_time);

    copy_archive(&archive, temp_arch, &header);

    gzclose(archive);
    fclose(temp_arch);

    temp_arch = fopen(temp_path, "r");
    archivate(temp_arch, archive_path);
    fclose(temp_arch);
}

// Функция для отображения меню и обработки выбора пользователя
void display_menu(const char *archive_path) {
    int choice;
    printf("\nМеню:\n");
    printf("1. Корректировка дат файлов в архиве\n");
    printf("2. Просмотр содержимого архива\n");
    printf("3. Выход\n");
    printf("Выберите действие: ");
    scanf("%d", &choice);

    switch (choice) {
        case 1:
            correct_file_dates(archive_path);
            break;
        case 2:
            view_archive_contents_with_dates(archive_path);
            break;
        case 3:
            printf("Программа завершена.\n");
            exit(EXIT_SUCCESS);
        default:
            printf("Некорректный выбор. Пожалуйста, выберите снова.\n");
            break;
    }
}

// Функция для обработки выбора архива и отображения меню
void process_selected_archive(const char *archive_path) {
    // Отображаем меню для выбранного архива
    while (1) {
        display_menu(archive_path);
    }
}

// Функция для поиска архивов в директории и её поддиректориях
void find_archives(const char *dir_path, char archives[][PATH_MAX], int *num_archives) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("Ошибка при открытии директории");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char *ext = strrchr(entry->d_name, '.');
            if (ext != NULL && strcmp(ext, ".gz") == 0) {
                snprintf(archives[*num_archives], PATH_MAX, "%s/%s", dir_path, entry->d_name);
                (*num_archives)++;
            }
        } else if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char subdir_path[PATH_MAX];
            snprintf(subdir_path, sizeof(subdir_path), "%s/%s", dir_path, entry->d_name);
            find_archives(subdir_path, archives, num_archives); // Рекурсивный вызов для обхода поддиректорий
        }
    }

    closedir(dir);
}


int main() {
    // Укажите начальную директорию для поиска архивов
    const char *start_dir = "/home/xleb";

    // Находим все архивы в указанной директории и ее поддиректориях
    char archives[100][PATH_MAX]; // Массив для хранения найденных архивов
    int num_archives = 0;
    find_archives(start_dir, archives, &num_archives);

    if (num_archives == 0) {
        printf("В указанной директории и её поддиректориях не найдено архивов.\n");
        return EXIT_FAILURE;
    }

    printf("Найденные архивы:\n");
    for (int i = 0; i < num_archives; ++i) {
        printf("%d. %s\n", i + 1, archives[i]);
    }

    int selected_archive;
    printf("Выберите номер архива для продолжения: ");
    scanf("%d", &selected_archive);

    if (selected_archive < 1 || selected_archive > num_archives) {
        printf("Некорректный выбор архива.\n");
        return EXIT_FAILURE;
    }

    const char *archive_path = archives[selected_archive - 1];

    // Обрабатываем выбранный архив
    process_selected_archive(archive_path);

    return EXIT_SUCCESS;
}
