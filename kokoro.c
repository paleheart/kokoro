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

#ifdef _MSC_VER
    #define strcasecmp _stricmp
#endif


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

int is_constant(char *s)
{
    // Simple: is it all digits? Then it's a constant.
    for (int i = 0; s[i]; i++) {
        if (!isdigit((unsigned char)s[i])) return 0;
    }
    return 1;
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
void handle_if(char *line, FILE *output, FILE *input);
void handle_unknown(char *line, FILE *output);
void math_eval(char *expr, char *result, FILE *output);
void emit_multiply(char *left, char *right, FILE *output);
void emit_divide(char *left, char *right, FILE *output);
void trim_cr(char *s);

void trim_cr(char *s) {
    char *cr = strchr(s, '\r');
    if (cr) *cr = '\0';
}

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

    // Skip leading whitespace
    while (isspace((unsigned char)*start)) start++;

    // Strip trailing CR if present
    char *cr = strchr(start, '\r');
    if (cr) *cr = '\0';



    // Skip blank lines
    if (*start == '\0')
        continue;

    // Skip full-line comments
    if (*start == '#')
        continue;

    // Now parse the line as normal
    char keyword[32];
    sscanf(start, "%31s", keyword);
    printf("DEBUG: Processing line: '%s'\n", start);
    // Lowercase the whole line for consistent parsing
    for (char *p = start; *p; ++p) *p = tolower((unsigned char)*p);

    for (char *p = keyword; *p; ++p) *p = tolower((unsigned char)*p);

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
            handle_if(start, output, input);
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

  
        trim_cr(var);


        char *token = strtok(values, ",");
        int idx = 0, arr_size = 0;
        char *tokens[128];
        while (token) {
            while (isspace((unsigned char)*token)) token++;
            tokens[arr_size++] = token;
            token = strtok(NULL, ",");
        }
        int base_addr = get_var_address(var, arr_size, 1);

        for (idx = 0; idx < arr_size; idx++) {
            fprintf(output, "LDA #%s\nSTA $%04X\n", tokens[idx], base_addr + idx);
        }
        fprintf(output, "\n");
    }
    // New: Array element access with "T'S ARRAY VALUE N"
    else if (strstr(line, "'s array value")) {
        char src_var[32], index_str[32], dest_var[32];

        // Example: store t's array value 3 in e as number
        sscanf(line, "store %31[^']'s array value %31[^i] in %31s as number", src_var, index_str, dest_var);

        trim_cr(src_var);
        trim_cr(index_str);
        trim_cr(dest_var);

        int base_addr = get_var_address(src_var, 0, 1);
        int index = atoi(index_str);

        // Arrays are 1-based → adjust index to 0-based
        int real_index = index - 1;

        // Get address of t[index]
        int src_addr = base_addr + real_index;

        // Get address of destination var
        int dest_addr = get_var_address(dest_var, 1, 0);

        // Emit code: load array value → store to destination
        fprintf(output, "LDA $%04X\n", src_addr);
        fprintf(output, "STA $%04X\n\n", dest_addr);
    }
    // Array element assignment (literal index only for now)
    else if (strstr(line, "[") && strstr(line, "]")) {
        char value[32], var[32], idx_str[32];
        sscanf(line, "store %31s in %31[^[][%31[^]] as number", value, var, idx_str);


        trim_cr(var);


        int base_addr = get_var_address(var, 0, 1);
        int idx = atoi(idx_str);
        fprintf(output, "LDA #%s\nSTA $%04X\n\n", value, base_addr + idx);
    }
    // Memory READ: STORE MEMORY <addr> IN <var> AS <type>
    else if (strstr(line, "store memory")) {
        char addr_str[32], var[32], type[32];

        sscanf(line, "store memory %31[^i] in %31s as %31s", addr_str, var, type);

        trim_cr(addr_str);
        trim_cr(var);
        trim_cr(type);

        unsigned int addr = 0;
        sscanf(addr_str, "%x", &addr);  // parse hex address

        int dest_addr = get_var_address(var, 1, 0);

        // Emit:
        fprintf(output, "LDA $%04X\n", addr);
        fprintf(output, "STA $%04X\n\n", dest_addr);
    }

    // Memory WRITE: STORE <value> IN MEMORY <addr> AS <type>
    else if (strstr(line, "in memory")) {
        char value[32], addr_str[32], type[32];

        sscanf(line, "store %31[^i] in memory %31[^a] as %31s", value, addr_str, type);

        trim_cr(value);
        trim_cr(addr_str);
        trim_cr(type);

        unsigned int addr = 0;
        sscanf(addr_str, "%x", &addr);  // parse hex address

        // Emit:
        fprintf(output, "LDA #%s\n", value);
        fprintf(output, "STA $%04X\n\n", addr);
    }
    // Scalar variable assignment
    else {
        char expr[64], var[32], type[32];
        sscanf(line, "store %63[^i] in %31s as %31s", expr, var, type);

        trim_cr(expr);
        trim_cr(var);
        trim_cr(type);
        printf("DEBUG: expr='%s' var='%s' type='%s'\n", expr, var, type);  // Add this!
        char *end = expr + strlen(expr) - 1;
        while (end > expr && isspace((unsigned char)*end)) *end-- = '\0';

        char result[64];
        int addr = get_var_address(var, 1, 0);

        math_eval(expr, result, output);

        if (strcmp(result, "IN_ACCUMULATOR") == 0) {
            // Value already in A — do not re-load
            // Just store result
            fprintf(output, "STA $%04X\n\n", addr);
        } else {
            // Need to load value
            fprintf(output, "LDA %s\n", result);
            fprintf(output, "STA $%04X\n\n", addr);
        }

    }
}

