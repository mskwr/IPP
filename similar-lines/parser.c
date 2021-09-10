// Flaga potrzebna do poprawnego działania funkcji getline
#define _GNU_SOURCE

#include "parser.h"
#include "multiset.h"
#include "recognizer.h"
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

// Stała zwracana przez getline, po wczytaniu ostatniego wiersza
#define END (-1)

/**
 * Funkcja, która inicjalizuje zadany multizbiór.
 * x - multizbiór, który trzeba zainicjalizować
 */
static multiset initializeMultiset(multiset x) {
    x.unsigInts = NULL;
    x.sigInts = NULL;
    x.anyFloats = NULL;
    x.notNumbers = NULL;
    x.sizeUnsigInts = 0;
    x.sizeSigInts = 0;
    x.sizeAnyFloats = 0;
    x.sizeNotNumbers = 0;
    x.maxSizeUnsigInts = 0;
    x.maxSizeSigInts = 0;
    x.maxSizeAnyFloats = 0;
    x.maxSizeNotNumbers = 0;
    x.lineCount = 0;

    return x;
}

/**
 * Funckja, która zmienia wszystkie duże litery w tablicy na małe.
 * word - tablica znaków, której duże litery zostaną zmniejszone
 * size - rozmiar dostarczonej tablicy znaków
 */
static char *convertBigLetters(char *word, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (word[i] >= 'A' && word[i] <= 'Z')
            word[i] = (char) tolower(word[i]);
    }

    return word;
}

/**
 * Funkcja, która przetwarza cały wiersz w multizbiór.
 * Inicjalizuje nowy multizbiór i z danej linii wyodrębnia wszystkie
 * słowa. Podaje je do przetworzenia, aby rozpoznać ich typy i zwraca
 * kompletny multizbiór, reprezentujący dany wiersz.
 * line - wskaźnik przechowujący wszystkie znaki z wiersza
 * count - numer wiersza z danych wejściowych
 */
static multiset createMultiset(char *line, size_t count) {
    size_t wordSize;
    multiset set = initializeMultiset(set);

    // Białe znaki, dziękim którym funkcja strtok wie, jak wyodrębniać słowa
    char *whitespaces = " \t\n\v\f\r";
    // Funkcja strtok wyodrębnia podciągi z danego ciągu
    char *word = strtok(line, whitespaces);

    while (word != NULL) {
        wordSize = strlen(word);
        word = convertBigLetters(word, wordSize);
        // Funkcja przetwarzająca słowa - główna funkcja modułu "recognizer.h"
        set = processWord(set, word, wordSize);

        // Aby funkcja szukała następnego słowa od ostatniego zakończenia
        word = strtok(NULL, whitespaces);
    }

    set.lineCount = count;
    return set;
}

/**
 * Funkcja zwracająca prawdę, gdy dany znak jest znakiem białym i fałsz wpp.
 * x - znak do sprawdzenia
 */
static bool isWhitespace(char x) {
    if (x == ' ' || x == '\t' || x == '\v'
        || x == '\f' || x == '\r' || x == '\n')
        return true;
    else
        return false;
}

/**
 * Funkcja zwracająca prawdę, gdy dany znak jest nielegalny i fałsz wpp.
 * x - znak do sprawdzenia
 */
static bool isIllegalSign(char x) {
    if (x < 9 || x > 126 || (x > 13 && x < 32))
        return true;
    else
        return false;
}

/**
 * Funkcja sprawdzająca, czy program powinien ignorować daną linię.
 * Najpierw sprawdza, czy zadana linia nie jest komentarzem. Jeśli nie, to
 * przeszukuje ciąg znaków w linii w poszukiwaniu znaków nielegalnych i sprawdza
 * czy linia jest pusta. Zwraca prawdę, gdy wiersz należy ignorować i fałsz wpp.
 * line - wskaźnik przechowujący wszystkie znaki z wiersza.
 * size - liczba znaków w wierszu
 * count - numer wiersza
 */
static bool ignoreLine(char *line, size_t size, size_t count) {
    size_t i;
    bool blankLine = true;

    // Sprawdzenie, czy wiersz jest komentarzem
    if (size > 0 && line[0] == '#') {
        return true;
    }
    else {
        for (i = 0; i < size; i++) {
            if (isIllegalSign(line[i])) {
                // Komunikat o błędnym znaku na wyjście diagnostyczne
                fprintf(stderr, "ERROR %zu\n", count);
                return true;
            }
            
            if (blankLine && !isWhitespace(line[i])) {
            	blankLine = false;
            }
        }
    }

    return blankLine;
}

/**
 * Funkcja parsujące dane wejściowe.
 * Pobiera kolejne linie z danych wejściowych przy pomocy getline, wyodrębnia
 * z legalnych słowa, które przetwarza i zwraca zebrane w multizbiorze.
 * text - wskaźnik na multizbiory reprezentujące kolejne linie tekstu
 * currentSize - obecna liczba multizbiorów wskazywanych przez wskaźnik text
 */
multiset *loadInput(multiset *text, size_t *currentSize) {
    char *line = NULL;
    size_t reservedSize, buffSize, count;
    ssize_t read;

    // Wiersze są numerowane od 1
    count = 1;
    reservedSize = DEFAULT_SIZE;
    *currentSize = 0;

    while ((read = getline(&line, &buffSize, stdin)) != END) {
        text = expand(text, sizeof(multiset), *currentSize, &reservedSize);

        // Ignorowane linie nie są przetwarzane. Getline zwraca długość linii
        // zbyt dużą o jeden - odpowiednia korekta.
        if (!ignoreLine(line, read - 1, count)) {
            text[*currentSize] = createMultiset(line, count);
            ++*currentSize;
        }

        // Ignorowane wiersze również są numerowane
        count++;
    }

    free(line);
    return text;
}

/**
 * Funkcja zwalniająca pamięć po danym multizbiorze.
 * x - multizbiór, z którego chcemy zwolnić pamięć.
 */
void freeMultiset(multiset *x) {
    for (size_t i = 0; i < (*x).sizeNotNumbers; i++) {
        if ((*x).notNumbers[i] != NULL)
            free((*x).notNumbers[i]);
    }

    if ((*x).notNumbers != NULL)
        free((*x).notNumbers);
    if ((*x).unsigInts != NULL)
        free((*x).unsigInts);
    if ((*x).sigInts != NULL)
        free((*x).sigInts);
    if ((*x).anyFloats != NULL)
        free((*x).anyFloats);
}
