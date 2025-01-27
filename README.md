# Workshop Buffer overflow

### Ou plutôt : Comment s'introduire dans l'ordinateur de votre voisin uniquement grâce à une erreur de "segmentation fault" ?

En C, une segfault peut survenir quand on écrit au delà de la taille d'un tableau.

### Par exemple :
`char buffer[64];`

Si on rempli ce tableau avec 90 'A' alors que la taille du tableau est 64...
On obtiens le fameux:
`Segmentation fault (core dumped)`

### Pourquoi est-ce que ça se produit vraiment ?

Non, ce n'est pas parce qu'on n'est pas "autorisé" à écrire + de 64 octets.

En réalité, en dépassant du tableau, on vient juste d'écrire par dessus des zones mémoires qui contiennent l'adresse de la prochaine instruction à exécuter.

L'ordinateur tente de trouver une fonction dont l'adresse est "AAAA" sauf qu'elle n'existe pas.

Et si on remplaçait ce texte par une véritable adresse de fonction ?

C'est tout le sujet de la partie 1 !


## Pré-requis:

- Avoir installé: python3, gdb, objdump

- Désactiver les protections [ASLR](https://fr.wikipedia.org/wiki/Address_space_layout_randomization#:~:text=L'address%20space%20layout%20randomization,la%20pile%20et%20des%20biblioth%C3%A8ques.) pour faciliter l'exploitation des vulnérabilités:
> (On réactivera cette protection lorsque vous maîtriserez les bases des buffer overflow, plusieurs méthodes existent pour contourner l'ASLR.)
```sh
# Désactiver l'ASLR:
sudo bash -c 'echo "kernel.randomize_va_space = 0" >> /etc/sysctl.conf'
sudo sysctl -p
cat /proc/sys/kernel/randomize_va_space
# Vérifiez que vous obtenez "0"

ulimit -c unlimited
ulimit -c
# Vérifiez que vous obtenez "unlimited"
```

# Partie 1

## Exercice 1: Apprendre à utiliser GDB pour le hacking.

Compilez le fichier `part1.c` avec:
```sh
gcc -m32 -fno-stack-protector -z execstack -no-pie -o part1 part1.c
```

- Ce fichier est vulnérable aux buffer overflow.
- Il existe une fonction `secret_function` à l'intérieur de ce programme qui est bien déclaré mais pourtant elle n'est jamais appelée.

L'objectif est de parvenir à exécuter `secret_function` grâce à la vulnérabilité buffer overflow.

### Étapes:

1. Lancez `gdb ./part1`

2. Dans l'interface de gdb, lancez le commande `disas vulnerable_function` pour désassembler la fonction.

3. Cherchez l'adresse de la dernière instruction de cette fonction : `ret`. Les adresses sont des valeurs hexadécimales se trouvant dans la colonne de gauche. (N'hésitez pas à appuyer sur [ENTRER] pour afficher + de lignes.)

4. Utilisez l'adresse trouvée dans la commande `break *<ADRESSE DE RET>`. Exemple: `break *0x8049200`. Cela va créer un "checkpoint", on pourra analyser l'état de la mémoire avant que cette fonction se termine.

5. Utilisez ces commandes pour envoyer un input au programme, et analyser les registres entre temps:
```sh
info registers

run < <(python3 -c 'import sys; sys.stdout.buffer.write(b"A" * 120)')

info registers
```

### Pourquoi faut-il utiliser `sys.stdout.buffer.write()` au lieu de `print()` ?

La fonction `print()` encode certains caractères non-printable, mais dans notre cas, nous voulont qu'aucune modification soit appliquée !

- Vous remarquerez certains registres tels que "ebx", "ebp" et "eip" sont remplacés par des `0x41414141`. Entre temps, nous avons dépassé le tableau avec 120 fois le caractère 'A'. 

> Dans la table ASCII: `41` = 'A'

- Vous pouvez utiliser la commande `continue` pour avancer après un checkpoint.

## Exercice 2: Exploiter la buffer overflow !

1. Notre objectif est de savoir combien de 'A' il nous faut avant que le registre "eip" soit remplacé par `0x41414141` : Continuez à utiliser `run < <(python3 -c 'import sys; sys.stdout.buffer.write(b"A" * <LONGUEUR DE L'INPUT>)')` et `info registers` pour faire varier la longueur de l'input envoyé.

2. Une fois que vous avez ce nombre, utilisez `info functions` pour retrouver l'adresse de **secret_function**.

3. Vous devez noter l'adresse quelque part. En admettant que l'adresse trouvée est : 0x08049196, vous devez l'écrire à l'envers et sous cette forme : `\x96\x91\x04\x08`.

> On écrit l'adresse à l'envers pour respecter le format **Little endian**.

3. Finalement, utilisez:
```sh
run < <(python3 -c 'import sys; sys.stdout.buffer.write(b"A" * <DÉCALAGE EIP> + b"<ADRESSE EIP>")')
```

4. Le message :
```
Bravo ! Vous avez exécuté la fonction secrète !
```
Devrait maintenant s'afficher même si ce message est suivi d'une segfault !

C'est normal de toujours avoir l'erreur "segmentation fault" (après le message), car à ce stade, on a tout de même potentiellement écrit par dessus plusieurs autres registres avant d'arriver à EIP.

Ces autres registres sont aussi potentiellement utilisés après l'EIP et causent à nouveau un crash, mais l'objectif de détourner le flux d'exécution du programme a quand même été atteint !

# Partie 2

## Exercice 1

L'objectif est d'injecter le code qu'on souhaite dans un programme vulnérable.

Cela va nécessiter de créer un shellcode. Un shellcode est une suite d'instructions en langage machine.

1. Compilez `part2.c` avec:
```sh
gcc -m32 -fno-stack-protector -z execstack -no-pie -o part2 part2.c
```

2. Afin d'injecter le shell code, on pourra se baser sur cette vidéo tutoriel proposé par LiveOverflow :
https://www.youtube.com/watch?v=HSlhY4Uy8SA

# Partie 3

## Exercice 1:

L'objectif est d'exploiter une buffer overflow à distance. Cette fois-ci, la buffer overflow est présente sur un serveur `part3_server.c` qu'on compilera avec:
```sh
gcc -m32 -fno-stack-protector -z execstack -no-pie -o part3_server part3_server.c
```

On vous fourni également un client pour se connecter au serveur: `part3_client.py`.
```sh
python3 ./part3_client.py 127.0.0.1
```

Liste des failles à exploiter:
- Une vulnérabilité "leak memory" dans la commande `/inbox view` cela vous permet d'identifier facilement l'adresse de la fonction `handle_admin_panel` qui va vous octroyer un shell bash à distance !

- Une vulnérabilité buffer overflow dans la commande `/inbox send` également.

### Comment utiliser le client `part3_client.py` ?

Afin d'envoyer et de recevoir des données binaire (nécessaire pour exploiter les vulnérabilités), le client met à disposition des commandes de redirection `>` et `<` qui fonctionnent comme dans un environnement bash.
Voici des exemples:

```sh
/inbox view 30 > output.txt
/inbox send 42 < input.txt
```

Pour trouver la vulnérabilité de fuite mémoire, essayez des nombres un peu plus grand que 30 dans `/inbox view 30 > output.txt`.

Certaines valeurs sont "non-printable", mais on peut utiliser les redirections ainsi que la commande `xxd output.txt` afin d'afficher les valeurs hexadécimales en brut.

## Étapes:

1. Lire suffisamment de données à partir du leak mémoire et identifier la taille maximale du buffer.

2. Identifier le décalage de l'EIP (l'offset).

3. Écrire par dessus le buffer grâce à la commande `/inbox send` pour écraser l'EIP et injecter son shellcode ou appeler `handle_admin_panel`.

## Exercice 2:

L'objectif final de cette dernière partie est de parvenir à exploiter cette même faille mais silencieusement, sans aucun crash côté serveur.

1. Sauvegardez les registres qui ont fuité via la faille de leak mémoire avant d'écraser l'EIP.

2. Utilisez ces adresses de registres afin d'éviter de corrompre ces mêmes registres lorsque vous remplacez la valeur de l'EIP.

3. Si le crash persiste, n'hésitez pas à faire usage des "NOP" (https://en.wikipedia.org/wiki/NOP_slide)
