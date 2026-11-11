#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_VARS 256
#define MAX_LINE 512
#define MAX_LINES 1000
#define MAX_FUNCS 64

typedef struct {
    char name[64];
    int value;
} Symbol;

typedef struct {
    Symbol vars[MAX_VARS];
    int count;
} Env;

typedef struct {
    char name[64];
    char arg_name[64];
    int start_pc;
    int end_pc;
} Function;

Function functions[MAX_FUNCS];
int func_count = 0;

void env_set(Env *env, const char *name, int val) {
    for (int i = 0; i < env->count; i++) {
        if (strcmp(env->vars[i].name, name) == 0) {
            env->vars[i].value = val;
            return;
        }
    }
    if (env->count < MAX_VARS) {
        strncpy(env->vars[env->count].name, name, 63);
        env->vars[env->count].name[63] = '\0';
        env->vars[env->count].value = val;
        env->count++;
    }
}

int env_get(Env *env, const char *name, int *val) {
    if (isdigit((unsigned char)name[0]) || (name[0] == '-' && isdigit((unsigned char)name[1]))) {
        *val = atoi(name);
        return 1;
    }
    for (int i = 0; i < env->count; i++) {
        if (strcmp(env->vars[i].name, name) == 0) {
            *val = env->vars[i].value;
            return 1;
        }
    }
    *val = 0;
    return 0;
}

int eval_expr(Env *env, const char *expr) {
    char var1[64], op[8], var2[64];
    int v1 = 0, v2 = 0;

    if (sscanf(expr, "%s %s %s", var1, op, var2) == 3) {
        env_get(env, var1, &v1);
        env_get(env, var2, &v2);
        if (strcmp(op, "+") == 0) return v1 + v2;
        if (strcmp(op, "-") == 0) return v1 - v2;
        if (strcmp(op, "*") == 0) return v1 * v2;
        if (strcmp(op, "/") == 0) return v2 != 0 ? v1 / v2 : 0;
        if (strcmp(op, "%") == 0) return v2 != 0 ? v1 % v2 : 0;
    } else {
        env_get(env, expr, &v1);
        return v1;
    }
    return 0;
}

// Déclaration anticipée
void run_script(Env *env, char lines[MAX_LINES][MAX_LINE], int *pc, int total_lines, int *return_flag, int *return_val);

void execute_call(Env *env, char lines[MAX_LINES][MAX_LINE], int total_lines, const char *func_name, const char *arg_expr, int *dest_val) {
    // Fonctions natives intégrées
    if (strcmp(func_name, "rand") == 0) {
        int max = eval_expr(env, arg_expr);
        *dest_val = (max > 0) ? (rand() % max) : 0;
        return;
    }
    if (strcmp(func_name, "abs") == 0) {
        int val = eval_expr(env, arg_expr);
        *dest_val = (val < 0) ? -val : val;
        return;
    }

    // Fonctions utilisateur
    for (int i = 0; i < func_count; i++) {
        if (strcmp(functions[i].name, func_name) == 0) {
            Env local_env = *env; // Copie de l'environnement pour l'isolation
            int arg_val = eval_expr(env, arg_expr);
            env_set(&local_env, functions[i].arg_name, arg_val);

            int pc = functions[i].start_pc;
            int ret_flag = 0;
            int ret_val = 0;

            run_script(&local_env, lines, &pc, functions[i].end_pc, &ret_flag, &ret_val);
            *dest_val = ret_val;
            return;
        }
    }
    printf("Erreur : fonction inconnue « %s »\n", func_name);
}

