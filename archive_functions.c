#include "archive_functions.h"
#include "date_corrector.h"

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
