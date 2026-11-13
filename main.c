#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "runtime.h"

int main(int argc, char *argv[]) {
    char *nom = strrchr(argv[0], '\\');
    if (!nom) nom = strrchr(argv[0], '/');
    nom = nom ? nom + 1 : argv[0];

    int is_runner = (strstr(nom, "run") != NULL) || (argc >= 2);

    if (is_runner) {
        if (argc < 2) {
            printf("Erreur : Aucun script specifie.\n");
            printf("Usage : citron-run.exe <script.ct>\n");
            printf("\nAppuie sur Entree pour quitter...");
            getchar();
            return 1;
        }

        load_and_run(argv[1]);
        return 0;
    }

    Env env = { .count = 0 };
    char line[MAX_LINE];

    printf("Citron REPL v1.0\n");
    printf("Tape « exit » pour quitter.\n\n");

    while (1) {
        printf("citron> ");
        if (!fgets(line, sizeof(line), stdin)) break;

        line[strcspn(line, "\r\n")] = '\0';
        if (strcmp(line, "exit") == 0) break;
        if (strlen(line) == 0) continue;

        execute_line(&env, line);
    }

    return 0;
}