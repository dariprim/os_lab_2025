#!/bin/bash  
  
# $# в Bash — специальная переменная, которая хранит количество аргументов командной строки, переданных скрипту.  
sum=0  
count=$#  
for arg in "$@"; do 
    # Суммируем числа  
    sum=$(echo "$sum + $arg"| bc -l)  
done  
average=$(echo "$sum / $count" | bc -l)  
  
# Выводим результат  
echo "Количество чисел: $count"  
echo "Среднее арифметическое: $average"  
