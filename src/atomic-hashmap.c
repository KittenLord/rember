#ifndef KITTENLORD_ATOMIC_HASHMAP__
#define KITTENLORD_ATOMIC_HASHMAP__

#include <stdlib.h>
#include <stdio.h> 
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// TODO: obviously needs rebalancing functionality, but ill add it later lol

struct AtomicHashmapNode {
    uint8_t                     *key;
    size_t                      keyLen;

    uint8_t                     *data;
    size_t                      dataLen;

    struct AtomicHashmapNode    *next;
};

struct AtomicHashmap {
    _Atomic bool                locked;

    size_t                      capacity;

    size_t                      tyA;
    size_t                      tyB;
    
    size_t                      maxChain;
    size_t                      count;

    struct AtomicHashmapNode    **nodes;
};

#define HM_INITCAPACITY 32
#define createHM_SX(ty) createHMAll(HM_INITCAPACITY, sizeof(ty), 0)
#define createHM_XS(ty) createHMAll(HM_INITCAPACITY, 0, sizeof(ty))
#define createHM_SS(tyA, tyB) createHMAll(HM_INITCAPACITY, sizeof(tyA), sizeof(tyB))
#define createHM() createHMAll(HM_INITCAPACITY, 0, 0)
#define createHMAll(cap, a, b) ((struct AtomicHashmap){ \
    .locked = false, \
    .capacity = cap, \
    .maxChain = 0, \
    .count = 0, \
    .tyA = a, \
    .tyB = b, \
    .nodes = calloc(cap, sizeof(struct AtomicHashmapNode *)) \
})

// Please be responsible :D
void acquireLock(struct AtomicHashmap *hm);
void releaseLock(struct AtomicHashmap *hm);

#define getHM_SX(hm, key, dataLen) getHM(hm, (uint8_t *)(key), (hm)->tyA, dataLen)
#define getHM_XS(hm, key, keyLen) getHM(hm, (uint8_t *)(key), keyLen, NULL)
#define getHM_SS(hm, key) getHM(hm, (uint8_t *)(key), (hm)->tyA, NULL)
uint8_t *getHM(struct AtomicHashmap *hm, uint8_t *key, size_t keyLen, size_t *dataLen);

#define setHM_SX(hm, key, data, dataLen) setHM(hm, (uint8_t *)(key), (hm)->tyA, (uint8_t *)(data), dataLen)
#define setHM_XS(hm, key, keyLen, data) setHM(hm, (uint8_t *)(key), keyLen, (uint8_t *)(data), (hm)->tyB)
#define setHM_SS(hm, key, data) setHM(hm, (uint8_t *)(key), (hm)->tyA, (uint8_t *)(data), (hm)->tyB)
#define setHM_LL(hm, key, data) setHM(hm, key, strlen(key), data, strlen(data))
void setHM(struct AtomicHashmap *hm, uint8_t *key, size_t keyLen, uint8_t *data, size_t dataLen);

bool removeHM(struct AtomicHashmap *hm, uint8_t *key, size_t keyLen);

// https://github.com/fabiogaluppo/fnv/blob/main/fnv64.hpp
// from here
uint64_t fnv64hash(uint8_t *data, size_t len) {
    uint64_t h = 0xcbf29ce484222325ULL;
    while (len--) {
        h += (h << 1) + (h << 4) + (h << 5) + (h << 7) + (h << 8) + (h << 40);
        h ^= (uint64_t)(*data++);
    }
    return h;
}
// to here

void acquireLock(struct AtomicHashmap *hm) { 
    while(hm->locked);  
    hm->locked = true;
}

void releaseLock(struct AtomicHashmap *hm) {
    hm->locked = false;
}

void trySetMaxChain(struct AtomicHashmap *hm, size_t value) {
    if(hm->maxChain < value) hm->maxChain = value;
}

void __freeHMNode(struct AtomicHashmapNode *node) {
    free(node->key);
    free(node->data);
    free(node);
}

bool __nodeMatchesKey(struct AtomicHashmapNode *node, uint8_t *key, size_t keyLen) {
    return node->keyLen == keyLen && !memcmp(node->key, key, keyLen);
}

bool removeHM(struct AtomicHashmap *hm, uint8_t *key, size_t keyLen) {
    size_t hash = fnv64hash(key, keyLen);
    size_t index = hash % hm->capacity;

    struct AtomicHashmapNode *node = hm->nodes[index];
    if(__nodeMatchesKey(node, key, keyLen)) {
        hm->nodes[index] = node->next;
        __freeHMNode(node);
        hm->count--;
        return true;
    }

    while(node->next) {
        struct AtomicHashmapNode *next = node->next;
        if(__nodeMatchesKey(next, key, keyLen)) {
            node->next = next->next;
            __freeHMNode(next);
            hm->count--;
            return true;
        }
    }

    return false;
}

uint8_t *getHM(struct AtomicHashmap *hm, uint8_t *key, size_t keyLen, size_t *dataLen) {
    size_t hash = fnv64hash(key, keyLen);
    size_t index = hash % hm->capacity;

    struct AtomicHashmapNode *node = hm->nodes[index];
    while(node) {
        if(__nodeMatchesKey(node, key, keyLen)) {
            if(dataLen) *dataLen = node->dataLen;
            return node->data;
        }

        node = node->next;
    }

    return NULL;
}

void setHM(struct AtomicHashmap *hm, uint8_t *key, size_t keyLen, uint8_t *data, size_t dataLen) {
    size_t hash = fnv64hash(key, keyLen);
    size_t index = hash % hm->capacity;

    size_t maxChain = 1;
    struct AtomicHashmapNode **insertNode = &(hm->nodes[index]);

    // definitely doesnt exist
    if(!*insertNode) { goto insert; }

    do {
        struct AtomicHashmapNode *node = *insertNode;
        if(__nodeMatchesKey(node, key, keyLen)) {
            free(node->data);
            node->data = calloc(sizeof(uint8_t), dataLen + 1);
            node->dataLen = dataLen;
            memcpy(node->data, data, dataLen);
            return;
        }

        insertNode = &(node->next);
        maxChain++;
    }
    while(*insertNode);
    goto insert;

insert:
    struct AtomicHashmapNode *node = malloc(sizeof(struct AtomicHashmapNode));
    *node = (struct AtomicHashmapNode){
        .key = calloc(sizeof(uint8_t), keyLen + 1),
        .keyLen = keyLen,
        .data = calloc(sizeof(uint8_t), dataLen + 1),
        .dataLen = dataLen,
        .next = NULL
    };
    memcpy(node->key, key, keyLen);
    memcpy(node->data, data, dataLen);
    *insertNode = node;
    trySetMaxChain(hm, maxChain);
    hm->count++;
}

#else
#endif
