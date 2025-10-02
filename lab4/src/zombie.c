#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int main() {
    pid_t pid;
    int status;
    
    printf("Родительский процесс PID: %d\n", (int)getpid());
    
    // Создаем дочерний процесс
    pid = fork();
    
    if (pid < 0) {
        perror("Ошибка при создании процесса");
        exit(1);
    }
    
    if (pid == 0) {
        // Дочерний процесс
        printf("Дочерний процесс PID: %d (родитель: %d)\n", 
               (int)getpid(), (int)getppid());
        printf("Дочерний процесс завершается...\n");
        exit(42); // Завершаемся с кодом 42
    } else {
        // Родительский процесс
        printf("Родитель создал дочерний процесс с PID: %d\n", (int)pid);
        printf("\nТеперь дочерний процесс стал ЗОМБИ на 20 секунд!\n");
        
        // Ждем 20 секунд - в это время дочерний процесс зомби
        sleep(20);
        
        // Забираем статус завершения дочернего процесса
        pid_t finished_pid = wait(&status);
        
        if (finished_pid == -1) {
            perror("Ошибка wait");
        } else if (WIFEXITED(status)) {
            printf("Дочерний процесс %d завершился со статусом: %d\n", 
                   (int)finished_pid, WEXITSTATUS(status));
        }
        
        printf("Зомби-процесс уничтожен!\n");
    }
    
    return 0;
}