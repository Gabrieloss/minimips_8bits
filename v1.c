#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum
{
    tipo_I,
    tipo_J,
    tipo_R,
    tipo_OUTROS
} classe_inst;

typedef struct
{
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

typedef struct
{
    int dado;          // valor decimal
} dado;

//  MEMÓRIA DE INSTRUÇÕES
int carregarMemoria(instrucao *mem_instrucao, const char *nome_arquivo)
{
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (!arquivo)
    {
        printf("Erro ao abrir o arquivo.");
        return 0;
    }

    int cont = 0;
    char linha[20];
    while (fgets(linha, sizeof(linha), arquivo))
    {
        linha[strcspn(linha, "\n")] = '\0';
        if (strlen(linha) == 0)
        {
            continue; // RESOLVIDO: BUG DE CARREGAR LINHA VAZIA
        }
        strncpy(mem_instrucao[cont].inst_char, linha, 17);
        mem_instrucao[cont].inst_char[16] = '\0';
        cont++;
        if (cont >= 256)
        {
            printf("Limite de instruções atingido (256).\n");
            break;
        }
    }

    fclose(arquivo);
    return cont;
}

//
void decoder(instrucao *inst, int cont)
{
    char *bin = inst[cont].inst_char;

    char opcode_bin[5];
    strncpy(opcode_bin, bin, 4);
    opcode_bin[4] = '\0';
    inst[cont].opcode = (int)strtol(opcode_bin, NULL, 2);

    int opcode = inst[cont].opcode;

    if (opcode == 0)
    {
        inst[cont].tipo_inst = tipo_R;

        char rs[4], rt[4], rd[4], funct[4];
        strncpy(rs, bin + 4, 3);
        rs[3] = '\0'; // PEGA OS 3 BITS
        strncpy(rt, bin + 7, 3);
        rt[3] = '\0';
        strncpy(rd, bin + 10, 3);
        rd[3] = '\0';
        strncpy(funct, bin + 13, 3);
        funct[3] = '\0';

        inst[cont].rs = (int)strtol(rs, NULL, 2);
        inst[cont].rt = (int)strtol(rt, NULL, 2);
        inst[cont].rd = (int)strtol(rd, NULL, 2);
        inst[cont].funct = (int)strtol(funct, NULL, 2);
    }
    else if (opcode == 2)
    {
        inst[cont].tipo_inst = tipo_J;

        char addr[8];
        strncpy(addr, bin + 9, 7);
        addr[7] = '\0';
        inst[cont].addr = (int)strtol(addr, NULL, 2);
    }
    else if (opcode == 4 || opcode == 8 || opcode == 11 || opcode == 15)
    {
        inst[cont].tipo_inst = tipo_I;

        char rs[4], rt[4], imm[7];
        strncpy(rs, bin + 4, 3);
        rs[3] = '\0';
        strncpy(rt, bin + 7, 3);
        rt[3] = '\0';
        strncpy(imm, bin + 10, 6);
        imm[6] = '\0';

        inst[cont].rs = (int)strtol(rs, NULL, 2);
        inst[cont].rt = (int)strtol(rt, NULL, 2);

        // IMM NEGATIVO
        if (imm[0] == '1')
        {
            for (int i = 5; i >= 0; i--)
            { // SUBTRAI 1 BIT
                if (imm[i] == '1')
                {
                    imm[i] = '0';
                    break;
                }
                else
                {
                    imm[i] = '1';
                }
            }
            for (int i = 0; i < 6; i++)
            { // NOT
                if (imm[i] == '1')
                {
                    imm[i] = '0';
                }
                else
                {
                    imm[i] = '1';
                }
            }

            int complemento = (int)strtol(imm, NULL, 2);
            inst[cont].imm = -complemento; // negativa
        }
        else
        {
            inst[cont].imm = (int)strtol(imm, NULL, 2);
        }
    }
    else
    {
        inst[cont].tipo_inst = tipo_OUTROS;
        inst[cont].opcode = -1;
        printf("\nOPCODE INVALIDO\n");
    }
}


void carregarMemoriaDados(dado *mem_dados, const char *ArquivoDados) {

    for (int i = 0; i < 256; i++) {
        mem_dados[i].dado = 0;
    }

    FILE *arquivo = fopen(ArquivoDados, "r");
    if (!arquivo) {
        printf("Erro ao abrir o arquivo\n");
        return;
    }

    for (int i = 0; i < 256; i++) {
        fscanf(arquivo, "%d", &mem_dados[i].dado);
        }
    
    fclose(arquivo);
}

int ULA(int operacao, int num1, int num2, int *ZERO)
{

    int resultado;

    switch (operacao)
    {
    case 0:
        resultado = num1 + num2;
        break;
    case 1:
        resultado = num1 - num2;
        break;
    case 2:
        resultado = num1 & num2;
        break;
    case 3:
        resultado = num1 | num2;
        break;
    default:
        printf("Inválido.\n");
        return 0;
    }

    if (resultado > 127 || resultado < -128)
        printf("OVERFLOW\n");

    *ZERO = (resultado == 0);
    return resultado;
}

void ExecutaInstrucao(int *PC, instrucao *mem_inst, dado *mem_dados, int *registradores, int *ZERO)
{
    instrucao inst_local = mem_inst[*PC];
    int resultado;

    switch (inst_local.opcode){

    case 0: //TIPO R
        resultado = ULA(inst_local.funct, registradores[inst_local.rs], registradores[inst_local.rt], ZERO);
        registradores[inst_local.rd] = resultado;
        break;
    case 4: //ADDI
        resultado = ULA(0, registradores[inst_local.rs], inst_local.imm, ZERO);
        registradores[inst_local.rt] = resultado;
        break;
    case 8: //BEQ
        resultado = ULA(1, registradores[inst_local.rs], registradores[inst_local.rt], ZERO);
        if (*ZERO == 1)
        {                          
            *PC += inst_local.imm; 
            return;                // Não incrementa PC
        }
        break;
    case 11: // LW (usa ULA para calcular rs + imm)
        resultado = ULA(0, registradores[inst_local.rs], inst_local.imm, ZERO);
        if (resultado >= 0 && resultado < 256)
        {
            registradores[inst_local.rt] = mem_dados[resultado].dado;
        }
        else
        {
            printf("[ERRO LW]\n");
        }
        break;
    case 15: // SW 
        resultado = ULA(0, registradores[inst_local.rs], inst_local.imm, ZERO);
        if (resultado >= 0 && resultado < 256)
        {
            mem_dados[resultado].dado = registradores[inst_local.rt];
        }
        else
        {
            printf("[ERRO SW]\n");
        }
        break;

    // J (Jump absoluto - não usa ULA)
    case 2:
        *PC = inst_local.addr; 
        return;                // Não incrementa PC

    default:
        printf("[ERRO]\n");
        break;
    }

    (*PC)++; 
}

void imprimirRegistradores(int *registradores)
{
    printf("\nBanco de Registradores:\n");
    for (int i = 0; i < 8; i++)
    {
        printf("R%d: %d\n", i, registradores[i]);
    }
}


void imprimirMemoriaDados(dado *memoria_dados) {
    printf("\nMemória de Dados:\n");
    for (int i = 0; i < 256; i++) {
            printf("[%03d] %d\n", i, memoria_dados[i].dado); //imprime a posição 000 e o conteúdo 
        }
    }


void ImprimirMemoriaInstrucoes(instrucao *mem_inst)
{

    printf("==== Memória de Instruções ====\n\n");

    for (int i = 0; i < 256; i++)
    {
        if (mem_inst[i].opcode == -1)
        {
            continue;
        }
        printf("[%03d] ", i);

        switch (mem_inst[i].tipo_inst)
        {
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

void salvarASM(instrucao *mem_inst, int tamanho, char *ArquivoASM)
{
    FILE *arq = fopen(ArquivoASM, "w");
    if (arq == NULL)
    {
        printf("Erro ao abrir arquivo .asm.\n");
        return;
    }

    for (int i = 0; i < tamanho; i++)
    {
        instrucao inst = mem_inst[i];
        switch (inst.tipo_inst)
        {
        case tipo_R:
            if (inst.funct == 0)
                fprintf(arq, "add $%d, $%d, $%d\n", inst.rd, inst.rs, inst.rt);
            else if (inst.funct == 2)
                fprintf(arq, "sub $%d, $%d, $%d\n", inst.rd, inst.rs, inst.rt);
            else if (inst.funct == 4)
                fprintf(arq, "and $%d, $%d, $%d\n", inst.rd, inst.rs, inst.rt);
            else if (inst.funct == 5)
                fprintf(arq, "or  $%d, $%d, $%d\n", inst.rd, inst.rs, inst.rt);
            break;

        case tipo_I:
            if (inst.opcode == 4)
                fprintf(arq, "addi $%d, $%d, %d\n", inst.rt, inst.rs, inst.imm);
            else if (inst.opcode == 11)
                fprintf(arq, "lw   $%d, %d($%d)\n", inst.rt, inst.imm, inst.rs);
            else if (inst.opcode == 15)
                fprintf(arq, "sw   $%d, %d($%d)\n", inst.rt, inst.imm, inst.rs);
            else if (inst.opcode == 8)
                fprintf(arq, "beq  $%d, $%d, %d\n", inst.rt, inst.rs, inst.imm);
            break;

        case tipo_J:
            if (inst.opcode == 2)
                fprintf(arq, "j %d\n", inst.addr); //
            break;

        default:
            fprintf(arq, "# Instrução desconhecida");
        }
    }

    fclose(arq);
    printf("Arquivo .asm salvo com sucesso!");
}


void salvarArquivoDat(dado *mem_dados, const char *SaidaDados) {
    FILE *arquivo = fopen(SaidaDados, "w");
    if (!arquivo) {
        printf("Erro ao criar arquivo .dat\n");
        return;
    }

    for (int i = 0; i < 256; i++) {
            fprintf(arquivo, "%d\n", mem_dados[i].dado);
        }
    
    fclose(arquivo);
    printf("Arquivo .dat salvo!\n");
    }


void menu()
{
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

int main()
{
    instrucao memoria_instrucoes[256];
    dado memoria_dados[256] = {0};
    int *registradores = malloc(sizeof(int) * 8);
    int pc = 0;
    int MemoriaInstrucoesLOAD = 0;
    int ZERO = 0;

    int total_instr = 0;
    int opcao;
    do
    {
        menu();
        scanf("%d", &opcao);
        setbuf(stdout, NULL);
        printf("\n");
        switch (opcao)
        {
        case 1:
            total_instr = carregarMemoria(memoria_instrucoes, "instrucoes.mem");
            for (int i = 0; i < total_instr; i++)
            {
                decoder(memoria_instrucoes, i);
            }
            MemoriaInstrucoesLOAD = 1;
            break;
        case 2:
            carregarMemoriaDados(memoria_dados, "dados.dat");
            break;
        case 3: // PRINT MEMORIA
            if (MemoriaInstrucoesLOAD == 1)
            {
                printf("===== Memória de Instruções =====\n");
                ImprimirMemoriaInstrucoes(memoria_instrucoes);
            }
            else
            {
                printf("Memória de Instruções não carregada.\n");
            }

            printf("\n===== Memória de Dados =====\n");
            imprimirMemoriaDados(memoria_dados);
            break;
        case 4: // PRINT REGISTRADORES
            imprimirRegistradores(registradores);
            break;
        case 5: // IMPRIMIR O PROGRAMA(MEMORIAS, REGISTRADORES)
            if (MemoriaInstrucoesLOAD == 1)
            {
                printf("===== Memória de Instruções =====\n");
                ImprimirMemoriaInstrucoes(memoria_instrucoes);
            }
            else
            {
                printf("Memória de Instruções não carregada.\n");
            }

            printf("\n===== Memória de Dados =====\n");
            imprimirMemoriaDados(memoria_dados);

            imprimirRegistradores(registradores);
            break;
        case 6: // SALVA O .ASM
            if (MemoriaInstrucoesLOAD == 1)
                salvarASM(memoria_instrucoes, total_instr, "saida.asm");
            break;
        case 7: //SALVAR .DAT
        salvarArquivoDat(memoria_dados, "saida_dados.dat");
            break;
        case 8: // RUN
            if (MemoriaInstrucoesLOAD == 1)
            {
                while (pc < total_instr)
                {
                    ExecutaInstrucao(&pc, memoria_instrucoes, memoria_dados, registradores, &ZERO);
                    printf("********Próxima Instrução********\n\n");
                    printf("\tPC = %d\n\n", pc);
                }
            }
            else
            {
                printf("\nMemórias de instruções não foi carregada.\n");
            }
            break;
        case 9: // STEP
            if (MemoriaInstrucoesLOAD == 1 && pc < total_instr)
            {
                ExecutaInstrucao(&pc, memoria_instrucoes, memoria_dados, registradores, &ZERO);
                printf("========Próxima Instrução=======\n\n");
                printf("\tPC = %d\n\n", pc);
            }
            else
            {
                printf("\tMemórias de instruções não foi carregada ou fim das instruções.\n");
            }
            break;
        case 10: // BACK
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
