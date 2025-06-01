// Kokoro Compiler - Level 0 with Symbol Table
// By Kuroshio Industrial Logic Systems

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define TARGET_6502 1
#define TARGET_MEGA6502 2
int TARGET = TARGET_6502;

#define MAX_LINE 256
#define MAX_SYMBOLS 256
#define MAX_NAME 32
#define START_ADDR 0x0200

typedef struct {
    char name[MAX_NAME];
    int address;   // base address for array, scalar address otherwise
    int size;      // 1 for scalar, N for array
    int is_array;  // 1 if array, 0 otherwise
} Symbol;

Symbol symbols[MAX_SYMBOLS];
int symbol_count = 0;
int next_address = START_ADDR;

// --- Symbol Table Management ---
int get_var_address(const char *name, int size, int is_array) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbols[i].name, name) == 0) {
            return symbols[i].address;
        }
    }
    int addr = next_address;
    strncpy(symbols[symbol_count].name, name, MAX_NAME-1);
    symbols[symbol_count].address = addr;
    symbols[symbol_count].size = size > 0 ? size : 1;
    symbols[symbol_count].is_array = is_array;
    symbol_count++;
    next_address += size > 0 ? size : 1;
    return addr;
}

void print_memory_map() {
    printf("Kokoro Variable Memory Map:\n");
    for (int i = 0; i < symbol_count; i++) {
        printf("  %-16s @ $%04X (%s, %d byte%s)\n", symbols[i].name, symbols[i].address,
               symbols[i].is_array ? "array" : "scalar",
               symbols[i].size, symbols[i].size > 1 ? "s" : "");
    }
}

// --- Function declarations ---
void handle_store(char *line, FILE *output);
void handle_print(char *line, FILE *output);
void handle_call(char *line, FILE *output);
void handle_bookmark(char *line, FILE *output);
void handle_goto(char *line, FILE *output);
void handle_if(char *line, FILE *output);
void handle_unknown(char *line, FILE *output);
void math_eval(char *expr, char *result);
void emit_multiply(char *left, char *right, FILE *output);
void emit_divide(char *left, char *right, FILE *output);

// --- Main ---
int main(int argc, char *argv[])
{
    if (argc < 3) {
        printf("Usage: kokoro input.kokoro output.asm\n");
        return 1;
    }

    FILE *input = fopen(argv[1], "r");
    if (!input) {
        perror("Error opening input file");
        return 1;
    }

    FILE *output = fopen(argv[2], "w");
    if (!output) {
        perror("Error opening output file");
        fclose(input);
        return 1;
    }

    char line[MAX_LINE];

    while (fgets(line, MAX_LINE, input)) {
        char *start = line;
        while (isspace(*start)) start++;
        if (*start == '\0' || *start == '#')
            continue;

        char keyword[32];
        sscanf(start, "%31s", keyword);
        for (char *p = keyword; *p; ++p) *p = tolower(*p);

        if (strcmp(keyword, "store") == 0) {
            handle_store(start, output);
        } else if (strcmp(keyword, "print") == 0) {
            handle_print(start, output);
        } else if (strcmp(keyword, "call") == 0) {
            handle_call(start, output);
        } else if (strcmp(keyword, "bookmark") == 0) {
            handle_bookmark(start, output);
        } else if (strcmp(keyword, "goto") == 0) {
            handle_goto(start, output);
        } else if (strcmp(keyword, "if") == 0) {
            handle_if(start, output);
        } else {
            handle_unknown(start, output);
        }
    }

    fclose(input);
    fclose(output);

    print_memory_map();

    printf("Kokoro compile complete.\n");
    return 0;
}

// --- Handlers ---

