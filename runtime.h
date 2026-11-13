#ifndef RUNTIME_H
#define RUNTIME_H

#define MAX_VARS 256
#define MAX_LINE 512

typedef struct {
    char name[64];
    int value;
} Symbol;

typedef struct {
    Symbol vars[MAX_VARS];
    int count;
} Env;

void execute_line(Env *env, const char *line);
void load_and_run(const char *filename);

#endif