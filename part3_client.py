#!/bin/python3

import socket
import struct
import curses
import time
import argparse
import signal
import sys
import select
import logging

PORT = 4242
BUFFER_SIZE = 1024
lines = []
input_str = ""
last_rendered_lines = []
redirect_to_file = None
scroll_offset = 0
input_history = []
input_history_index = 0

def setup_logger(log_file):
    logger = logging.getLogger('log_file_logger')

    logger.setLevel(logging.DEBUG)
    file_handler = logging.FileHandler(log_file)
    file_handler.setLevel(logging.DEBUG)    
    formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
    file_handler.setFormatter(formatter)
    logger.addHandler(file_handler)
    return logger

logger = setup_logger("part3_client.log")

def send_message(sock, message):
    """Envoie un message au serveur."""
    try:
        msg_len = len(message)
        msg_len_packed = struct.pack('!H', msg_len)
        sock.sendall(msg_len_packed + message)
    except Exception as e:
        lines.append(('error', f"Erreur d'envoi : {e}"))

def receive_message(sock):
    """Reçoit un message du serveur."""
    try:
        msg_len_packed = sock.recv(2)
        if not msg_len_packed:
            return None
        msg_len = struct.unpack('!H', msg_len_packed)[0]
        message = sock.recv(msg_len)
        return message
    except Exception as e:
        lines.append(('error', f"Erreur de réception : {e}"))
        logger.debug(f"Erreur de réception : {e}")
        return None

def init_curses():
    """Initialise les paramètres de curses."""
    curses.start_color()
    curses.use_default_colors()
    curses.init_pair(1, curses.COLOR_YELLOW, -1)
    curses.init_pair(2, curses.COLOR_BLACK, curses.COLOR_WHITE)
    curses.init_pair(3, curses.COLOR_RED, -1)
    curses.curs_set(0)

def setup_socket(server_ip):
    """Crée et connecte un socket au serveur."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.connect((server_ip, PORT))
        return sock
    except Exception as e:
        lines.append(('error', f"Impossible de se connecter : {e}"))
        return None

def draw_title(stdscr, max_x, server_ip):
    """Dessine la barre de titre."""
    stdscr.attron(curses.color_pair(2))
    banner_title = f"Part2 Client 1.0 - [connected] {server_ip}"
    stdscr.addstr(0, 0, banner_title[:max_x] + " " * (max_x - len(banner_title)))
    stdscr.attroff(curses.color_pair(2))

def draw_lines(stdscr, max_y, max_x, current_lines):
    """Affiche les lignes de messages."""
    for i, (msg_type, line) in enumerate(current_lines):
        line_num = i + 1
        if line_num >= max_y - 1:
            break
        truncated_line = line[:max_x - 1]
        if msg_type == 'received':
            stdscr.addstr(line_num, 0, truncated_line, curses.color_pair(1))
        elif msg_type == 'error':
            stdscr.addstr(line_num, 0, truncated_line, curses.color_pair(3))
        else:
            stdscr.addstr(line_num, 0, truncated_line)

def draw_input_field(stdscr, max_y, input_str):
    """Dessine la zone de saisie de texte."""
    stdscr.addstr(max_y - 1, 0, "> " + input_str, curses.A_BOLD)

def draw_interface(stdscr, force_update=False):
    """Dessine l'interface utilisateur complète."""
    global last_rendered_lines, scroll_offset

    max_y, max_x = stdscr.getmaxyx()
    max_displayable_lines = max_y - 2
    start_index = max(0, len(lines) - max_displayable_lines - scroll_offset)
    current_lines = lines[start_index:]

    if current_lines == last_rendered_lines and not force_update:
        return

    stdscr.clear()

    draw_title(stdscr, max_x, SERVER_IP)
    draw_lines(stdscr, max_y, max_x, current_lines)
    draw_input_field(stdscr, max_y, input_str)

    stdscr.refresh()
    last_rendered_lines = current_lines

def handle_input_key(key, stdscr, sock):
    """Gère les actions en fonction de la touche pressée."""
    global input_str, input_history, input_history_index, redirect_to_file

    if key == 10:
        process_input_command(sock)
    elif key == 27:
        return False
    elif key == 263:
        input_str = input_str[:-1]
    elif key == curses.KEY_RESIZE:
        draw_interface(stdscr, force_update=True)
    elif key == curses.KEY_UP:
        navigate_input_history(-1)
    elif key == curses.KEY_DOWN:
        navigate_input_history(1)
    elif 0 <= key < 256:
        input_str += chr(key)

    draw_interface(stdscr, force_update=True)
    return True

def process_input_command(sock):
    """Traite la commande entrée par l'utilisateur."""
    global input_str, redirect_to_file

    if input_str:
        if '>' in input_str:
            parts = input_str.split('>')
            command = parts[0].strip()
            redirect_to_file = parts[1].strip()
            send_message(sock, command.encode())
        elif '<' in input_str:  # Ajout de la gestion de '<'
            parts = input_str.split('<')
            command = parts[0].strip()
            redirect_from_file = parts[1].strip()
            try:
                with open(redirect_from_file, 'rb') as f:
                    file_data = f.read()
                    send_message(sock, command.encode() + file_data)  # Envoie le contenu du fichier au serveur
                lines.append(('info', f"Message envoyé depuis {redirect_from_file}"))
            except Exception as e:
                lines.append(('error', f"Erreur de lecture du fichier : {e}"))
        else:
            send_message(sock, input_str.encode())
        lines.append(('sent', input_str))
        input_history.append(input_str)
        input_history_index = len(input_history)
        input_str = ""

def navigate_input_history(direction):
    """Navigue dans l'historique des saisies."""
    global input_str, input_history_index
    new_index = input_history_index + direction

    if 0 <= new_index < len(input_history):
        input_history_index = new_index
        input_str = input_history[input_history_index]
    elif new_index == len(input_history):
        input_str = ""

def signal_handler(sig, frame):
    """Gère la fermeture propre du programme."""
    print("\nExiting gracefully...")
    sys.exit(0)

def main(stdscr, server_ip):
    """Point d'entrée principal."""
    global input_str, redirect_to_file, scroll_offset, input_history, input_history_index

    init_curses()
    sock = setup_socket(server_ip)
    if sock is None:
        draw_interface(stdscr, force_update=True)
        return

    draw_interface(stdscr, force_update=True)
    stdscr.nodelay(1)

    while True:
        draw_interface(stdscr)
        readable, _, _ = select.select([sock], [], [], 0.1)

        if sock in readable:
            message = receive_message(sock)
            if message is None:
                lines.append(('error', "Connexion au serveur perdue."))
                break

            if redirect_to_file:
                with open(redirect_to_file, 'wb') as f:
                    f.write(message)
                lines.append(('info', f"Message redirigé vers {redirect_to_file}"))
                redirect_to_file = None
            else:
                for line in message.decode('utf-8', errors='ignore').replace('\0', '').split('\n'):
                    lines.append(('received', line))

            draw_interface(stdscr, force_update=True)

        key = stdscr.getch()
        if not handle_input_key(key, stdscr, sock):
            break

if __name__ == "__main__":
    signal.signal(signal.SIGINT, signal_handler)

    parser = argparse.ArgumentParser(description="Part2 Client")
    parser.add_argument("server_ip", help="Adresse IP du serveur auquel se connecter")
    args = parser.parse_args()

    SERVER_IP = args.server_ip
    curses.wrapper(main, SERVER_IP)
