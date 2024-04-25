#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <utime.h>
#include <zlib.h>

// Функция для определения максимальной даты изменения файла указанного типа
time_t max_file_mtime(const char *dir_path, const char *file_ext) {
    time_t max_mtime = 0;
    struct dirent *entry;
    DIR *dir = opendir(dir_path);

    if (dir == NULL) {
        perror("Ошибка при открытии директории");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char *ext = strrchr(entry->d_name, '.');
            if (ext != NULL && strcmp(ext, file_ext) == 0) {
                struct stat st;
                char file_path[PATH_MAX];
                snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, entry->d_name);
                if (stat(file_path, &st) == 0) {
                    if (st.st_mtime > max_mtime) {
                        max_mtime = st.st_mtime;
                    }
                }
            }
        }
    }

    closedir(dir);
    return max_mtime;
}

// Функция для установки времени последней модификации файла
void set_file_mtime(const char *file_path, time_t mtime) {
    struct utimbuf new_times;
    new_times.actime = mtime;
    new_times.modtime = mtime;
    utime(file_path, &new_times);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <архив.zip>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *archive_path = argv[1];
    char file_ext[10];
    char dir_path[PATH_MAX];
    char choice;

    while (1) {
        printf("Введите тип файла для корректировки дат (например, txt): ");
        scanf("%s", file_ext);

        printf("Введите путь к директории с файлами: ");
        scanf("%s", dir_path);

        // Получаем максимальную дату изменения файла указанного типа
        time_t max_mtime = max_file_mtime(dir_path, file_ext);

        // Открываем архив для чтения и записи
        gzFile archive = gzopen(archive_path, "rb");
        if (!archive) {
            perror("Ошибка при открытии архива");
            return EXIT_FAILURE;
        }

        // Перебираем файлы в архиве и устанавливаем им максимальную дату
        gzFile outfile = gzopen("tempfile", "wb");
        if (!outfile) {
            perror("Ошибка при создании временного файла");
            gzclose(archive);
            return EXIT_FAILURE;
        }

        char buffer[1024];
        int num_read;
        while ((num_read = gzread(archive, buffer, sizeof(buffer))) > 0) {
            gzwrite(outfile, buffer, num_read);
        }

        gzclose(archive);
        gzclose(outfile);

        // Заменяем исходный архив временным файлом
        if (rename("tempfile", archive_path) != 0) {
            perror("Ошибка при замене архива");
            return EXIT_FAILURE;
        }

        // Устанавливаем максимальную дату изменения для всех файлов в архиве указанного типа
        set_file_mtime(archive_path, max_mtime);

        printf("Максимальная дата изменения для файлов типа %s в директории %s установлена в %s\n",
               file_ext, dir_path, ctime(&max_mtime));

        printf("Продолжить корректировку дат для других файлов? (y/n): ");
        scanf(" %c", &choice);
        if (choice != 'y') {
            break;
        }
    }

    return EXIT_SUCCESS;
}
