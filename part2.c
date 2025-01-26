/*
** EPITECH PROJECT, 2025
** WORKSHOP BUFFER OVERFLOWS
** File description:
** PART2
*/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    char buffer[64];

    gets(buffer);
}
