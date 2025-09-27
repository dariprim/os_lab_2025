#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    // Проверяем количество аргументов
    if (argc != 3) {
        printf("Usage: %s seed array_size\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("Ошибка при создании процесса");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Передаём аргументы в sequential_min_max
        execlp("./sequential_min_max", "sequential_min_max", argv[1], argv[2], NULL);
        perror("Ошибка при выполнении программы");
        exit(EXIT_FAILURE);
    }

    wait(NULL);
    printf("Процесс завершен\n");

    return 0;
}
