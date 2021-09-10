#include "similar.h"
#include "multiset.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

/**
 * Funkcja porównująca dwie "nieliczby" do qsort.
 * a - pierwsza nieliczba
 * b - druga nieliczba
 */
static int compareNotNumbers(const void *a, const void *b) {
    const char **x = (const char **) a;
    const char **y = (const char **) b;
    return strcmp(*x, *y);
}

/**
 * Funkcja porównująca dwie liczby do qsort.
 * a - pierwsza liczba
 * b - druga liczba
 */
static int compareAnyFloats(const void *a, const void *b) {
    long double x = *((long double*) a);
    long double y = *((long double*) b);
    if (x < y)
        return -1;
    else if (x > y)
        return 1;
    return 0;
}

/**
 * Funkcja porównująca dwie liczby do qsort.
 * a - pierwsza liczba
 * b - druga liczba
 */
static int compareSigInts(const void *a, const void *b) {
    long long x = *((long long*) a);
    long long y = *((long long*) b);
    if (x < y)
        return -1;
    else if (x > y)
        return 1;
    return 0;
}

/**
 * Funkcja porównująca dwie liczby do qsort.
 * a - pierwsza liczba
 * b - druga liczba
 */
static int compareUnsigInts(const void *a, const void *b) {
    unsigned long long x = *((unsigned long long*) a);
    unsigned long long y = *((unsigned long long*) b);
    if (x < y)
        return -1;
    else if (x > y)
        return 1;
    return 0;
}

/**
 * Funkcja sprawdzająca czy dwa multizbioru składają się z tych samych nieliczb.
 * set1 - pierwszy multizbiór
 * set2 - drugi multizbiór
 */
static bool similarNotNumbers(multiset set1, multiset set2) {
    for (size_t j = 0; j < set1.sizeNotNumbers; j++) {
        if (strcmp(set1.notNumbers[j], set2.notNumbers[j]) != 0)
            return false;
    }

    return true;
}

/**
 * Funkcja sprawdzająca, czy dwa multizbiory mają dokładnie tyle samo elementów
 * każdego z typów przechowywanych przez multizbiór.
 * set1 - pierwszy multizbiór
 * set2 - drugi multizbiór
 */
static bool similarSizes(multiset set1, multiset set2) {
    if (set1.sizeUnsigInts != set2.sizeUnsigInts ||
        set1.sizeSigInts != set2.sizeSigInts ||
        set1.sizeAnyFloats != set2.sizeAnyFloats ||
        set1.sizeNotNumbers != set2.sizeNotNumbers) {

        return false;
    }

    return true;
}

/**
 * Funkcja sprawdzająca czy dwa multizbiory składają się z tych samych liczb.
 * set1 - pierwszy multizbiór
 * set2 - drugi multizbiór
 */
static bool similarAnyFloats(multiset set1, multiset set2) {
    for (size_t j = 0; j < set1.sizeAnyFloats; j++) {
        if (set1.anyFloats[j] != set2.anyFloats[j])
            return false;
    }

    return true;
}

/**
 * Funkcja sprawdzająca czy dwa multizbiory składają się z tych samych liczb.
 * set1 - pierwszy multizbiór
 * set2 - drugi multizbiór
 */
static bool similarSigInts(multiset set1, multiset set2) {
    for (size_t j = 0; j < set1.sizeSigInts; j++) {
        if (set1.sigInts[j] != set2.sigInts[j])
            return false;
    }

    return true;
}

/**
 * Funkcja sprawdzająca czy dwa multizbiory składają się z tych samych liczb.
 * set1 - pierwszy multizbiór
 * set2 - drugi multizbiór
 */
static bool similarUnsigInts(multiset set1, multiset set2) {
    for (size_t j = 0; j < set1.sizeUnsigInts; j++) {
        if (set1.unsigInts[j] != set2.unsigInts[j])
            return false;
    }

    return true;
}

/**
 * Funkcja sprawdzająca czy dwa multizbiory są podobne.
 */
static bool similarSets(multiset set1, multiset set2) {
    return (similarSizes(set1, set2) && similarUnsigInts(set1, set2) &&
            similarSigInts(set1, set2) && similarAnyFloats(set1, set2) &&
            similarNotNumbers(set1, set2));
}

/**
 * Funkcja wypisująca wszystkie podobne multizbiory zgodnie ze specyfikacją.
 * set - wskaźnik na wszystkie multizbiory
 * size - ilość wszystkich multizbiorów
 */
void findSimilar(multiset *set, size_t size) {
    size_t i, j;
    // Tablica mówiąca, czy dany multizbiór był już wypisany
    bool visited[size];

    // Inicjalizacja tablicy na false
    for (i = 0; i < size; i++ )
        visited[i] = false;

    for (i = 0; i < size; i++) {
        if (!visited[i]) {
            printf("%zu", set[i].lineCount);

            for (j = i + 1; j < size; j++) {
                if (!visited[j]) {
                    if (similarSets(set[i], set[j])) {
                        printf(" %zu", set[j].lineCount);
                        visited[j] = true;
                    }
                }
            }

            visited[i] = true;
            printf("\n");
        }
    }
}

/**
 * Funkcja sortująca wszystkie multizbiory.
 * set - wskaźnik na wszystkie multizbiory
 * size - ilość multizbiorów
 */
multiset *sortAll(multiset *set, size_t size) {
    for (size_t i = 0; i < size; i++) {
        qsort(set[i].unsigInts, set[i].sizeUnsigInts,
              sizeof(unsigned long long), compareUnsigInts);

        qsort(set[i].sigInts, set[i].sizeSigInts,
              sizeof(long long), compareSigInts);

        qsort(set[i].anyFloats, set[i].sizeAnyFloats,
              sizeof(long double), compareAnyFloats);

        qsort(set[i].notNumbers, set[i].sizeNotNumbers,
              sizeof(set[i].notNumbers), compareNotNumbers);
    }

    return set;
}
