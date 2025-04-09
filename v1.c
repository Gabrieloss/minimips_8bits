#include <stdio.h>
#include <stdlib.h>
#include <string.h>



typedef enum {
    tipo_I, tipo_J, tipo_R, tipo_OUTROS
} classe_inst;

typedef struct {
    classe_inst tipo_inst;
    char inst_char[17];
    int opcode;
    int rs;
    int rt;
    int rd;
    int funct;
    int imm;
    int addr;
} instrucao;

typedef struct {
    char dado_char[9];  // 9 do NULO
    int dado;           // valor decimal
} dado;

//  MEMÓRIA DE INSTRUÇÕES
int carregarMemoria(instrucao *mem_instrucao, const char *nome_arquivo) {
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (!arquivo) {
        printf("Erro ao abrir o arquivo %s\n", nome_arquivo);
        return 0;
    }

    int cont = 0;
    char linha[20];
    while (fgets(linha, sizeof(linha), arquivo)) {
        linha[strcspn(linha, "\n")] = '\0';
        if (strlen(linha) == 0){
            continue; // RESOLVIDO: BUG DE CARREGAR LINHA VAZIA 
        }
        strncpy(mem_instrucao[cont].inst_char, linha, 17);
        mem_instrucao[cont].inst_char[16] = '\0';
        cont++;
        if (cont >= 256) {
            printf("Limite de instruções atingido (256).\n");
            break;
        }
    }

    fclose(arquivo);
    return cont;
}

// 
void decoder(instrucao *inst, int cont) {  
    char *bin = inst[cont].inst_char;

    char opcode_bin[5];
    strncpy(opcode_bin, bin, 4);
    opcode_bin[4] = '\0';
    inst[cont].opcode = (int)strtol(opcode_bin, NULL, 2);

    int opcode = inst[cont].opcode;

    if (opcode == 0) {
        inst[cont].tipo_inst = tipo_R;

        char rs[4], rt[4], rd[4], funct[4];
        strncpy(rs, bin + 4, 3); rs[3] = '\0'; //PEGA OS 3 BITS
        strncpy(rt, bin + 7, 3); rt[3] = '\0';
        strncpy(rd, bin + 10, 3); rd[3] = '\0';
        strncpy(funct, bin + 13, 3); funct[3] = '\0';

        inst[cont].rs = (int)strtol(rs, NULL, 2); 
        inst[cont].rt = (int)strtol(rt, NULL, 2);
        inst[cont].rd = (int)strtol(rd, NULL, 2);
        inst[cont].funct = (int)strtol(funct, NULL, 2);

    } else if (opcode == 2) {
        inst[cont].tipo_inst = tipo_J;

        char addr[8];
        strncpy(addr, bin + 9, 7); addr[7] = '\0';
        inst[cont].addr = (int)strtol(addr, NULL, 2);

    } else if (opcode == 4 || opcode == 8 || opcode == 11 || opcode == 15) {
        inst[cont].tipo_inst = tipo_I;

        char rs[4], rt[4], imm[7];
        strncpy(rs, bin + 4, 3); rs[3] = '\0';
        strncpy(rt, bin + 7, 3); rt[3] = '\0';
        strncpy(imm, bin + 10, 6); imm[6] = '\0';

        inst[cont].rs = (int)strtol(rs, NULL, 2);
        inst[cont].rt = (int)strtol(rt, NULL, 2);

        if (imm[0] == '1') {
            for (int i = 0; i < 6; i++) imm[i] = (imm[i] == '1') ? '0' : '1';
            int complemento = (int)strtol(imm, NULL, 2) + 1;
            inst[cont].imm = -complemento;
        } else {
            inst[cont].imm = (int)strtol(imm, NULL, 2);
        }
    } else {
        inst[cont].tipo_inst = tipo_OUTROS;
        inst[cont].opcode = -1;
        printf("Instrução com opcode não reconhecido: %d\n", opcode);
    }
}

int carregarMemoriaDados(dado *mem_dados, const char *nome_arquivo) {
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (!arquivo) {
        printf("Erro ao abrir o arquivo %s\n", nome_arquivo);
        return 0;
    }

    int cont = 0;
    while (fgets(mem_dados[cont].dado_char, sizeof(mem_dados[cont].dado_char), arquivo)) {
        mem_dados[cont].dado_char[strcspn(mem_dados[cont].dado_char, "\n")] = '\0';
        mem_dados[cont].dado = (int)strtol(mem_dados[cont].dado_char, NULL, 2); //SE FOR BINARIO CONVERTE PRA DECIMAL
        cont++;
        if (cont >= 256) {
            break;
        }
    }

    fclose(arquivo);
    return cont;
}


