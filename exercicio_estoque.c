#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------- Tipos e constantes ------------------- */
typedef struct {
    char nome[50];
    int codigo;
    float preco;
} Produto;

const char *NOME_ARQUIVO = "estoque.dat";
const char *NOME_TEMP    = "estoque.tmp";
const char *NOME_REL     = "relatorio.txt";

/* ------------------- Protótipos ------------------- */
/* limpeza e leitura */
void limpaBuffer(void);
void ler_string(const char *prompt, char *buffer, size_t tamanho);

/* operações de arquivo */
long tamanho(FILE *fp);            /* retorna número de registros no arquivo */
void cadastrar(FILE *fp);          /* adiciona um registro ao final */
void consultar(FILE *fp);          /* consulta um registro por índice */
void listar_todos(FILE *fp);       /* lista todos na tela */
void excluir(FILE **pfp);          /* exclui um registro (recebe &fp para atualizar) */
void gerar_relatorio(FILE *fp);    /* cria relatorio.txt com todos os registros */

/* ------------------- Implementações ------------------- */

/* Limpa buffer de stdin até '\n' ou EOF */
void limpaBuffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

/* Lê string de forma segura com fgets e remove '\n' usando strcspn */
void ler_string(const char *prompt, char *buffer, size_t tamanho) {
    printf("%s", prompt);
    if (fgets(buffer, (int)tamanho, stdin) == NULL) {
        buffer[0] = '\0';
        return;
    }
    /* remove o newline caso exista */
    buffer[strcspn(buffer, "\n")] = '\0';

    /* se o usuário digitou mais que o buffer, limpa o resto */
    if (strlen(buffer) == tamanho - 1) {
        int c;
        if ((c = getchar()) != '\n' && c != EOF) {
            while ((c = getchar()) != '\n' && c != EOF) { }
        }
    }
}

/* Retorna o número total de registros no arquivo (0 se fp==NULL) */
long tamanho(FILE *fp) {
    long bytes;
    if (fp == NULL) return 0;
    if (fseek(fp, 0, SEEK_END) != 0) {
        perror("fseek (SEEK_END) falhou em tamanho()");
        return 0;
    }
    bytes = ftell(fp);
    if (bytes < 0) {
        perror("ftell falhou em tamanho()");
        return 0;
    }
    /* divide pelo tamanho da struct para obter número de registros */
    return bytes / (long)sizeof(Produto);
}

/* Cadastra novo produto e grava no final do arquivo */
void cadastrar(FILE *fp) {
    Produto p;
    size_t escritos;

    if (fp == NULL) {
        printf("Arquivo nao esta aberto para cadastrar.\n");
        return;
    }

    /* leitura dos campos */
    ler_string("Nome do produto: ", p.nome, sizeof(p.nome));

    printf("Codigo (inteiro): ");
    if (scanf("%d", &p.codigo) != 1) {
        printf("Entrada invalida para codigo.\n");
        limpaBuffer();
        return;
    }
    limpaBuffer();

    printf("Preco (ex: 19.90): ");
    if (scanf("%f", &p.preco) != 1) {
        printf("Entrada invalida para preco.\n");
        limpaBuffer();
        return;
    }
    limpaBuffer();

    /* posiciona no final e escreve */
    if (fseek(fp, 0, SEEK_END) != 0) {
        perror("fseek (SEEK_END) falhou em cadastrar()");
        return;
    }

    escritos = fwrite(&p, sizeof(Produto), 1, fp);
    if (escritos != 1) {
        perror("fwrite falhou em cadastrar()");
        return;
    }

    fflush(fp); /* garante escrita em disco */
    printf("Produto cadastrado com sucesso!\n");
}

