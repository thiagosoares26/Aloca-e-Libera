#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#define TAM_HEAP   (16 * 1024)
#define ALINHAMENTO 16

typedef struct bloco {
    size_t tam;   
    size_t livre; 
} bloco_t;

#define TAM_CAB ((sizeof(bloco_t) + ALINHAMENTO - 1) & ~(size_t)(ALINHAMENTO - 1))

static _Alignas(ALINHAMENTO) unsigned char heap[TAM_HEAP];
static int heap_iniciado = 0;

static size_t alinha(size_t n) {
    return (n + ALINHAMENTO - 1) & ~(size_t)(ALINHAMENTO - 1);
}

static void heap_init(void) {
    bloco_t *b = (bloco_t *)heap;
    b->tam = TAM_HEAP - TAM_CAB;
    b->livre = 1;
    heap_iniciado = 1;
}

static bloco_t *proximo(bloco_t *b) {
    unsigned char *p = (unsigned char *)b + TAM_CAB + b->tam;
    return (p < heap + TAM_HEAP) ? (bloco_t *)p : NULL;
}

void *aloca(size_t tam) {
    if (!heap_iniciado) heap_init();
    if (tam == 0) return NULL;
    tam = alinha(tam);

    for (bloco_t *b = (bloco_t *)heap; b != NULL; b = proximo(b)) {
        if (!b->livre || b->tam < tam) continue;

        if (b->tam >= tam + TAM_CAB + ALINHAMENTO) {
            bloco_t *novo = (bloco_t *)((unsigned char *)b + TAM_CAB + tam);
            novo->tam = b->tam - tam - TAM_CAB;
            novo->livre = 1;
            b->tam = tam;
        }
        b->livre = 0;
        return (unsigned char *)b + TAM_CAB;
    }
    return NULL; 
}

void libera(void *ptr) {
    if (ptr == NULL) return;
    unsigned char *p = (unsigned char *)ptr;
    if (p < heap + TAM_CAB || p >= heap + TAM_HEAP) return; /

    bloco_t *b = (bloco_t *)(p - TAM_CAB);
    if (b->livre) return; 
    b->livre = 1;

    for (bloco_t *cur = (bloco_t *)heap; cur != NULL; ) {
        bloco_t *nx = proximo(cur);
        if (cur->livre && nx != NULL && nx->livre) {
            cur->tam += TAM_CAB + nx->tam;
        } else {
            cur = nx;
        }
    }
}

typedef struct no {
    int valor;
    struct no *ant;
    struct no *prox;
} no_t;

typedef struct {
    no_t *inicio;
    no_t *fim;
    size_t tam;
} lista_t;

void lista_init(lista_t *l) {
    l->inicio = l->fim = NULL;
    l->tam = 0;
}

static no_t *novo_no(int valor) {
    no_t *n = aloca(sizeof(no_t));
    if (n == NULL) return NULL;
    n->valor = valor;
    n->ant = n->prox = NULL;
    return n;
}

int lista_insere_inicio(lista_t *l, int valor) {
    no_t *n = novo_no(valor);
    if (!n) return -1;
    n->prox = l->inicio;
    if (l->inicio) l->inicio->ant = n; else l->fim = n;
    l->inicio = n;
    l->tam++;
    return 0;
}

int lista_insere_fim(lista_t *l, int valor) {
    no_t *n = novo_no(valor);
    if (!n) return -1;
    n->ant = l->fim;
    if (l->fim) l->fim->prox = n; else l->inicio = n;
    l->fim = n;
    l->tam++;
    return 0;
}

int lista_remove(lista_t *l, int valor) {
    for (no_t *n = l->inicio; n != NULL; n = n->prox) {
        if (n->valor != valor) continue;
        if (n->ant) n->ant->prox = n->prox; else l->inicio = n->prox;
        if (n->prox) n->prox->ant = n->ant; else l->fim = n->ant;
        libera(n);
        l->tam--;
        return 0;
    }
    return -1;
}

void lista_imprime(const lista_t *l) {
    printf("[");
    for (no_t *n = l->inicio; n; n = n->prox)
        printf("%d%s", n->valor, n->prox ? " <-> " : "");
    printf("] (tam=%zu)\n", l->tam);
}

void lista_imprime_reverso(const lista_t *l) {
    printf("[");
    for (no_t *n = l->fim; n; n = n->ant)
        printf("%d%s", n->valor, n->ant ? " <-> " : "");
    printf("]\n");
}

void lista_destroi(lista_t *l) {
    no_t *n = l->inicio;
    while (n) {
        no_t *prox = n->prox;
        libera(n);
        n = prox;
    }
    lista_init(l);
}

int main(void) {
    lista_t l;
    lista_init(&l);

    for (int i = 1; i <= 5; i++) lista_insere_fim(&l, i * 10);
    lista_insere_inicio(&l, 5);
    lista_imprime(&l);
    lista_imprime_reverso(&l);

    lista_remove(&l, 30);
    lista_remove(&l, 5);
    lista_imprime(&l);

    int inseridos = 0;
    while (lista_insere_fim(&l, inseridos) == 0) inseridos++;
    printf("Inseridos até esgotar os 16K: %d nós\n", inseridos);

    lista_destroi(&l);

    for (int i = 0; i < 100; i++) lista_insere_fim(&l, i);
    printf("Após liberar, reinseriu 100 nós: tam=%zu\n", l.tam);
    lista_destroi(&l);

    return 0;
}