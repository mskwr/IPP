#include "multiset.h"

#ifndef INPUT_H
#define INPUT_H

// Początkowy rozmiar przydzielanej wolnej pamięci
#define DEFAULT_SIZE 32

// Funkcja parsująca dane wejściowe i odpowiednio przetwarzająca wiersze
// w tablicę multizbiorów, dynamicznie przydzielając wolną pamięć
extern multiset *loadInput(multiset *text, size_t *currentSize);

// Funkcja zwalniająca pamięć po wszystkich multizbiorach
extern void freeMultiset(multiset *x);

#endif //INPUT_H
