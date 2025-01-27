/*
** EPITECH PROJECT, 2025
** WORKSHOP BUFFER OVERFLOWS
** File description:
** PART3
*/

#ifndef PART2_H
#define PART2_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/select.h>
#include <signal.h>

#define MAX_CLIENTS 10
#define PORT 4242
#define BUFFER_SIZE 1024

static int server_fd = -1;

typedef struct command_s {
    const char *name;
    void (*handle)(int client_fd, int *clients, int max_clients, char *buffer, char *inbox_msg);
} command_t;

static const char *help_message = \
    "/help : Afficher cette page d'aide.\n" \
    "/inbox <view|send> <length> <message> : Gérer l'inbox général du serveur.\n";

void handle_client_command_help(int client_fd, int *, int, char *, char *);

void handle_client_command_inbox(int client_fd, int *clients, int max_clients, char *buffer, char *inbox_msg);

static const command_t commands[] = {
    { "help", handle_client_command_help },
    { "inbox", handle_client_command_inbox }
};

#define COMMANDS_COUNT (sizeof(commands) / sizeof(commands[0]))

#endif
