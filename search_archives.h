#ifndef SEARCH_ARCHIVES_H
#define SEARCH_ARCHIVES_H
#define PATH_MAX 4096

void find_archives(const char *dir_path, char archives[][PATH_MAX], int *num_archives);
void run_program(const char *start_dir);

#endif 