void handle_store(char *line, FILE *output)
{
    // Bulk array assignment
    if (strstr(line, ",") && strstr(line, "as array")) {
        char values[256], var[32];
        sscanf(line, "store %255[^i] in %31s as array", values, var);

        char *token = strtok(values, ",");
        int idx = 0, arr_size = 0;
        char *tokens[128];
        while (token) {
            while (isspace(*token)) token++;
            tokens[arr_size++] = token;
            token = strtok(NULL, ",");
        }
        int base_addr = get_var_address(var, arr_size, 1);

        for (idx = 0; idx < arr_size; idx++) {
            fprintf(output, "LDA #%s\nSTA $%04X\n", tokens[idx], base_addr + idx);
        }
        fprintf(output, "\n");
    }
    // Array element assignment (literal index only for now)
    else if (strstr(line, "[") && strstr(line, "]")) {
        char value[32], var[32], idx_str[32];
        sscanf(line, "store %31s in %31[^[][%31[^]] as number", value, var, idx_str);

        int base_addr = get_var_address(var, 0, 1);
        int idx = atoi(idx_str);
        fprintf(output, "LDA #%s\nSTA $%04X\n\n", value, base_addr + idx);
    }
    // Scalar variable assignment
    else {
        char expr[64], var[32], type[32];
        sscanf(line, "store %63[^i] in %31s as %31s", expr, var, type);

        char *end = expr + strlen(expr) - 1;
        while (end > expr && isspace(*end)) *end-- = '\0';

        char result[64];
        math_eval(expr, result);

        int addr = get_var_address(var, 1, 0);
        fprintf(output, "LDA #%s\n", result);
        fprintf(output, "STA $%04X\n\n", addr);
    }
}

void handle_print(char *line, FILE *output)
{
    char what[64];
    int n = sscanf(line, "print %63[^\n]", what);

    int addr = get_var_address(what, 1, 0); // Ensure symbol exists/get addr
    fprintf(output, "LDA $%04X\n", addr);
    fprintf(output, "JSR PrintDigit\n\n"); // Assume PrintDigit prints value in A to screen
}

void handle_call(char *line, FILE *output)
{
    char func[32];
    int n = sscanf(line, "call %31s", func);
    fprintf(output, "JSR %s\n\n", func);
}

void handle_bookmark(char *line, FILE *output)
{
    char name[32];
    int n = sscanf(line, "bookmark %31s", name);
    fprintf(output, "%s:\n\n", name);
}

void handle_goto(char *line, FILE *output)
{
    char name[32];
    int n = sscanf(line, "goto %31s", name);
    fprintf(output, "JMP %s\n\n", name);
}

void handle_if(char *line, FILE *output)
{
    // Example: IF x IS GREATER_THAN y DO loop_start
    char var1[32], var2[32], cmp[32], label[32];
    int n = sscanf(line, "if %31s is %31s %31s do %31s", var1, cmp, var2, label);

    int addr1 = get_var_address(var1, 1, 0);
    int addr2 = get_var_address(var2, 1, 0);

    fprintf(output, "LDA $%04X\n", addr1);
    fprintf(output, "CMP $%04X\n", addr2);

    if (strcasecmp(cmp, "greater_than") == 0) {
        fprintf(output, "BCC skip_%s\n", label);
        fprintf(output, "JMP %s\n", label);
        fprintf(output, "skip_%s:\n\n", label);
    } else if (strcasecmp(cmp, "less_than") == 0) {
        fprintf(output, "BCS skip_%s\n", label);
        fprintf(output, "JMP %s\n", label);
        fprintf(output, "skip_%s:\n\n", label);
    } else if (strcasecmp(cmp, "equal") == 0) {
        fprintf(output, "BEQ %s\n\n", label);
    } else if (strcasecmp(cmp, "not_equal") == 0) {
        fprintf(output, "BNE %s\n\n", label);
    } else {
        fprintf(output, "; Unsupported comparison: %s\n\n", cmp);
    }
}

void handle_unknown(char *line, FILE *output)
{
    fprintf(output, "; Unknown line: %s\n\n", line);
}

void math_eval(char *expr, char *result)
{
    // For now, just pass through constant or variable (can expand later)
    strcpy(result, expr);
}

void emit_multiply(char *left, char *right, FILE *output)
{
    if (TARGET == TARGET_6502) {
        fprintf(output, "; 6502 multiply: JSR Multiply (args: %s, %s)\n", left, right);
    } else if (TARGET == TARGET_MEGA6502) {
        fprintf(output, "MUL %s, %s\n", left, right);
    }
}

void emit_divide(char *left, char *right, FILE *output)
{
    if (TARGET == TARGET_6502) {
        fprintf(output, "; 6502 divide: JSR Divide (args: %s, %s)\n", left, right);
    } else if (TARGET == TARGET_MEGA6502) {
        fprintf(output, "DIV %s, %s\n", left, right);
    }
}
