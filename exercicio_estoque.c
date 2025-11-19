/*
  Gerenciador de Estoque Simplificado (C puro)
  Requisitos:
  - struct Produto { nome, codigo, preco }
  - abrir/criar arquivo binário em main()
  - tamanho(): usa fseek(SEEK_END) e sizeof(struct)
  - cadastrar(): lê dados do usuário, fseek(SEEK_END) + fwrite()
  - consultar(): solicita índice (posição) e exibe registro com fseek(SEEK_SET) + fread()
  - modularização: protótipos e funções fora da main()
  - limpaBuffer() padronizada
  - leitura de strings segura com fgets() e remoção de '\n' via strcspn()
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ----------- Tipos e constantes ----------- */
typedef struct {
    char nome[50];
    int codigo;
    float preco;
} Produto;

const char *NOME_ARQUIVO = "estoque.dat";

/* ----------- Protótipos (modularização) ----------- */
void limpaBuffer(void);
void ler_string(const char *prompt, char *buffer, size_t tamanho);
long tamanho(FILE *fp); /* retorna número de registros no arquivo */
void cadastrar(FILE *fp);
void consultar(FILE *fp);

/* ----------- Implementações ----------- */

/* Limpa buffer de stdin até '\n' ou EOF */
void limpaBuffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

/* Lê string com fgets e remove o '\n' (usa strcspn) */
/* Se a string preenchida não contiver '\n', limpa o restante do buffer */
void ler_string(const char *prompt, char *buffer, size_t tamanho) {
    printf("%s", prompt);
    if (fgets(buffer, (int)tamanho, stdin) == NULL) {
        /* Em caso de erro, coloca string vazia */
        buffer[0] = '\0';
        return;
    }
    /* Remove newline se presente */
    buffer[strcspn(buffer, "\n")] = '\0';

    /* Se o último caractere lido não foi '\n' e não atingimos EOF,
       é porque sobrou dados no stdin -> limpamos */
    if (strchr(buffer, '\n') == NULL && strlen(buffer) == tamanho - 1) {
        /* Ainda pode haver caracteres restantes na entrada */
        int c;
        if ((c = getchar()) != '\n' && c != EOF) {
            /* existe resto -> limpar todo o resto */
            while ((c = getchar()) != '\n' && c != EOF) { }
        }
    }
}

/* Retorna número total de registros no arquivo */
long tamanho(FILE *fp) {
    if (fp == NULL) return 0;
    if (fseek(fp, 0, SEEK_END) != 0) {
        perror("fseek (SEEK_END) falhou em tamanho()");
        return 0;
    }
    long bytes = ftell(fp);
    if (bytes < 0) {
        perror("ftell falhou em tamanho()");
        return 0;
    }
    return bytes / (long)sizeof(Produto);
}

/* Cadastra novo produto e grava no final do arquivo */
void cadastrar(FILE *fp) {
    if (fp == NULL) {
        printf("Arquivo nao esta aberto para cadastrar.\n");
        return;
    }

    Produto p;
    /* Ler campos */
    ler_string("Nome do produto: ", p.nome, sizeof(p.nome));

    printf("Codigo (inteiro): ");
    if (scanf("%d", &p.codigo) != 1) {
        printf("Entrada invalida para código.\n");
        limpaBuffer();
        return;
    }
    limpaBuffer(); /* remove '\n' após scanf */

    printf("Preco (ex: 19.90): ");
    if (scanf("%f", &p.preco) != 1) {
        printf("Entrada inválida para preço.\n");
        limpaBuffer();
        return;
    }
    limpaBuffer();

    /* posiciona no final e escreve */
    if (fseek(fp, 0, SEEK_END) != 0) {
        perror("fseek (SEEK_END) falhou em cadastrar()");
        return;
    }

    size_t escritos = fwrite(&p, sizeof(Produto), 1, fp);
    if (escritos != 1) {
        perror("fwrite falhou em cadastrar()");
        return;
    }

    /* garantir que os dados vão para o disco */
    fflush(fp);

    printf("Produto cadastrado com sucesso!\n");
}

