#include "multiset.h"

#ifndef PARSING_H
#define PARSING_H

// Uniwersalna funkcja realokująca pamięć dla elementów dowolnego typu
extern void *expand(void *x, size_t typeSize, size_t current, size_t *reserved);

// Funkcja przetwarzająca dane słowo i przekazująca multizbiór z nim w środku
extern multiset processWord(multiset set, char *word, size_t wordSize);

#endif //PARSING_H