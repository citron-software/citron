#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <SDL2/SDL.h>
#include "runtime.h"

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static int running_game = 1;
static int last_key = 0;

static Uint8 draw_r = 50, draw_g = 205, draw_b = 50;

static void citron_init(int w, int h) {
    if (window) return; // Empêche de recréer la fenêtre si elle existe déjà
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return;
    window = SDL_CreateWindow("Citron Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
}

static void env_set(Env *env, const char *name, int val) {
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

static int env_get(Env *env, const char *name, int *val) {
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

static int eval_expr(Env *env, const char *expr) {
    char var1[64], op[8], var2[64];
    int v1 = 0, v2 = 0;

    if (sscanf(expr, "%63s %7s %63s", var1, op, var2) == 3) {
        env_get(env, var1, &v1);
        env_get(env, var2, &v2);
        if (strcmp(op, "+") == 0) return v1 + v2;
        if (strcmp(op, "-") == 0) return v1 - v2;
        if (strcmp(op, "*") == 0) return v1 * v2;
        if (strcmp(op, "/") == 0) return v2 != 0 ? v1 / v2 : 0;
        if (strcmp(op, "==") == 0) return v1 == v2;
        if (strcmp(op, "!=") == 0) return v1 != v2;
        if (strcmp(op, "<") == 0) return v1 < v2;
        if (strcmp(op, ">") == 0) return v1 > v2;
    } else {
        env_get(env, expr, &v1);
        return v1;
    }
    return 0;
}

void execute_line(Env *env, const char *line) {
    char p[MAX_LINE];
    strncpy(p, line, MAX_LINE - 1);
    p[MAX_LINE - 1] = '\0';

    char *start = p;
    while (*start && isspace((unsigned char)*start)) start++;
    if (*start == '\0' || *start == '#') return;

    if (strncmp(start, "window ", 7) == 0) {
        char sw[32], sh[32];
        if (sscanf(start + 7, "%31s %31s", sw, sh) == 2) {
            int w = 0, h = 0;
            env_get(env, sw, &w);
            env_get(env, sh, &h);
            citron_init(w, h);
        }
        return;
    }

    if (strcmp(start, "clear") == 0) {
        if (renderer) {
            SDL_SetRenderDrawColor(renderer, 15, 15, 25, 255);
            SDL_RenderClear(renderer);
        }
        return;
    }

    if (strcmp(start, "present") == 0) {
        if (renderer) {
            SDL_RenderPresent(renderer);
        }
        return;
    }

    if (strncmp(start, "color ", 6) == 0) {
        char sr[32], sg[32], sb[32];
        if (sscanf(start + 6, "%31s %31s %31s", sr, sg, sb) == 3) {
            int r = 0, g = 0, b = 0;
            env_get(env, sr, &r);
            env_get(env, sg, &g);
            env_get(env, sb, &b);
            draw_r = (Uint8)r;
            draw_g = (Uint8)g;
            draw_b = (Uint8)b;
        }
        return;
    }

    if (strncmp(start, "rect ", 5) == 0) {
        char sx[32], sy[32], sw[32], sh[32];
        if (sscanf(start + 5, "%31s %31s %31s %31s", sx, sy, sw, sh) == 4) {
            int x = 0, y = 0, w = 0, h = 0;
            env_get(env, sx, &x);
            env_get(env, sy, &y);
            env_get(env, sw, &w);
            env_get(env, sh, &h);
            if (renderer) {
                SDL_SetRenderDrawColor(renderer, draw_r, draw_g, draw_b, 255);
                SDL_Rect r = { x, y, w, h };
                SDL_RenderFillRect(renderer, &r);
            }
        }
        return;
    }

    if (strcmp(start, "poll_events") == 0) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running_game = 0;
            }
            if (e.type == SDL_KEYDOWN) {
                last_key = e.key.keysym.sym;
            }
            if (e.type == SDL_KEYUP) {
                if (e.key.keysym.sym == last_key) {
                    last_key = 0;
                }
            }
        }
        env_set(env, "key", last_key);
        env_set(env, "running", running_game);
        return;
    }

    if (strncmp(start, "delay ", 6) == 0) {
        char sms[32];
        if (sscanf(start + 6, "%31s", sms) == 1) {
            int ms = 0;
            env_get(env, sms, &ms);
            SDL_Delay((Uint32)ms);
        }
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
        return;
    }

    char var_name[64], expr[256];
    if (sscanf(start, "%63s = %[^\n]", var_name, expr) == 2) {
        int val = eval_expr(env, expr);
        env_set(env, var_name, val);
        return;
    }
}

void load_and_run(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("Erreur : impossible d'ouvrir le fichier %s\n", filename);
        return;
    }

    char lines[512][MAX_LINE];
    int line_count = 0;
    while (fgets(lines[line_count], MAX_LINE, f) && line_count < 512) {
        lines[line_count][strcspn(lines[line_count], "\r\n")] = '\0';
        line_count++;
    }
    fclose(f);

    Env env = { .count = 0 };
    running_game = 1;
    env_set(&env, "running", 1);
    // Codes touches SDL standard (Z/Q/S/D ou flèches)
    env_set(&env, "KEY_UP", SDLK_UP);
    env_set(&env, "KEY_DOWN", SDLK_DOWN);
    env_set(&env, "KEY_LEFT", SDLK_LEFT);
    env_set(&env, "KEY_RIGHT", SDLK_RIGHT);
    env_set(&env, "KEY_ESC", SDLK_ESCAPE);

    int loop_start = -1;
    int i = 0;

    while (i < line_count && running_game) {
        char *start = lines[i];
        while (*start && isspace((unsigned char)*start)) start++;

        if (strcmp(start, "loop") == 0) {
            loop_start = i;
            i++;
            continue;
        }

        if (strcmp(start, "endloop") == 0) {
            if (loop_start != -1 && running_game) {
                i = loop_start + 1;
                continue;
            }
        }

        if (strncmp(start, "if ", 3) == 0) {
            int cond = eval_expr(&env, start + 3);
            if (!cond) {
                // Sauter jusqu'au endif correspondant
                int depth = 1;
                i++;
                while (i < line_count && depth > 0) {
                    char *s = lines[i];
                    while (*s && isspace((unsigned char)*s)) s++;
                    if (strncmp(s, "if ", 3) == 0) depth++;
                    if (strcmp(s, "endif") == 0) depth--;
                    i++;
                }
                continue;
            }
            i++;
            continue;
        }

        if (strcmp(start, "endif") == 0) {
            i++;
            continue;
        }

        execute_line(&env, start);
        i++;
    }

    if (window) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        window = NULL;
        renderer = NULL;
    }
}