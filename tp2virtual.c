#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Estruturas de dados para simular as páginas e a tabela de páginas
typedef struct Page {
    unsigned int frameNumber; // Número do quadro físico
    bool referenced;         // Bit de referência
    bool modified;           // Bit modificado (sujo)
    struct Page* next;       // Ponteiro para a próxima página na lista
} Page;

// Variáveis globais
Page *pageList = NULL; // Lista de páginas na memória
unsigned int pageSize; // Tamanho da página em KB
unsigned int memorySize; // Tamanho total da memória em KB
unsigned int numPages; // Número total de páginas na memória
unsigned int pageFaults = 0; // Contador de page faults
unsigned int writebacks = 0; // Contador de páginas sujas escritas de volta
unsigned int totalAccesses = 0; // Contador de acessos totais
unsigned int pageHits = 0; // Contador de acertos de página

// Função para determinar o número da página a partir do endereço
unsigned int getPageNumber(unsigned int address) {
    return address >> (31 - __builtin_clz(pageSize * 1024 - 1)); // Calcula o número da página
}

// Função para encontrar uma página na lista de páginas
Page* findPage(unsigned int pageNumber) {
    Page *current = pageList;
    while (current != NULL) {
        if (current->frameNumber == pageNumber) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Função para adicionar uma nova página na lista
void addPage(unsigned int pageNumber, bool modified) {
    Page *newPage = (Page*)malloc(sizeof(Page));
    newPage->frameNumber = pageNumber;
    newPage->referenced = true;
    newPage->modified = modified;
    newPage->next = pageList;
    pageList = newPage;
}

// Função para substituir uma página usando LRU (Least Recently Used)
void replacePageLRU(unsigned int pageNumber, bool modified) {
    Page *current = pageList;
    Page *previous = NULL;
    Page *lruPage = pageList;
    Page *lruPrev = NULL;

    // Encontrar a página LRU (Menos Recentemente Usada)
    while (current != NULL) {
        if (current->referenced == 0) {
            lruPage = current;
            lruPrev = previous;
            break;
        }
        previous = current;
        current = current->next;
    }

    if (lruPage == pageList) {
        // Substituição na cabeça da lista
        pageList = pageList->next;
    } else {
        lruPrev->next = lruPage->next;
    }
    
    // Verificar se a página substituída foi modificada
    if (lruPage->modified) {
        writebacks++;
    }
    
    // Adicionar a nova página
    lruPage->frameNumber = pageNumber;
    lruPage->referenced = true;
    lruPage->modified = modified;
    lruPage->next = pageList;
    pageList = lruPage;
}

// Função para substituir uma página usando Segunda Chance
void replacePageSecondChance(unsigned int pageNumber, bool modified) {
    Page *current = pageList;
    Page *previous = NULL;

    while (current != NULL) {
        if (current->referenced == 0) {
            // Substituir a página
            if (previous != NULL) {
                previous->next = current->next;
            } else {
                pageList = current->next;
            }

            if (current->modified) {
                writebacks++;
            }
            
            // Adicionar nova página
            current->frameNumber = pageNumber;
            current->referenced = true;
            current->modified = modified;
            current->next = pageList;
            pageList = current;
            return;
        } else {
            current->referenced = 0; // Dar segunda chance
        }
        previous = current;
        current = current->next;
    }
}

// Função para substituir uma página usando NRU (Not Recently Used)
void replacePageNRU(unsigned int pageNumber, bool modified) {
    Page *current = pageList;
    Page *previous = NULL;
    Page *nruPage = NULL;
    Page *nruPrev = NULL;

    while (current != NULL) {
        if (current->referenced == 0) {
            if (nruPage == NULL || (current->modified < nruPage->modified)) {
                nruPage = current;
                nruPrev = previous;
            }
        }
        previous = current;
        current = current->next;
    }

    if (nruPage != NULL) {
        if (nruPrev != NULL) {
            nruPrev->next = nruPage->next;
        } else {
            pageList = nruPage->next;
        }

        if (nruPage->modified) {
            writebacks++;
        }
        
        // Adicionar nova página
        nruPage->frameNumber = pageNumber;
        nruPage->referenced = true;
        nruPage->modified = modified;
        nruPage->next = pageList;
        pageList = nruPage;
    }
}

// Função para processar uma linha de acesso
void processAccess(unsigned int address, char accessType, char *algorithm) {
    unsigned int pageNumber = getPageNumber(address);
    bool modified = (accessType == 'W');

    Page *page = findPage(pageNumber);
    if (page != NULL) {
        pageHits++;
        page->referenced = true;
        if (strcmp(algorithm, "lru") == 0) {
            // Mover a página para o início da lista (se necessário)
        }
    } else {
        pageFaults++;
        if (pageList != NULL && numPages > 0) {
            if (strcmp(algorithm, "lru") == 0) {
                replacePageLRU(pageNumber, modified);
            } else if (strcmp(algorithm, "segundaChance") == 0) {
                replacePageSecondChance(pageNumber, modified);
            } else if (strcmp(algorithm, "nru") == 0) {
                replacePageNRU(pageNumber, modified);
            }
        } else {
            // Adicionar a nova página
            addPage(pageNumber, modified);
        }
    }
}


// Função principal
int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Uso: %s <algoritmo> <arquivo> <tamanho_pagina> <tamanho_memoria>\n", argv[0]);
        return 1;
    }

    char *algorithm = argv[1];
    char *fileName = argv[2];
    pageSize = atoi(argv[3]);
    memorySize = atoi(argv[4]);

    numPages = memorySize / pageSize;
    FILE *file = fopen(fileName, "r");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        return 1;
    }

    unsigned int address;
    char accessType;
    while (fscanf(file, "%x %c", &address, &accessType) == 2) {
        totalAccesses++;
        processAccess(address, accessType,algorithm);
    }

    fclose(file);

    printf("\nExecutando o simulador...\n");
    printf("Arquivo de entrada: %s\n", fileName);
    printf("Tamanho da memória: %d KB\n", memorySize);
    printf("Tamanho das páginas: %d KB\n", pageSize);
    printf("Técnica de reposição: %s\n", algorithm);
    printf("Número total de acessos: %d\n", totalAccesses);
    printf("Número de page faults: %d\n", pageFaults);
    printf("Número de writebacks: %d\n", writebacks);

    // Liberar memória
    while (pageList != NULL) {
        Page *temp = pageList;
        pageList = pageList->next;
        free(temp);
    }

    return 0;
}
