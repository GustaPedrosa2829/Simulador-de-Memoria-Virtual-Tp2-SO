#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Estrutura da Página
struct Pagina {
    char endereco[9];
    bool referencia;
    bool modificada;
    struct Pagina *proxima;
};

// Variáveis globais
char *algoritmo, *caminhoArquivo, linha[20], enderecoTmp[9];
int tamanhoPagina, tamanhoMemoria, numeroPaginas, operacoes=0, leituras=0, escritas=0, acertos=0, faltas=0, writebacks=0, paginasUsadas=0;
float taxaFaltas=0;
FILE *arquivo;
struct Pagina *primeiraPagina, *ultimaPagina;

// Função para inserir uma nova página
void inserirPagina(char endereco[9], bool modificada) {
    struct Pagina *novaPagina = (struct Pagina*)malloc(sizeof(struct Pagina));
    strcpy(novaPagina->endereco, endereco);
    novaPagina->referencia = true;  // Nova página tem o bit de referência como true
    novaPagina->modificada = modificada;
    novaPagina->proxima = NULL;
    
    if (paginasUsadas == 0) {
        primeiraPagina = novaPagina;
        ultimaPagina = novaPagina;
    } else {
        ultimaPagina->proxima = novaPagina;
        ultimaPagina = novaPagina;
    }
    
    if (paginasUsadas < numeroPaginas)
        paginasUsadas++;
    
    escritas++;
}

// Algoritmo LRU (Least Recently Used)
void LRU(char endereco[9]) {
    struct Pagina *temp = primeiraPagina, *paginaAnterior = NULL;

    // Percorre até o penúltimo elemento da lista
    while (temp->proxima != NULL) {
        paginaAnterior = temp;
        temp = temp->proxima;
    }

    // Se apenas uma página estiver presente
    if (paginaAnterior == NULL) {
        strcpy(temp->endereco, endereco);
        temp->referencia = true;
        temp->modificada = (linha[9] == 'W' || linha[9] == 'w');
    } else {
        // Substitui a última página
        if (temp == ultimaPagina) {
            ultimaPagina = paginaAnterior;
        }
        ultimaPagina->proxima = NULL;

        // Mover página para o início
        temp->proxima = primeiraPagina;
        primeiraPagina = temp;

        strcpy(temp->endereco, endereco);
        temp->referencia = true;
        temp->modificada = (linha[9] == 'W' || linha[9] == 'w');
    }
}

// Algoritmo NRU (Not Recently Used)
void NRU() {
    struct Pagina *temp = primeiraPagina, *paginaSubstituir = NULL;
    int classe = 4;  // Inicializa com uma classe inválida

    while (temp != NULL) {
        int classeAtual = temp->referencia * 2 + temp->modificada;
        if (classeAtual < classe) {
            classe = classeAtual;
            paginaSubstituir = temp;
        }
        temp = temp->proxima;
    }

    if (paginaSubstituir != NULL) {
        strcpy(paginaSubstituir->endereco, enderecoTmp);
        paginaSubstituir->referencia = true;
        paginaSubstituir->modificada = (linha[9] == 'W' || linha[9] == 'w');
    }
}

// Algoritmo Segunda Chance
void segundaChance() {
    while (primeiraPagina != NULL) {
        if (primeiraPagina->referencia == 0) {
            // Substituir a página
            struct Pagina *paginaSubstituir = primeiraPagina;
            strcpy(paginaSubstituir->endereco, enderecoTmp);
            paginaSubstituir->referencia = true;
            paginaSubstituir->modificada = (linha[9] == 'W' || linha[9] == 'w');
            break;
        } else {
            // Dar uma segunda chance e mover para o fim da fila
            primeiraPagina->referencia = 0;
            ultimaPagina->proxima = primeiraPagina;
            ultimaPagina = primeiraPagina;
            primeiraPagina = primeiraPagina->proxima;
            ultimaPagina->proxima = NULL;
        }
    }
}