void run_script(Env *env, char lines[MAX_LINES][MAX_LINE], int *pc, int total_lines, int *return_flag, int *return_val) {
    while (*pc < total_lines) {
        char line[MAX_LINE];
        strncpy(line, lines[*pc], MAX_LINE - 1);
        line[MAX_LINE - 1] = '\0';

        char *start = line;
        while (*start && isspace((unsigned char)*start)) start++;
        if (*start == '\0' || *start == '#') {
            (*pc)++;
            continue;
        }

        if (strcmp(start, "end") == 0) {
            return;
        }

        if (strncmp(start, "return ", 7) == 0) {
            *return_val = eval_expr(env, start + 7);
            *return_flag = 1;
            return;
        }

        if (strncmp(start, "print ", 6) == 0) {
            char *arg = start + 6;
            while (*arg && isspace((unsigned char)*arg)) arg++;
            if (arg[0] == '"') {
                char *end = strchr(arg + 1, '"');
                if (end) *end = '\0';
                printf("%s\n", arg + 1);
            } else {
                int val = eval_expr(env, arg);
                printf("%d\n", val);
            }
        }
        else if (strncmp(start, "input ", 6) == 0) {
            char prompt[128], var_name[64];
            if (sscanf(start + 6, "\"%[^\"]\", %s", prompt, var_name) == 2) {
                printf("%s", prompt);
                int user_val;
                if (scanf("%d", &user_val) == 1) {
                    env_set(env, var_name, user_val);
                }
                while (getchar() != '\n');
            }
        }
        else if (strncmp(start, "if ", 3) == 0) {
            char var1[64], op[8], var2[64];
            sscanf(start + 3, "%s %s %s", var1, op, var2);
            int v1, v2;
            env_get(env, var1, &v1);
            env_get(env, var2, &v2);

            int condition = 0;
            if (strcmp(op, "==") == 0) condition = (v1 == v2);
            else if (strcmp(op, "!=") == 0) condition = (v1 != v2);
            else if (strcmp(op, ">") == 0) condition = (v1 > v2);
            else if (strcmp(op, "<") == 0) condition = (v1 < v2);

            (*pc)++;
            if (condition) {
                run_script(env, lines, pc, total_lines, return_flag, return_val);
                if (*return_flag) return;
            } else {
                int depth = 1;
                while (*pc < total_lines && depth > 0) {
                    char *l = lines[*pc];
                    while (*l && isspace((unsigned char)*l)) l++;
                    if (strncmp(l, "if ", 3) == 0 || strncmp(l, "while ", 6) == 0 || strncmp(l, "func ", 5) == 0) depth++;
                    if (strcmp(l, "end") == 0) depth--;
                    (*pc)++;
                }
                continue;
            }
        }
        else if (strncmp(start, "while ", 6) == 0) {
            char var1[64], op[8], var2[64];
            sscanf(start + 6, "%s %s %s", var1, op, var2);
            int loop_pc = *pc;
            (*pc)++;
            int block_start = *pc;

            while (1) {
                int v1, v2;
                env_get(env, var1, &v1);
                env_get(env, var2, &v2);

                int condition = 0;
                if (strcmp(op, "!=") == 0) condition = (v1 != v2);
                else if (strcmp(op, "==") == 0) condition = (v1 == v2);
                else if (strcmp(op, "<") == 0) condition = (v1 < v2);
                else if (strcmp(op, ">") == 0) condition = (v1 > v2);

                if (!condition) break;

                *pc = block_start;
                run_script(env, lines, pc, total_lines, return_flag, return_val);
                if (*return_flag) return;
            }

            int depth = 1;
            *pc = loop_pc;
            while (*pc < total_lines && depth > 0) {
                char *l = lines[*pc];
                while (*l && isspace((unsigned char)*l)) l++;
                if (strncmp(l, "while ", 6) == 0 || strncmp(l, "if ", 3) == 0 || strncmp(l, "func ", 5) == 0) depth++;
                if (strcmp(l, "end") == 0) depth--;
                (*pc)++;
            }
            continue;
        }
        else {
            char var_name[64], call_func[64], call_arg[128], expr[256];
            // Appel de fonction avec assignation : res = carre(5) ou res = rand(10)
            if (sscanf(start, "%63s = %63[a-zA-Z_](%127[^)])", var_name, call_func, call_arg) == 3) {
                int val = 0;
                execute_call(env, lines, total_lines, call_func, call_arg, &val);
                env_set(env, var_name, val);
            }
            // Affectation standard : x = 10
            else if (sscanf(start, "%63s = %[^\n]", var_name, expr) == 2) {
                int val = eval_expr(env, expr);
                env_set(env, var_name, val);
            }
        }
        (*pc)++;
    }
}

void load_and_run(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("Erreur : impossible de charger l'application %s\n", filename);
        return;
    }

    char lines[MAX_LINES][MAX_LINE];
    int total_lines = 0;
    while (fgets(lines[total_lines], MAX_LINE, f)) {
        lines[total_lines][strcspn(lines[total_lines], "\r\n")] = '\0';
        total_lines++;
    }
    fclose(f);

    // Première passe : indexer les fonctions du fichier .ct
    for (int i = 0; i < total_lines; i++) {
        char *l = lines[i];
        while (*l && isspace((unsigned char)*l)) l++;
        if (strncmp(l, "func ", 5) == 0) {
            char fname[64], farg[64];
            if (sscanf(l + 5, "%63[a-zA-Z_](%63[a-zA-Z_])", fname, farg) == 2) {
                strcpy(functions[func_count].name, fname);
                strcpy(functions[func_count].arg_name, farg);
                functions[func_count].start_pc = i + 1;
                
                // Trouver la fin de la fonction
                int depth = 1;
                int j = i + 1;
                while (j < total_lines && depth > 0) {
                    char *sub = lines[j];
                    while (*sub && isspace((unsigned char)*sub)) sub++;
                    if (strncmp(sub, "func ", 5) == 0 || strncmp(sub, "if ", 3) == 0 || strncmp(sub, "while ", 6) == 0) depth++;
                    if (strcmp(sub, "end") == 0) depth--;
                    j++;
                }
                functions[func_count].end_pc = j - 1;
                func_count++;
            }
        }
    }

    // Deuxième passe : exécuter le code hors fonctions (ou lancer le point d'entrée)
    Env env = { .count = 0 };
    int pc = 0;
    int return_flag = 0;
    int return_val = 0;
    
    // Sauter les définitions de fonctions au démarrage global
    while (pc < total_lines) {
        char *l = lines[pc];
        while (*l && isspace((unsigned char)*l)) l++;
        if (strncmp(l, "func ", 5) == 0) {
            int depth = 1;
            pc++;
            while (pc < total_lines && depth > 0) {
                char *sub = lines[pc];
                while (*sub && isspace((unsigned char)*sub)) sub++;
                if (strncmp(sub, "func ", 5) == 0 || strncmp(sub, "if ", 3) == 0 || strncmp(sub, "while ", 6) == 0) depth++;
                if (strcmp(sub, "end") == 0) depth--;
                pc++;
            }
            continue;
        }
        run_script(&env, lines, &pc, total_lines, &return_flag, &return_val);
    }
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    if (argc < 2) {
        printf("Usage : ./citron <app.ct>\n");
        return 1;
    }
    load_and_run(argv[1]);
    return 0;
}