#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <getopt.h>
#include <string.h>
#include "utils.h"
#include "sum_utils.h"

void *ThreadSum(void *args) {
    struct SumArgs *sum_args = (struct SumArgs *)args;
    return (void *)(size_t)Sum(sum_args);
}

int main(int argc, char **argv) {
    uint32_t threads_num = 0;
    uint32_t array_size = 0;
    uint32_t seed = 0;
    
    // Парсинг аргументов командной строки
    static struct option options[] = {
        {"threads_num", required_argument, 0, 0},
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    while (1) {
        int c = getopt_long(argc, argv, "", options, &option_index);
        if (c == -1) break;

        switch (c) {
            case 0:
                switch (option_index) {
                    case 0:
                        threads_num = atoi(optarg);
                        if (threads_num <= 0) {
                            printf("threads_num must be a positive number\n");
                            return 1;
                        }
                        break;
                    case 1:
                        seed = atoi(optarg);
                        break;
                    case 2:
                        array_size = atoi(optarg);
                        if (array_size <= 0) {
                            printf("array_size must be a positive number\n");
                            return 1;
                        }
                        break;
                    default:
                        printf("Index %d is out of options\n", option_index);
                }
                break;
            case '?':
                break;
            default:
                printf("getopt returned character code 0%o?\n", c);
        }
    }

    if (threads_num == 0 || array_size == 0) {
        printf("Usage: %s --threads_num \"num\" --seed \"num\" --array_size \"num\"\n", argv[0]);
        return 1;
    }

    // Выделение памяти для массива
    int *array = malloc(sizeof(int) * array_size);
    if (array == NULL) {
        printf("Memory allocation failed for array\n");
        return 1;
    }

    // Генерация массива (не входит в замер времени)
    GenerateArray(array, array_size, seed);

    // Создание потоков
    pthread_t threads[threads_num];
    struct SumArgs args[threads_num];
    
    // Вычисление размера чанка для каждого потока
    int chunk_size = array_size / threads_num;
    int remainder = array_size % threads_num;

    // Начало замера времени
    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    // Создание и запуск потоков
    for (uint32_t i = 0; i < threads_num; i++) {
        args[i].array = array;
        args[i].begin = i * chunk_size;
        args[i].end = (i == threads_num - 1) ? (args[i].begin + chunk_size + remainder) : (args[i].begin + chunk_size);
        
        if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i])) {
            printf("Error: pthread_create failed!\n");
            free(array);
            return 1;
        }
    }

    // Сбор результатов от потоков
    int total_sum = 0;
    for (uint32_t i = 0; i < threads_num; i++) {
        int sum = 0;
        pthread_join(threads[i], (void **)&sum);
        total_sum += sum;
    }

    // Конец замера времени
    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);

    // Вычисление времени выполнения
    double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    // Освобождение памяти и вывод результатов
    free(array);
    printf("Total: %d\n", total_sum);
    printf("Elapsed time: %fms\n", elapsed_time);
    
    return 0;
}