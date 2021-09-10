#include "multiset.h"

#ifndef COMPARING_H
#define COMPARING_H

// Funkcja, która znajduje i wypisuje podobne wiersze
extern void findSimilar(multiset *set, size_t size);

// Funkcja, która sortuje wszystkie multizbiory
extern multiset *sortAll(multiset *set, size_t size);

#endif //COMPARING_H
