#include <stdio.h>

#ifndef MULTISET_H
#define MULTISET_H

/**
 * Główna struktura, na której oparty jest program - multizbiór.
 * Struktura multizbioru przechowująca obiekty różnych typów - tzw. słowa.
 * unsigInts - nieujemne liczby całkowite w zbiorze
 * sigInts - ujemne liczby całkowite w zbiorze
 * anyFloats - liczby zmiennoprzecinkowe w zbiorze
 * notNumbers - "nieliczby" w zbiorze
 * size* - ilość elementów poszczególnych typów w zbiorze
 * maxSize* - pamięć przydzielona poszczególnym typom słów
 * lineCount - numer wiersza reprezentowanego przez multizbiór
 */
struct multiset {
    unsigned long long *unsigInts;
    long long *sigInts;
    long double *anyFloats;
    char **notNumbers;
    size_t sizeUnsigInts, sizeSigInts, sizeAnyFloats, sizeNotNumbers;
    size_t maxSizeUnsigInts, maxSizeSigInts, maxSizeAnyFloats, maxSizeNotNumbers;
    size_t lineCount;
};
typedef struct multiset multiset;

#endif //MULTISET_H