/* Consulta um registro por índice (posição) */
void consultar(FILE *fp) {
    long total;
    long idx;
    Produto p;

    if (fp == NULL) {
        printf("Arquivo nao esta aberto para consulta.\n");
        return;
    }

    total = tamanho(fp);
    if (total == 0) {
        printf("Arquivo vazio. Nenhum registro para consultar.\n");
        return;
    }

    printf("Há %ld registros. Informe o indice (0 .. %ld): ", total, total - 1);
    if (scanf("%ld", &idx) != 1) {
        printf("Indice inválido.\n");
        limpaBuffer();
        return;
    }
    limpaBuffer();

    if (idx < 0 || idx >= total) {
        printf("Indice fora da faixa.\n");
        return;
    }

    /* posiciona no registro desejado e lê */
    if (fseek(fp, idx * (long)sizeof(Produto), SEEK_SET) != 0) {
        perror("fseek (SEEK_SET) falhou em consultar()");
        return;
    }
    if (fread(&p, sizeof(Produto), 1, fp) != 1) {
        perror("fread falhou em consultar()");
        return;
    }

    printf("\n--- Registro %ld ---\n", idx);
    printf("Nome  : %s\n", p.nome);
    printf("Codigo: %d\n", p.codigo);
    printf("Preco : %.2f\n", p.preco);
    printf("-------------------\n");
}

/* Lista todos os registros na tela (útil para verificação) */
void listar_todos(FILE *fp) {
    long tot;
    long i;
    Produto p;

    if (fp == NULL) {
        printf("Arquivo nao esta aberto.\n");
        return;
    }

    tot = tamanho(fp);
    if (tot == 0) {
        printf("Arquivo vazio.\n");
        return;
    }

    printf("\n=== Listagem de todos os registros (%ld) ===\n", tot);
    if (fseek(fp, 0, SEEK_SET) != 0) {
        perror("fseek falhou na listagem");
        return;
    }

    for (i = 0; i < tot; i++) {
        if (fread(&p, sizeof(Produto), 1, fp) != 1) {
            perror("fread falhou na listagem");
            return;
        }
        printf("[%ld] Nome: %s | Codigo: %d | Preco: %.2f\n", i, p.nome, p.codigo, p.preco);
    }
    printf("=========================================\n");
}

void excluir(FILE **pfp) {
    FILE *fp;
    FILE *ft;
    long tot;
    long idx;
    long i;
    Produto p;
    int conseguiu;

    if (pfp == NULL) return;
    fp = *pfp;
    if (fp == NULL) {
        printf("Arquivo nao esta aberto.\n");
        return;
    }

    tot = tamanho(fp);
    if (tot == 0) {
        printf("Arquivo vazio. Nada para excluir.\n");
        return;
    }

    printf("Ha %ld registros. Informe o indice para excluir (0 .. %ld): ", tot, tot - 1);
    if (scanf("%ld", &idx) != 1) {
        printf("Entrada invalida.\n");
        limpaBuffer();
        return;
    }
    limpaBuffer();

    if (idx < 0 || idx >= tot) {
        printf("Indice fora da faixa.\n");
        return;
    }

    /* abre arquivo temporário para escrita */
    ft = fopen(NOME_TEMP, "w+b");
    if (ft == NULL) {
        perror("Nao foi possivel criar arquivo temporario");
        return;
    }

    /* copia todos os registros exceto o índice escolhido */
    if (fseek(fp, 0, SEEK_SET) != 0) {
        perror("fseek falhou no arquivo original");
        fclose(ft);
        remove(NOME_TEMP);
        return;
    }

    conseguiu = 1;
    for (i = 0; i < tot; i++) {
        if (fread(&p, sizeof(Produto), 1, fp) != 1) {
            perror("fread falhou durante exclusao");
            conseguiu = 0;
            break;
        }
        if (i == idx) continue; /* pula o registro a excluir */
        if (fwrite(&p, sizeof(Produto), 1, ft) != 1) {
            perror("fwrite falhou no arquivo temporario");
            conseguiu = 0;
            break;
        }
    }

    fflush(ft);
    fclose(ft);

    if (!conseguiu) {
        /* tentativa de limpeza */
        remove(NOME_TEMP);
        printf("Falha ao excluir registro.\n");
        return;
    }

    /* fecha o arquivo original antes de substituir */
    fclose(fp);

    /* substitui arquivo original pelo temporário */
    if (remove(NOME_ARQUIVO) != 0) {
        perror("Falha ao remover arquivo original");
        /* tenta manter consistência: tenta reabrir o original */
        *pfp = fopen(NOME_ARQUIVO, "r+b");
        return;
    }
    if (rename(NOME_TEMP, NOME_ARQUIVO) != 0) {
        perror("Falha ao renomear arquivo temporario");
        /* tenta reabrir (pode não existir) */
        *pfp = fopen(NOME_ARQUIVO, "r+b");
        return;
    }

    /* reabre o arquivo principal em r+b e atualiza ponteiro na main */
    *pfp = fopen(NOME_ARQUIVO, "r+b");
    if (*pfp == NULL) {
        perror("Falha ao reabrir arquivo apos exclusao");
        return;
    }

    printf("Registro %ld excluido com sucesso!\n", idx);
}

