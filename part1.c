/*
** EPITECH PROJECT, 2025
** WORKSHOP BUFFER OVERFLOWS
** File description:
** PART1
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void secret_function(void)
{
    printf("Bravo ! Vous avez exécuté la fonction secrète !\n");
}

void vulnerable_function(char *input)
{
    char buffer[64];

    strcpy(buffer, input);
    printf("Bienvenue %s\n", buffer);
}

int main(void)
{
    char input[256];

    printf("Quel est votre nom ? ");
    fgets(input, sizeof(input), stdin);
    vulnerable_function(input);
    return 0;
}