int ULA(int *PC, instrucao *inst, dado *mem_dados, int *registradores) {
    int op = inst[*PC].opcode;
    int rs = inst[*PC].rs;
    int rt = inst[*PC].rt;
    int rd = inst[*PC].rd;
    int funct = inst[*PC].funct;
    int imm = inst[*PC].imm;
    int resultado = 0;

    if (op == 0) {  // Tipo R
        switch (funct) {
            case 0: resultado = registradores[rs] + registradores[rt]; break; // ADD
            case 2: resultado = registradores[rs] - registradores[rt]; break; // SUB
            case 4: resultado = registradores[rs] & registradores[rt]; break; // AND
            case 5: resultado = registradores[rs] | registradores[rt]; break; // OR
            default:
                printf("\nERRO\n");
                return 0;
        }
        if (resultado > 127 || resultado < -128){
            printf("[OVERFLOW DETECTADO]\n");
        }
        registradores[rd] = resultado;
    }

    else if (op == 4) {  // ADDI
        resultado = registradores[rs] + imm;
        if (resultado > 127 || resultado < -128){
            printf("[OVERFLOW DETECTADO]\n");
        }
        registradores[rt] = resultado;
    }

    else if (op == 11) {  // LW
        int endereco = registradores[rs] + imm;
        if (endereco >= 0 && endereco < 256) {
            registradores[rt] = mem_dados[endereco].dado;
        } else {
            printf("[ERRO LW] Endereço fora dos limites: %d\n", endereco);
        }
    }

    else if (op == 15) {  // SW
        int endereco = registradores[rs] + imm;
        if (endereco >= 0 && endereco < 256) {
            mem_dados[endereco].dado = registradores[rt];
            for (int i = 7; i >= 0; i--)
                mem_dados[endereco].dado_char[7 - i] = ((registradores[rt] >> i) & 1) + '0';
            mem_dados[endereco].dado_char[8] = '\0';
        } else {
            printf("[ERRO SW] Endereço fora dos limites: %d\n", endereco);
        }
    }

    else if (op == 8) {  // BEQ
        if (registradores[rs] == registradores[rt]) {
            *PC += imm;  // Pula instruções
            return 0;    
        }
    }

    else {
        printf("[ERRO] Opcode não reconhecido. \n");
    }

    return resultado;
}


void Execute_Instrucao(int *PC, instrucao *inst, dado *mem_dados, int *registradores) {
    if (inst[*PC].opcode == 2) {
        printf("Instrução JUMP para endereço %d\n", inst[*PC].addr);
        *PC = inst[*PC].addr;
    } else if (inst[*PC].opcode == -1) {
        printf("PC está apontando para um campo vazio!\n");
    } else {
        int resultado = ULA(PC, inst, mem_dados, registradores);
        printf("Resultado da ULA: %d\n", resultado);
        (*PC)++;
    }
}

void imprimirRegistradores(int *registradores) {
    printf("\nBanco de Registradores:\n");
    for (int i = 0; i < 8; i++) {
        printf("R%d: %d\n", i, registradores[i]);
    }
}

void imprimirMemoriaDados(dado *memoria_dados) {
    printf("\nMemória de Dados:\n");
    for (int i = 0; i < 256; i++) {
        if (strlen(memoria_dados[i].dado_char) > 0) {
            printf("[%3d] %s = %d\n", i, memoria_dados[i].dado_char, memoria_dados[i].dado);
        }
    }
}

void ImprimirMemoriaInstrucoes(instrucao *mem_inst) {
    printf("==== Memória de Instruções ====\n\n");
    for (int i = 0; i < 256; i++) {
        if (mem_inst[i].opcode == -1){
        continue;
        }
        printf("[%03d] ", i);

        switch (mem_inst[i].tipo_inst) {
            case tipo_R:
                printf("R | opcode: %d, rs: %d, rt: %d, rd: %d, funct: %d\n",
                       mem_inst[i].opcode, mem_inst[i].rs, mem_inst[i].rt,
                       mem_inst[i].rd, mem_inst[i].funct);
                break;

            case tipo_I:
                printf("I | opcode: %d, rs: %d, rt: %d, imm: %d\n",
                       mem_inst[i].opcode, mem_inst[i].rs,
                       mem_inst[i].rt, mem_inst[i].imm);
                break;

            case tipo_J:
                printf("J | opcode: %d, addr: %d\n",
                       mem_inst[i].opcode, mem_inst[i].addr);
                break;

            default:
                printf("Instrução inválida.\n");
                break;
        }
    }
    printf("\n");
}


