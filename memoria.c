#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    tipo_I,
    tipo_J,
    tipo_R,
    tipo_OUTROS
} classe_inst;


typedef struct {
    classe_inst tipo_inst;  // VARIAVEL DO TIPO NUM (I, J, R)
    char inst_char[17];     
    int opcode;
    int rs;
    int rt;
    int rd;
    int funct;
    int imm;
    int addr;
} instrucao;

void salvarinstrucoes(instrucao *mem, const char *nomeArquivo) {
    FILE *arquivo = fopen(nomeArquivo, "r");
    if (arquivo == NULL) {
        printf("Erro ao abrir o arquivo %s\n", nomeArquivo);
        exit(1);  
    }

    char binario[17]; 
    
    for (int i = 0; i < 11; i++) { 
        if (fgets(binario, sizeof(binario), arquivo) != NULL) {
            
            strcpy(mem[i].inst_char, binario);
        }
    }
    
    fclose(arquivo);  
}

void printmemoria(instrucao *mem) {
    printf("Conteúdo da memória:\n");
    for (int i = 0; i < 11; i++) {
        printf("Memória[%d]: %s\n", i, mem[i].inst_char);
    }
}

int main() {
    //11 PRA TESTE, DEPOIS 256
    instrucao *mem = (instrucao *)malloc(11 * sizeof(instrucao));

    salvarinstrucoes(mem, "instrucoes.txt");

    printmemoria(mem);
    
    return 0;
}
