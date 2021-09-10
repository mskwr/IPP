/**
 * Małe zadanie z IPP - program wyszukujący podobne grupy wierszy
 * Autor: Michał Skwarek
 */

#include "multiset.h"
#include "parser.h"
#include "similar.h"
#include <stdlib.h>

int main() {
    size_t size;
    // Główny element programu - tablica multizbiorów, która będzie
    // przechowywać wszystkie slowa z kolejnych linii danych wejściowych
    multiset *text = malloc(DEFAULT_SIZE * sizeof(multiset));

    // Awaryjne wyjście z programu w przypadku braku pamięci
    if (text == NULL)
    	exit(1);

    // Parsowanie danych wejściowych
    text = loadInput(text, &size);

    // Porównywanie i wypisywanie podobnych multizbiorów
    text = sortAll(text, size);
    findSimilar(text, size);

    // Zwalnianie pamięci po wszystkich wytworzonych multizbiorach
    for (size_t i = 0; i < size; i++)
        freeMultiset(&(text[i]));

    free(text);

    return 0;
}
