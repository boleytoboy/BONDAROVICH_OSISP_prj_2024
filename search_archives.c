#include "archive_functions.h"
#include "search_archives.h"

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
            // Проверка на расширение .tar.gz
            size_t len = strlen(entry->d_name);
            if (len > 7 && strcmp(entry->d_name + len - 7, ".tar.gz") == 0) {
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

void run_program(const char *start_dir) {
    // Находим все архивы в указанной директории и ее поддиректориях
    char archives[100][PATH_MAX]; // Массив для хранения найденных архивов
    int num_archives = 0;
    find_archives(start_dir, archives, &num_archives);

    if (num_archives == 0) {
        printf("В указанной директории и её поддиректориях не найдено архивов.\n");
        return;
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
        return;
    }

    const char *archive_path = archives[selected_archive - 1];

    // Обрабатываем выбранный архив
    process_selected_archive(archive_path);
}
