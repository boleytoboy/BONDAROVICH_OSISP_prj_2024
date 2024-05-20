#include "search_archives.h"
#include "archive_functions.h"
#include "date_corrector.h"

int main() {
     // Укажите начальную директорию для поиска архивов
    const char *start_dir = "/home/xleb";

    run_program(start_dir);

    return EXIT_SUCCESS;
}
