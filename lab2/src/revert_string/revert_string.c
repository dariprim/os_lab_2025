#include "revert_string.h"

void RevertString(char *str)
{
    // два указателя для начала и конца, будем двигать их к друг другу
    char *start = str;
    char *end = str;
    
    // Находим конец строки
    while (*end != '\0') {
        end++;
    }
    end--; // Возвращаемся к последнему символу перед '\0'
    
    // Меняем символы местами
    while (start < end) {
        char temp = *start;
        *start = *end;
        *end = temp;
        
        start++;
        end--;
    }
}