/* Gera arquivo de relatório (texto) com todos os registros */
void gerar_relatorio(FILE *fp) {
    long tot;
    long i;
    Produto p;
    FILE *fr;

    if (fp == NULL) {
        printf("Arquivo nao esta aberto.\n");
        return;
    }

    tot = tamanho(fp);
    if (tot == 0) {
        printf("Arquivo vazio. Nenhum dado para relatorio.\n");
        return;
    }

    fr = fopen(NOME_REL, "w");
    if (fr == NULL) {
        perror("No foi possivel criar relatorio");
        return;
    }

    fprintf(fr, "===== RELATORIO DE PRODUTOS (%ld registros) =====\n\n", tot);

    if (fseek(fp, 0, SEEK_SET) != 0) {
        perror("fseek falhou na geracao de relatorio");
        fclose(fr);
        return;
    }

    for (i = 0; i < tot; i++) {
        if (fread(&p, sizeof(Produto), 1, fp) != 1) {
            perror("fread falhou na geracao de relatorio");
            break;
        }
        fprintf(fr, "[%ld] Nome: %s\n", i, p.nome);
        fprintf(fr, "     Codigo: %d\n", p.codigo);
        fprintf(fr, "     Preco : %.2f\n\n", p.preco);
    }

    fclose(fr);
    printf("Relatrio gerado com sucesso em '%s'!\n", NOME_REL);
}

/* ------------------- main: abre/cria arquivo binário e menu ------------------- */
int main(void) {
    FILE *fp;
    int opcao;

    /* tenta abrir arquivo existente; se não existir, cria */
    fp = fopen(NOME_ARQUIVO, "r+b");
    if (fp == NULL) {
        fp = fopen(NOME_ARQUIVO, "w+b");
        if (fp == NULL) {
            perror("Nao foi possivel criar o arquivo");
            return EXIT_FAILURE;
        }
    }

    opcao = -1;
    while (opcao != 0) {
        printf("\n=== Gerenciador de Estoque ===\n");
        printf("1 - Cadastrar produto\n");
        printf("2 - Consultar por indice\n");
        printf("3 - Mostrar numero de registros\n");
        printf("4 - Listar todos (opcional)\n");
        printf("5 - Excluir por indice\n");
        printf("6 - Gerar relatorio\n");
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
                long tot;
                tot = tamanho(fp);
                printf("Total de registros no arquivo: %ld\n", tot);
                break;
            }
            case 4:
                listar_todos(fp);
                break;
            case 5:
                excluir(&fp); /* passa &fp para que a função atualize o ponteiro */
                break;
            case 6:
                gerar_relatorio(fp);
                break;
            case 0:
                printf("Saindo...\n");
                break;
            default:
                printf("Opcao invalida.\n");
                break;
        }
    }

    if (fp != NULL) fclose(fp);
    return 0;
}