/* Consulta um registro por índice (posição) */
void consultar(FILE *fp) {
    if (fp == NULL) {
        printf("Arquivo nao esta aberto para consulta.\n");
        return;
    }

    long total = tamanho(fp);
    if (total == 0) {
        printf("Arquivo vazio. Nenhum registro para consultar.\n");
        return;
    }

    printf("Há %ld registros. Informe o indice (0 .. %ld): ", total, total - 1);
    long idx;
    if (scanf("%ld", &idx) != 1) {
        printf("Indice invalido.\n");
        limpaBuffer();
        return;
    }
    limpaBuffer();

    if (idx < 0 || idx >= total) {
        printf("Indice fora da faixa.\n");
        return;
    }

    /* posiciona no registro desejado */
    long desloc = idx * (long)sizeof(Produto);
    if (fseek(fp, desloc, SEEK_SET) != 0) {
        perror("fseek (SEEK_SET) falhou em consultar()");
        return;
    }

    Produto p;
    size_t lidos = fread(&p, sizeof(Produto), 1, fp);
    if (lidos != 1) {
        perror("fread falhou em consultar()");
        return;
    }

    printf("\n--- Registro %ld ---\n", idx);
    printf("Nome  : %s\n", p.nome);
    printf("Codigo: %d\n", p.codigo);
    printf("Preco : %.2f\n", p.preco);
    printf("-------------------\n");
}

/* ----------- main: abre/cria arquivo binário e menu ----------- */
int main(void) {
    FILE *fp = fopen(NOME_ARQUIVO, "r+b"); /* tenta abrir para leitura/escrita binária */
    if (fp == NULL) {
        /* se não existe, cria com w+b e reabre com r+b */
        fp = fopen(NOME_ARQUIVO, "w+b");
        if (fp == NULL) {
            perror("Nao foi possivel criar o arquivo");
            return EXIT_FAILURE;
        }
        /* já aberto em w+b, podemos continuar */
    }

    int opcao = -1;
    while (opcao != 0) {
        printf("\n=== Gerenciador de Estoque ===\n");
        printf("1 - Cadastrar produto\n");
        printf("2 - Consultar por indice\n");
        printf("3 - Mostrar numero de registros\n");
        printf("4 - Listar todos (opcional)\n");
        printf("0 - Sair\n");
        printf("Escolha: ");
        if (scanf("%d", &opcao) != 1) {
            printf("Entrada invalida.\n");
            limpaBuffer();
            continue;
        }
        limpaBuffer();

        switch (opcao) {
            case 1:
                cadastrar(fp);
                break;
            case 2:
                consultar(fp);
                break;
            case 3: {
                long tot = tamanho(fp);
                printf("Total de registros no arquivo: %ld\n", tot);
                break;
            }
            case 4: {
    long tot = tamanho(fp);
    if (tot == 0) {
        printf("Arquivo vazio.\n");
    } else {
        long i;  // <<--- declare AQUI
        printf("\n=== Listagem de todos os registros (%ld) ===\n", tot);
        for (i = 0; i < tot; i++) {
            if (fseek(fp, i * (long)sizeof(Produto), SEEK_SET) != 0) {
                perror("fseek falhou na listagem");
                break;
            }
            Produto p;
            if (fread(&p, sizeof(Produto), 1, fp) != 1) {
                perror("fread falhou na listagem");
                break;
            }
            printf("[%ld] Nome: %s | Codigo: %d | Preco: %.2f\n",
                   i, p.nome, p.codigo, p.preco);
        }
        printf("=========================================\n");
    }
    break;
}
            case 0:
                printf("Saindo...\n");
                break;
            default:
                printf("Opcao invalida.\n");
                break;
        }
    }

    fclose(fp);
    return 0;
}

