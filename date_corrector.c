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
#include <libgen.h> 

#define PATH_MAX 4096

// Функция для получения максимальной даты изменения файла указанного типа в архиве
time_t max_file_mtime_in_archive(const char *archive_path, const char *file_ext) {
    time_t max_mtime = 0;

    // Открываем архив для чтения
    gzFile archive = gzopen(archive_path, "rb");
    if (!archive) {
        perror("Ошибка при открытии архива");
        return max_mtime;
    }

    // Структура для хранения информации о файле в архиве
    gz_header header;
    int ret;

    // Читаем заголовок каждого файла в архиве
    while ((ret = gzread(archive, &header, sizeof(gz_header))) > 0) {
        // Получаем имя файла из заголовка
        char *ext = strrchr(header.name, '.');
        if (ext != NULL && strcmp(ext, file_ext) == 0) {
            // Получаем информацию о файле
            struct stat st;
            if (stat(header.name, &st) == 0 && st.st_mtime > max_mtime) {
                max_mtime = st.st_mtime;
            }
        }

        // Пропускаем содержимое файла
        gzseek(archive, header.extra_len + strlen(header.name) + 1, SEEK_CUR);

    }

    gzclose(archive);

    return max_mtime;
}

// Функция для установки времени последней модификации файла в архиве
void set_archive_file_mtime(const char *archive_path, const char *file_ext, time_t mtime) {
    // Открываем архив для чтения и записи
    gzFile archive = gzopen(archive_path, "rb");
    if (!archive) {
        perror("Ошибка при открытии архива");
        return;
    }

    // Создаем временный файл для записи обновленного содержимого архива
    gzFile temp_archive = gzopen("tempfile", "wb");
    if (!temp_archive) {
        perror("Ошибка при создании временного файла");
        gzclose(archive);
        return;
    }

    // Структура для хранения информации о файле в архиве
    gz_header header;
    int ret;

    // Читаем заголовок каждого файла в архиве
    while ((ret = gzread(archive, &header, sizeof(gz_header))) > 0) {
        // Получаем имя файла из заголовка
        char *ext = strrchr(header.name, '.');
        if (ext != NULL && strcmp(ext, file_ext) == 0) {
            // Устанавливаем новую дату изменения файла
            struct utimbuf new_times;
            new_times.actime = mtime;
            new_times.modtime = mtime;
            utime(header.name, &new_times);
        }

        // Записываем заголовок и содержимое файла во временный архив
        gzwrite(temp_archive, &header, sizeof(gz_header));
        char buffer[1024];
        int num_read;
        while ((num_read = gzread(archive, buffer, sizeof(buffer))) > 0) {
            gzwrite(temp_archive, buffer, num_read);
        }
    }

    gzclose(archive);
    gzclose(temp_archive);

    // Заменяем исходный архив временным файлом
    if (rename("tempfile", archive_path) != 0) {
        perror("Ошибка при замене архива");
        return;
    }
}

// Функция для корректировки дат файлов указанного типа в архиве
void correct_file_dates(const char *archive_path) {
    char choice;

    while (1) {
        // Предопределенные типы файлов
        const char *file_types[] = {"txt", "jpg", "png", "pdf", "doc"};
        int num_file_types = sizeof(file_types) / sizeof(file_types[0]);

        printf("Выберите тип файла для корректировки дат:\n");
        for (int i = 0; i < num_file_types; ++i) {
            printf("%d. %s\n", i + 1, file_types[i]);
        }

        int selected_type;
        printf("Введите номер типа файла: ");
        scanf("%d", &selected_type);

        if (selected_type < 1 || selected_type > num_file_types) {
            printf("Некорректный выбор типа файла.\n");
            return;
        }

        const char *file_ext = file_types[selected_type - 1];

        // Получаем максимальную дату изменения файла указанного типа
        time_t max_mtime = max_file_mtime_in_archive(archive_path, file_ext);

        // Устанавливаем максимальную дату изменения для всех файлов в архиве указанного типа
        set_archive_file_mtime(archive_path, file_ext, max_mtime);

        printf("Максимальная дата изменения для файлов типа %s в архиве %s установлена в %s\n",
               file_ext, archive_path, ctime(&max_mtime));

        printf("Продолжить корректировку дат для других файлов? (y/n): ");
        scanf(" %c", &choice);
        if (choice != 'y') {
            break;
        }
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

    // Буфер для чтения данных из архива
    char buffer[1024];
    int num_read;

    // Читаем содержимое архива
    while ((num_read = gzread(archive, buffer, sizeof(buffer))) > 0) {
        // Выводим содержимое на экран
        fwrite(buffer, 1, num_read, stdout);
    }

    if (num_read < 0) {
        perror("Ошибка при чтении архива");
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

// Функция для проверки выполнения корректировки дат
int check_correction_done(const char *archive_path) {
    struct stat st;
    if (stat(archive_path, &st) != 0) {
        perror("Ошибка при получении информации об архиве");
        return 0;
    }

    time_t mtime = st.st_mtime;
    time_t current_time = time(NULL);

    return difftime(current_time, mtime) <= 0;
}

// Функция для отображения меню и обработки выбора пользователя
void display_menu(const char *archive_path) {
    int choice;
    printf("\nМеню:\n");
    printf("1. Корректировка дат файлов в архиве\n");
    printf("2. Просмотр содержимого архива\n");
    printf("3. Добавление файла в архив\n");
    printf("4. Удаление файла из архива\n");
    printf("5. Проверка выполнения корректировки дат\n");
    printf("6. Выбор другого архива\n");
    printf("7. Выход\n");
    printf("Выберите действие: ");
    scanf("%d", &choice);

    switch (choice) {
        case 1:
            correct_file_dates(archive_path);
            break;
        case 2:
            view_archive_contents_with_dates(archive_path);
            break;
        case 3: {
            char file_path[PATH_MAX];
            printf("Введите путь к файлу для добавления в архив: ");
            scanf("%s", file_path);
            add_file_to_archive(archive_path, file_path);
            break;
        }
        case 4: {
            char file_name[PATH_MAX];
            printf("Введите имя файла для удаления из архива: ");
            scanf("%s", file_name);
            remove_file_from_archive(archive_path, file_name);
            break;
        }
        case 5:
            if (check_correction_done(archive_path)) {
                printf("Корректировка дат выполнена.\n");
            } else {
                printf("Корректировка дат не выполнена.\n");
            }
            break;
        case 6:
            return; // Возврат к выбору архива
        case 7:
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
