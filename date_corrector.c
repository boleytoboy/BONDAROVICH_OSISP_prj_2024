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
        } else if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char subdir_path[PATH_MAX];
            snprintf(subdir_path, sizeof(subdir_path), "%s/%s", dir_path, entry->d_name);
            max_mtime = max_file_mtime(subdir_path, file_ext); // Рекурсивный вызов для обхода поддиректорий
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

// Функция для корректировки дат файлов указанного типа в архиве
void correct_file_dates(const char *archive_path) {
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
            return;
        }

        // Перебираем файлы в архиве и устанавливаем им максимальную дату
        gzFile outfile = gzopen("tempfile", "wb");
        if (!outfile) {
            perror("Ошибка при создании временного файла");
            gzclose(archive);
            return;
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
            return;
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
}

// Функция для просмотра содержимого архива
void view_archive_contents(const char *archive_path) {
    gzFile archive = gzopen(archive_path, "rb");
    if (!archive) {
        perror("Ошибка при открытии архива");
        return;
    }

    char buffer[1024];
    int num_read;
    printf("Содержимое архива %s:\n", archive_path);
    while ((num_read = gzread(archive, buffer, sizeof(buffer))) > 0) {
        printf("%.*s", num_read, buffer);
    }

    gzclose(archive);
}

// Функция для добавления файла в архив
void add_file_to_archive(const char *archive_path, const char *file_path) {
    gzFile archive = gzopen(archive_path, "ab");
    if (!archive) {
        perror("Ошибка при открытии архива для добавления файла");
        return;
    }

    FILE *file = fopen(file_path, "rb");
    if (!file) {
        perror("Ошибка при открытии файла для добавления в архив");
        gzclose(archive);
        return;
    }

    char buffer[1024];
    int num_read;
    while ((num_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        gzwrite(archive, buffer, num_read);
    }

    fclose(file);
    gzclose(archive);
}

// Функция для удаления файла из архива
void remove_file_from_archive(const char *archive_path, const char *file_name) {
    gzFile archive = gzopen(archive_path, "rb");
    if (!archive) {
        perror("Ошибка при открытии архива для удаления файла");
        return;
    }

    gzFile temp_archive = gzopen("tempfile", "wb");
    if (!temp_archive) {
        perror("Ошибка при создании временного файла");
        gzclose(archive);
        return;
    }

    char buffer[1024];
    int num_read;
    while ((num_read = gzread(archive, buffer, sizeof(buffer))) > 0) {
        if (strstr(buffer, file_name) == NULL) {
            gzwrite(temp_archive, buffer, num_read);
        }
    }

    gzclose(archive);
    gzclose(temp_archive);

    if (rename("tempfile", archive_path) != 0) {
        perror("Ошибка при замене архива");
        return;
    }
}

// Функция для поиска архивов в директории и её поддиректориях
void find_archives(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("Ошибка при открытии директории");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char *ext = strrchr(entry->d_name, '.');
            if (ext != NULL && strcmp(ext, ".zip") == 0) {
                printf("Найден архив: %s/%s\n", dir_path, entry->d_name);
                correct_file_dates(entry->d_name);
            }
        } else if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char subdir_path[PATH_MAX];
            snprintf(subdir_path, sizeof(subdir_path), "%s/%s", dir_path, entry->d_name);
            find_archives(subdir_path); // Рекурсивный вызов для обхода поддиректорий
        }
    }

    closedir(dir);
}

// Функция для компиляции программы
void compile_program() {
    system("gcc -o date_corrector date_corrector.c -lz");
}

// Функция для запуска программы
void run_program() {
    system("./date_corrector <начальная_директория>");
}

int main() {
    // Компилируем программу
    compile_program();

    // Запускаем программу
    run_program();

    return EXIT_SUCCESS;
}