void handle_print(char *line, FILE *output)
{
    char arg[256];
    sscanf(line, "print %255[^\n]", arg);
    trim_cr(arg);

    // Constants
    const int SCREEN_BASE = 0xE000;
    const int SCREEN_WIDTH = 40;

    // Addresses of cursor_x and cursor_y
    const int CURSOR_X_ADDR = 0xF002;
    const int CURSOR_Y_ADDR = 0xF003;

    // Check if it's a string literal
    if (arg[0] == '"' && arg[strlen(arg) - 1] == '"') {
        // Strip quotes
        arg[strlen(arg) - 1] = '\0';
        char *str = arg + 1;

        // For each character in the string
        for (size_t i = 0; i < strlen(str); i++) {
            // Multiply cursor_y * SCREEN_WIDTH
            emit_multiply("$F003", "40", output);

            fprintf(output, "CLC\n");

            // Load cursor_x and add
            fprintf(output, "ADC $%04X\n", CURSOR_X_ADDR);

            // Store as screen address
            fprintf(output, "STA temp_addr_low\n");
            fprintf(output, "LDA #%02X\n", (SCREEN_BASE >> 8));
            fprintf(output, "STA temp_addr_high\n");

            // Write character
            fprintf(output, "LDA #%d\n", str[i]);
            fprintf(output, "LDY #0\n");
            fprintf(output, "STA (temp_addr_low),Y\n");

            // Advance cursor_x
            fprintf(output, "INC $%04X\n", CURSOR_X_ADDR);
            fprintf(output, "LDA $%04X\n", CURSOR_X_ADDR);
            fprintf(output, "CMP #%d\n", SCREEN_WIDTH);
            fprintf(output, "BNE print_continue_%zu\n", i);

            // Wrap to next line
            fprintf(output, "LDA #0\n");
            fprintf(output, "STA $%04X\n", CURSOR_X_ADDR);
            fprintf(output, "INC $%04X\n", CURSOR_Y_ADDR);

            fprintf(output, "print_continue_%zu:\n", i);
        }

        fprintf(output, "\n");
    }
    else {
        // It's a variable → print its value
        int var_addr = get_var_address(arg, 1, 0);

        // Multiply cursor_y * SCREEN_WIDTH
        emit_multiply("$F003", "40", output);

        fprintf(output, "CLC\n");

        // Load cursor_x and add
        fprintf(output, "ADC $%04X\n", CURSOR_X_ADDR);

        // Store as screen address
        fprintf(output, "STA temp_addr_low\n");
        fprintf(output, "LDA #%02X\n", (SCREEN_BASE >> 8));
        fprintf(output, "STA temp_addr_high\n");

        // Write variable value
        fprintf(output, "LDA $%04X\n", var_addr);
        fprintf(output, "LDY #0\n");
        fprintf(output, "STA (temp_addr_low),Y\n");

        // Advance cursor_x
        fprintf(output, "INC $%04X\n", CURSOR_X_ADDR);
        fprintf(output, "LDA $%04X\n", CURSOR_X_ADDR);
        fprintf(output, "CMP #%d\n", SCREEN_WIDTH);
        fprintf(output, "BNE print_continue_var\n");

        // Wrap to next line
        fprintf(output, "LDA #0\n");
        fprintf(output, "STA $%04X\n", CURSOR_X_ADDR);
        fprintf(output, "INC $%04X\n", CURSOR_Y_ADDR);

        fprintf(output, "print_continue_var:\n");
        fprintf(output, "\n");
    }
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

void handle_if(char *line, FILE *output, FILE *input)

{
    char var1[32], var2[32], cmp[32];
    int n = sscanf(line, "if %31s is %31s %31s do {", var1, cmp, var2);

    // Safety check:
    if (n != 3) {
        fprintf(output, "; ERROR: Failed to parse IF line: %s\n", line);
        return;
    }

  // Get address of left-hand variable:
int addr1 = get_var_address(var1, 1, 0);

// Emit LDA for left-hand side:
fprintf(output, "LDA $%04X\n", addr1);

// Emit CMP:
if (is_constant(var2)) {
    // Right-hand side is a constant → use immediate mode
    fprintf(output, "CMP #%s\n", var2);
} else {
    // Right-hand side is a variable → get its address
    int addr2 = get_var_address(var2, 1, 0);
    fprintf(output, "CMP $%04X\n", addr2);
}

    // Generate unique label
    static int if_count = 0;
    char skip_label[32];
    sprintf(skip_label, "skip_if_%d", if_count++);
    
    // Emit branch to skip block if condition false
    if (strcasecmp(cmp, "greater_than") == 0) {
        fprintf(output, "BCC %s\n", skip_label);
    } else if (strcasecmp(cmp, "less_than") == 0) {
        fprintf(output, "BCS %s\n", skip_label);
    } else if (strcasecmp(cmp, "equal_to") == 0) {
        fprintf(output, "BNE %s\n", skip_label);
    } else if (strcasecmp(cmp, "not_equal_to") == 0) {
        fprintf(output, "BEQ %s\n", skip_label);
    } else {
        fprintf(output, "; Unsupported comparison: %s\n", cmp);
        fprintf(output, "JMP %s  ; skip block due to unknown cmp\n", skip_label);
    }

    // Now read and process block lines until '}'

    char block_line[MAX_LINE];
    while (fgets(block_line, MAX_LINE, input)) {   // You will need to pass input_file to handle_if!

        // Strip leading whitespace
        char *start = block_line;
        while (isspace((unsigned char)*start)) start++;

        // Skip blank or comment lines
        if (*start == '\0' || *start == '#')
            continue;

        // Check for '}'
        if (start[0] == '}') {
            break;  // End of IF block
        }

        // Lowercase whole line for consistency
        for (char *p = start; *p; ++p) *p = tolower((unsigned char)*p);

        // Dispatch line as normal — this is key!
        char keyword[32];
        sscanf(start, "%31s", keyword);

        if (strcmp(keyword, "store") == 0) {
            handle_store(start, output);
        } else if (strcmp(keyword, "print") == 0) {
            handle_print(start, output);
        } else if (strcmp(keyword, "call") == 0) {
            handle_call(start, output);
        } else if (strcmp(keyword, "goto") == 0) {
            handle_goto(start, output);
        } else if (strcmp(keyword, "bookmark") == 0) {
            handle_bookmark(start, output);
        } else {
            handle_unknown(start, output);
        }
    }

    // Emit label to skip block
    fprintf(output, "%s:\n\n", skip_label);
}


void handle_unknown(char *line, FILE *output)
{
    fprintf(output, "; Unknown line: %s\n\n", line);
}

void math_eval(char *expr, char *result, FILE *output)
{
    // Trim leading whitespace
    while (isspace((unsigned char)*expr)) expr++;

    // Look for any math operator
    char *op_pos = strpbrk(expr, "+-*/%");

    if (op_pos) {
        // Split left and right operands
        char op = *op_pos;
        *op_pos = '\0';  // terminate left side
        char *left = expr;
        char *right = op_pos + 1;

        // Trim spaces from left
        while (isspace((unsigned char)*left)) left++;
        char *lend = left + strlen(left) - 1;
        while (lend > left && isspace((unsigned char)*lend)) *lend-- = '\0';

        // Trim spaces from right
        while (isspace((unsigned char)*right)) right++;
        char *rend = right + strlen(right) - 1;
        while (rend > right && isspace((unsigned char)*rend)) *rend-- = '\0';

        // Determine if left is constant or variable
        int is_left_var = 0;
        int left_value = atoi(left);
        if (left_value == 0 && !isdigit(left[0])) is_left_var = 1;

        // Determine if right is constant or variable
        int is_right_var = 0;
        int right_value = atoi(right);
        if (right_value == 0 && !isdigit(right[0])) is_right_var = 1;

        // Emit load for left operand
        if (is_left_var) {
            int left_addr = get_var_address(left, 1, 0);
            fprintf(output, "LDA $%04X\n", left_addr);
        } else {
            fprintf(output, "LDA #%d\n", left_value);
        }

        // Emit operation
        switch (op) {
            case '+':
                fprintf(output, "CLC\n");
                if (is_right_var) {
                    int right_addr = get_var_address(right, 1, 0);
                    fprintf(output, "ADC $%04X\n", right_addr);
                } else {
                    fprintf(output, "ADC #%d\n", right_value);
                }
                break;

            case '-':
                fprintf(output, "SEC\n");
                if (is_right_var) {
                    int right_addr = get_var_address(right, 1, 0);
                    fprintf(output, "SBC $%04X\n", right_addr);
                } else {
                    fprintf(output, "SBC #%d\n", right_value);
                }
                break;

            case '*':
                emit_multiply(left, right, output);
                strcpy(result, "IN_ACCUMULATOR");
                break;

            case '/':
                emit_divide(left, right, output);
                strcpy(result, "IN_ACCUMULATOR");
                break;


            case '%':
                fprintf(output, "; MODULO operation not implemented yet\n");
                strcpy(result, "IN_ACCUMULATOR");
                break;

            default:
                fprintf(output, "; Unknown operator: %c\n", op);
                break;
        }

        // Write result so caller knows nothing else to do
        strcpy(result, "IN_ACCUMULATOR");  // signal: value already in A
        return;
    }

    // If it's not an expression, handle simple constant or variable

    // Trim trailing spaces
    char *end = expr + strlen(expr) - 1;
    while (end > expr && isspace((unsigned char)*end)) *end-- = '\0';

    // Is it a number?
    int value = atoi(expr);
    if (value != 0 || isdigit(expr[0])) {
        // It's a constant number
        sprintf(result, "#%d", value);
        return;
    }

    // It's a variable
    int addr = get_var_address(expr, 1, 0);
    sprintf(result, "$%04X", addr);
    return;
}



void emit_multiply(char *left, char *right, FILE *output)
{
    if (TARGET == TARGET_6502) {
        fprintf(output, "; 6502 multiply: %s * %s\n", left, right);

        if (strcmp(right, "40") == 0) {
            // Example temp addresses
            const int TEMP_MULTIPLY = 0xF004;
            const int TEMP_32 = 0xF005;
            const int TEMP_8 = 0xF006;

            // A = left
            fprintf(output, "LDA %s\n", left);
            fprintf(output, "STA $%04X\n", TEMP_MULTIPLY);

            // A * 32
            fprintf(output, "LDA $%04X\n", TEMP_MULTIPLY);
            fprintf(output, "ASL A\n");
            fprintf(output, "ASL A\n");
            fprintf(output, "ASL A\n");
            fprintf(output, "ASL A\n");
            fprintf(output, "ASL A\n");
            fprintf(output, "STA $%04X\n", TEMP_32);

            // A * 8
            fprintf(output, "LDA $%04X\n", TEMP_MULTIPLY);
            fprintf(output, "ASL A\n");
            fprintf(output, "ASL A\n");
            fprintf(output, "ASL A\n");
            fprintf(output, "STA $%04X\n", TEMP_8);

            // Add results
            fprintf(output, "CLC\n");
            fprintf(output, "LDA $%04X\n", TEMP_32);
            fprintf(output, "ADC $%04X\n", TEMP_8);
            // Result now in A
        } else {
            fprintf(output, "; Fallback multiply not implemented yet for %s * %s\n", left, right);
        }
    }
    else if (TARGET == TARGET_MEGA6502) {
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
