#include "archive_functions.h"
#include "date_corrector.h"

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