// Função para encontrar uma página e, se necessário, movê-la
bool encontrarPagina(char endereco[9]) {
    struct Pagina *temp = primeiraPagina, *paginaAnterior = NULL;
    while (temp != NULL) {
        if (strcmp(temp->endereco, endereco) == 0) {
            temp->referencia = true;  // Atualiza o bit de referência

            // Movendo para o final (simulando o LRU)
            if (strcmp(algoritmo, "lru") == 0) {
                if (paginaAnterior != NULL) {
                    paginaAnterior->proxima = temp->proxima;
                    ultimaPagina->proxima = temp;
                    ultimaPagina = temp;
                    temp->proxima = NULL;
                }
            }
            return true;
        }
        paginaAnterior = temp;
        temp = temp->proxima;
    }
    return false;
}

// Função para substituir uma página com base no algoritmo escolhido
void substituirPagina(char endereco[9]) {
    if (strcmp(algoritmo, "lru") == 0) {
        LRU(endereco);
    } else if (strcmp(algoritmo, "nru") == 0) {
        NRU();
    } else if (strcmp(algoritmo, "segundaChance") == 0) {
        segundaChance();
    }
    writebacks++;
}

// Função para processar uma escrita de endereço
void escreverEndereco(char endereco[9]) {
    if (paginasUsadas < numeroPaginas) {
        inserirPagina(enderecoTmp, (linha[9] == 'W' || linha[9] == 'w'));
    } else {
        taxaFaltas++;
        substituirPagina(enderecoTmp);
    }
}

// Função para liberar a memória alocada para as páginas
void liberarMemoria() {
    struct Pagina *temp = primeiraPagina;
    while (temp != NULL) {
        struct Pagina *proxima = temp->proxima;
        free(temp);
        temp = proxima;
    }
    fclose(arquivo);
}

int main(int argc, char *argv[]) {
    algoritmo = argv[1];
    caminhoArquivo = argv[2];
    tamanhoPagina = atoi(argv[3]);
    tamanhoMemoria = atoi(argv[4]);

    if (tamanhoPagina < 2 || tamanhoPagina > 64) {
        printf("ERRO: O tamanho de cada página deve estar entre 2 e 64 KB.\n");
        return 0;
    }

    if (tamanhoMemoria < 128 || tamanhoMemoria > 16384) {
        printf("ERRO: O tamanho da memória deve estar entre 128 e 16384 KB.\n");
        return 0;
    }

    if (strcmp(algoritmo, "lru") != 0 && strcmp(algoritmo, "nru") != 0 && strcmp(algoritmo, "segundaChance") != 0) {
        printf("ERRO: O algoritmo deve ser lru, nru ou segundaChance.\n");
        return 0;
    }

    numeroPaginas = tamanhoMemoria / tamanhoPagina;

    if (strlen(caminhoArquivo) > 0) {
        arquivo = fopen(caminhoArquivo, "r");
        if (arquivo == NULL) {
            printf("ERRO: Não foi possível abrir o arquivo.\n");
            return 0;
        }
        while (fgets(linha, 20, arquivo) != NULL) {
            operacoes++;
            strncpy(enderecoTmp, linha, 8);
            enderecoTmp[8] = '\0';
            if (linha[9] == 'W' || linha[9] == 'w') {
                escreverEndereco(enderecoTmp);
            } else if (linha[9] == 'R' || linha[9] == 'r') {
                if (encontrarPagina(enderecoTmp)) {
                    acertos++;
                } else {
                    faltas++;
                    escreverEndereco(enderecoTmp);
                }
                leituras++;
            } else {
                printf("ERRO: Os dados do arquivo de entrada estão em formato incorreto.\n");
                return 0;
            }
        }
    } else {
        printf("ERRO: Arquivo de entrada inválido.\n");
        return 0;
    }

    printf("\nExecutando o simulador...\n");
    printf("Tamanho da memória: %iKB\n", tamanhoMemoria);
    printf("Tamanho das páginas: %iKB\n", tamanhoPagina);
    printf("Técnica de reposição: %s\n", algoritmo);
    printf("Número de páginas: %i\n", numeroPaginas);
    printf("Operações no arquivo de entrada: %i\n", operacoes);
    printf("Operações de leitura: %i\n", leituras);
    printf("Operações de escrita: %i\n", escritas);
    printf("Page hits: %i\n", acertos);
    printf("Page misses: %i\n", faltas);
    printf("Número de writebacks: %i\n", writebacks);
    printf("Taxa de page fault: %f%% \n", taxaFaltas/escritas*100);

    liberarMemoria();

    return 0;
}
