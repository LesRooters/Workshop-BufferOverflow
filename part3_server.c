/*
** EPITECH PROJECT, 2025
** WORKSHOP BUFFER OVERFLOWS
** File description:
** PART3
*/

#include "part3_server.h"

void error_exit(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void handle_sigint(int sig)
{
    if (server_fd != -1) {
        close(server_fd);
        printf("Server shut down cleanly.\n");
    }
    exit(0);
}

int create_server_socket(void)
{
    int server_fd;
    struct sockaddr_in addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
        error_exit("socket");

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        error_exit("bind");

    if (listen(server_fd, MAX_CLIENTS) < 0)
        error_exit("listen");

    return server_fd;
}

void broadcast_message(int sender, int *clients, int max_clients, const char *msg, size_t len)
{
    for (int i = 0; i < max_clients; i++) {
        if (clients[i] != -1 && clients[i] != sender && send(clients[i], msg, len, 0) < 0) {
            perror("send");
        }
    }
}

uint16_t receive_message_length(int client_fd)
{
    uint16_t msg_len;

    if (recv(client_fd, &msg_len, sizeof(msg_len), 0) <= 0) {
        return 0;
    }
    return ntohs(msg_len);
}

int receive_message_content(int client_fd, uint16_t msg_len, char *buffer)
{
    if (msg_len > BUFFER_SIZE || recv(client_fd, buffer, msg_len, 0) <= 0) {
        return -1;
    }
    buffer[msg_len] = '\0';
    return 0;
}

void close_client_connection(int client_fd, int *clients, int max_clients)
{
    close(client_fd);
    for (int i = 0; i < max_clients; i++) {
        if (clients[i] == client_fd) {
            clients[i] = -1;
            break;
        }
    }
}

void handle_client_command_help(int client_fd, int *, int, char *)
{
    uint16_t help_msg_len = sizeof(char) * strlen(help_message);

    send(client_fd, &help_msg_len, sizeof(help_msg_len), 0);
    send(client_fd, help_message, help_msg_len, 0);
}

void handle_admin_panel()
{
    printf("PAWNED!\n");
}

void handle_client_command_list(int client_fd, int *clients, int max_clients, char *buffer)
{
    int list_max = atoi(buffer);

    if (list_max <= 0 || list_max > 2048) {
        return;
    }
    uint16_t send_max = sizeof(int) * list_max;
    int *send_buffer = malloc(send_max);
    if (send_buffer == NULL) {
        return;
    }
    memcpy(send_buffer, clients, send_max);
    size_t escaped_size = send_max * 4 + 1;
    char *escaped_buffer = malloc(escaped_size);
    if (escaped_buffer == NULL) {
        free(send_buffer);
        return;
    }
    char *escaped_ptr = escaped_buffer;
    for (size_t i = 0; i < send_max; i++) {
        uint8_t byte = ((uint8_t *)send_buffer)[i];
        escaped_ptr += sprintf(escaped_ptr, "\\x%02X", byte);
    }
    uint16_t escaped_length = strlen(escaped_buffer);
    send(client_fd, &escaped_length, sizeof(escaped_length), 0);
    send(client_fd, escaped_buffer, escaped_length, 0);
    free(send_buffer);
    free(escaped_buffer);
}

void handle_client_command_upload(int client_fd, int *clients, int max_clients, char *buffer)
{
    //08049616
}

void handle_client_command(int client_fd, int *clients, int max_clients, char *buffer)
{
    command_t *command_ptr = (command_t *)commands;
    size_t command_len = 0;

    for (; command_ptr < commands + COMMANDS_COUNT; command_ptr++) {
        command_len = strlen(command_ptr->name);
        if (strncmp(command_ptr->name, buffer, command_len) == 0) {
            command_ptr->handle(client_fd, clients, max_clients, &buffer[command_len]);
        }
    }
}

void handle_client_message(int client_fd, int *clients, int max_clients)
{
    uint16_t msg_len = receive_message_length(client_fd);
    char buffer[BUFFER_SIZE];

    if (msg_len == 0) {
        close_client_connection(client_fd, clients, max_clients);
        return;
    }
    if (receive_message_content(client_fd, msg_len, buffer) == -1) {
        close_client_connection(client_fd, clients, max_clients);
        return;
    }
    if (buffer[0] == '/') {
        handle_client_command(client_fd, clients, max_clients, &buffer[1]);
        return;
    }
    broadcast_message(client_fd, clients, max_clients, (char *)&msg_len, sizeof(msg_len));
    broadcast_message(client_fd, clients, max_clients, buffer, msg_len);
}

void setup_fd_sets(int server_fd, int *clients, fd_set *read_fds, int *max_fd)
{
    FD_ZERO(read_fds);
    FD_SET(server_fd, read_fds);
    *max_fd = server_fd;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != -1) {
            FD_SET(clients[i], read_fds);
            if (clients[i] > *max_fd)
                *max_fd = clients[i];
        }
    }
}

void accept_new_client(int server_fd, int *clients)
{
    int client_fd = accept(server_fd, NULL, NULL);

    if (client_fd < 0) {
        perror("accept");
        return;
    }
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == -1) {
            clients[i] = client_fd;
            break;
        }
    }
}

void handle_active_clients(fd_set *read_fds, int *clients, int max_clients)
{
    for (int i = 0; i < max_clients; i++) {
        if (clients[i] != -1 && FD_ISSET(clients[i], read_fds)) {
            handle_client_message(clients[i], clients, max_clients);
        }
    }
}

void server_loop(int server_fd)
{
    int clients[MAX_CLIENTS];
    fd_set read_fds;
    int max_fd;

    memset(clients, -1, sizeof(clients));
    while (1) {
        setup_fd_sets(server_fd, clients, &read_fds, &max_fd);

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0)
            error_exit("select");

        if (FD_ISSET(server_fd, &read_fds))
            accept_new_client(server_fd, clients);

        handle_active_clients(&read_fds, clients, MAX_CLIENTS);
    }
}

int main(void)
{
    server_fd = create_server_socket();

    signal(SIGINT, handle_sigint);
    printf("Server running on port %d\n", PORT);
    server_loop(server_fd);
    close(server_fd);
    return 0;
}