void menu() {
    printf("\n=============================================\n");
    printf("                   Menu                     \n");
    printf("=============================================\n");
    printf("  1. Carregar memória de instruções (.mem)  \n");
    printf("  2. Carregar memória de dados (.dat)       \n");
    printf("  3. Imprimir memórias                      \n");
    printf("  4. Imprimir banco de registradores        \n");
    printf("  5. Imprimir todo o simulador              \n");
    printf("  6. Salvar arquivo .asm                    \n");
    printf("  7. Salvar arquivo .dat                    \n");
    printf("  8. Executar programa (run)                \n");
    printf("  9. Executar uma instrução (step)          \n");
    printf("  10. Voltar uma instrução (back)           \n");
    printf("  0. Sair                                   \n");
    printf("=============================================\n");
    printf("Escolha uma opção: ");
}


int main() {
    instrucao memoria_instrucoes[256];
    dado memoria_dados[256] = {0};
    int *registradores = malloc(sizeof(int) * 8);
    int pc = 0; 
    int MemoriaInstrucoesLOAD = 0;

    int total_instr = 0;
    int opcao;
    do {
        menu();
        scanf("%d", &opcao);
        setbuf(stdout, NULL);
        printf("\n");
        switch (opcao) {
            case 1:
                total_instr = carregarMemoria(memoria_instrucoes, "instrucoes.mem");
                for (int i = 0; i < total_instr; i++) {
                    decoder(memoria_instrucoes, i);
                }
                MemoriaInstrucoesLOAD = 1;
                break;
            case 2:
                carregarMemoriaDados(memoria_dados, "dados.dat");
                break;
            case 3: //PRINT MEMORIA 
                if(MemoriaInstrucoesLOAD == 1){
                    printf("===== Memória de Instruções =====\n");
                    ImprimirMemoriaInstrucoes(memoria_instrucoes);
                }
                else{
                    printf("Memória de Instruções não carregada.\n");
                }
                
                printf("\n===== Memória de Dados =====\n");
                imprimirMemoriaDados(memoria_dados);
                break;            
            case 4: //PRINT REGISTRADORES
                imprimirRegistradores(registradores);
                break;
            case 5: //IMPRIMIR O PROGRAMA(MEMORIAS, REGISTRADORES)
                if(MemoriaInstrucoesLOAD == 1){
                    printf("===== Memória de Instruções =====\n");
                    ImprimirMemoriaInstrucoes(memoria_instrucoes);  
                }
                else{
                    printf("Memória de Instruções não carregada.\n");
                }
                
                printf("\n===== Memória de Dados =====\n");
                imprimirMemoriaDados(memoria_dados);

                imprimirRegistradores(registradores);
                break;
            case 6:
                printf("[Salvar .asm futuro]\n");
                break;
            case 7:
                printf("[Salvar .dat futuro]\n");
                break;
            case 8: //RUN
                if (MemoriaInstrucoesLOAD == 1) {
                    while (pc < total_instr && memoria_instrucoes[pc].opcode != -1) {
                        Execute_Instrucao(&pc, memoria_instrucoes, memoria_dados, registradores);
                        printf("********Próxima Instrução********\n\n");
                        printf("\tPC = %d\n\n", pc);
                    }
                } else {
                    printf("\nMemórias de instruções não foi carregada.\n");
                }
                break;
            case 9: //STEP
                if (MemoriaInstrucoesLOAD == 1 && pc < total_instr) {
                    Execute_Instrucao(&pc, memoria_instrucoes, memoria_dados, registradores);
                    printf("********Próxima Instrução********\n\n");
                    printf("\tPC = %d\n\n", pc);
                } else {
                    printf("\tMemórias de instruções não foi carregada ou fim das instruções.\n");
                }
                break;
            case 10: //BACK 
                printf("[Voltar step ainda não implementado]\n");
                break;
            case 0:
                printf("Saindo...\n");
                break;
            default:
                printf("Opção inválida.\n");
        }

    } while (opcao != 0);

    free(registradores);
    return 0;
}
        