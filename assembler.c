#define MEMORY_SIZE 256
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

uint16_t memory[MEMORY_SIZE]; // Memória principal
uint16_t current_address = 0; // Contador de endereço

typedef struct {
    char name[50];
    uint16_t address;
} Symbol;

Symbol symbol_table[MEMORY_SIZE];
int symbol_count = 0;

// Escreve o cabeçalho correto do Neander
void header_neander(FILE *bin_file) {
    const uint8_t header[] = {0x03, 0x4E, 0x44, 0x52};
    fwrite(header, sizeof(uint8_t), 4, bin_file);
}

void add_variables(const char *name, uint16_t address) {
    strncpy(symbol_table[symbol_count].name, name, 50);
    symbol_table[symbol_count].address = address;
    symbol_count++;
}

uint16_t find_variables(const char *name) {
    for(int i = 0; i < symbol_count; i++) {
        if(strcmp(symbol_table[i].name, name) == 0) {
            return symbol_table[i].address;
        }
    }
    printf("Erro: Símbolo não encontrado '%s'\n", name);
    exit(1);
}

void data_section(char *line) {
    char name[50], db[3], value[10];
    if(sscanf(line, "%49s %2s %9s", name, db, value) != 3) {
        printf("Erro de formatação em .DATA: %s\n", line);
        exit(1);
    }
    
    add_variables(name, current_address);
    
    if(value[0] != '?') {
        memory[current_address] = (uint16_t)atoi(value);
    }
    current_address++;
}

void code_section(char *line) {
    char mnemonic[5], operand[50];
    int count = sscanf(line, "%4s %49s", mnemonic, operand);
    
    // Mapeamento de instruções
    struct {
        char name[5];
        uint16_t opcode;
        bool has_operand;
    } instructions[] = {
        {"NOP", 0x0000, false}, {"STA", 0x0010, true}, {"LDA", 0x0020, true},
        {"ADD", 0x0030, true}, {"OR", 0x0040, true},  {"AND", 0x0050, true},
        {"NOT", 0x0060, false}, {"JMP", 0x0080, true}, {"JN", 0x0090, true},
        {"JZ", 0x00A0, true}, {"HLT", 0x00F0, false}
    };

    for(size_t i = 0; i < sizeof(instructions)/sizeof(instructions[0]); i++) {
        if(strcmp(mnemonic, instructions[i].name) == 0) {
            memory[current_address++] = instructions[i].opcode;
            
            if(instructions[i].has_operand) {
                uint16_t addr = isdigit(operand[0]) ? 
                    (uint16_t)atoi(operand) : 
                    find_variables(operand);
                    
                memory[current_address++] = addr;
            }
            return;
        }
    }
    printf("Instrução inválida: %s\n", mnemonic);
    exit(1);
}

void assembler(const char *input_file, const char *output_file) {
    FILE *src = fopen(input_file, "r");
    if(!src) {
        perror("Erro ao abrir arquivo");
        exit(1);
    }

    char line[256];
    enum { NONE, DATA, CODE } section = NONE;
    
    while(fgets(line, sizeof(line), src)) {
        // Remove comentários e espaços
        char *p = line;
        while(*p && *p != ';') p++;
        *p = '\0';
        
        if(strstr(line, ".DATA")) {
            section = DATA;
            continue;
        }
        if(strstr(line, ".CODE")) {
            section = CODE;
            continue;
        }
        
        if(section == DATA) data_section(line);
        if(section == CODE) code_section(line);
    }
    fclose(src);
    
    // Preenche o resto da memória com zeros
    while(current_address < MEMORY_SIZE) {
        memory[current_address++] = 0x0000;
    }
    
    // Escreve arquivo binário com header
    FILE *bin = fopen(output_file, "wb");
    header_neander(bin);
    fwrite(memory, sizeof(uint16_t), MEMORY_SIZE, bin);
    fclose(bin);
}

int main(int argc, char *argv[]) {
    if(argc != 3) {
        printf("Uso: %s entrada.asm saida.bin\n", argv[0]);
        return 1;
    }
    
    memset(memory, 0, sizeof(memory));
    assembler(argv[1], argv[2]);
    return 0;
}