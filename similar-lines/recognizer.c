#include "recognizer.h"
#include "multiset.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <math.h>

// Podstawy poszczególnych systemów liczbowych
#define BASE_OCTAL 8
#define BASE_DECIMAL 10
#define BASE_HEXADECIMAL 16

/**
 * Uniwersalna funkcja realokująca pamięć dla elementów dowolnego typu.
 * Sprawdza, czy potrzeba realokować pamięć i robi to, gdy jest konieczne.
 * Dodatkowo awaryjnie kończy program w przypadku braku pamięci.
 * x - element, któremu chcemy przyporządkować więcej pamięci
 * typeSize - rozmiar typu, jaki przechowuje ten element
 * current - obecny rozmiar x
 * reserved - ilość pamięci obecnie zarezerwowanej dla x
 */
void *expand(void *x, size_t typeSize, size_t current, size_t *reserved) {
    if (current >= *reserved) {
        // +1, aby bezpiecznie realokować również elementy ustawione na NULL
        *reserved = 1 + *reserved * 2;
        x = realloc(x, *reserved * typeSize);
    }

    // Awaryjne kończenie programu w przypadku braku pamięci
    if (x == NULL)
        exit(1);
    else
        return x;
}

/**
 * Funkcja, która rozpoznaje czy dany ciąg znaków jest nieskończonością.
 * Sprawdza, czy dany ciąg nie reprezentuje inf, +inf lub -inf. Wówczas
 * zwraca true - w przeciwnym przypadku false.
 * word - ciąg znaków składających się w słowo
 * size - ilość znaków w ciągu word
 */
static bool recognizeInfinity(char *word, size_t size) {
    // Nieskończoność ma zawsze 3 znaki i ewentualny "+" lub "-"
    if (size != 3 && size != 4) {
        return false;
    }
    else if (size == 4 && (word[0] == '-' || word[0] == '+')) {
        if (word[1] == 'i' && word[2] == 'n' && word[3] == 'f')
            return true;
        else
            return false;
    }
    else if (size == 3 && word[0] == 'i' && word[1] == 'n' && word[2] == 'f') {
        return true;
    }
    else {
        return false;
    }
}

/**
 * Funkcja rozpoznająca czy dany ciąg znaków jest liczbą zmiennoprzecinkową.
 * Najpierw sprawdza, czy ciąg nie jest nieskończonością. Następnie parsuje
 * cały wyraz i zgodnie z określonymi zasadami zwraca true, gdy wyraz jest
 * liczbą zmiennoprzecinkową i fałsz w przeciwnym przypadku.
 * word - ciąg znaków składający się w słowo
 * size - ilość znaków w ciągu word
 */
static bool recognizeAnyFloat(char *word, size_t size) {
    size_t i = 0;
    // Zmienna mówiąca czy w ciągu pojawiła się jakakolwiek cyfra
    bool wasAnyNumber = false;
    // Zmienne do liczenia "E" i kropek
    int countDots, countE;

    countDots = 0;
    countE = 0;

    // Nieskończoność jest traktowane jako liczba zmiennoprzecinkowa
    if (recognizeInfinity(word, size))
        return true;
    else if (size < 2 || word[0] == 'e' || word[size - 1] == 'e')
        return false;
    else if (word[0] == '-' || word[0] == '+')
        i++;

    while (i < size) {
        if (word[i] == 'e') {
            // Zawsze przed "e" musi być jakaś cyfra
            if (!wasAnyNumber)
                return false;
            else
                countE++;
        }
        else if (word[i] == '.') {
            // Kropka nigdy nie może występować po "e"
            if (countE > 0)
                return false;
            else
                countDots++;
        }
        // Z wyjątkiem pierwszego znaku "+" i "-" mogą występować tylko po "e"
        else if (word[i] == '+' || word[i] == '-') {
            if (word[i - 1] != 'e' || i == size - 1)
                return false;
        }
        else if (word[i] < '0' || word[i] > '9') {
            return false;
        }
        else {
            wasAnyNumber = true;
        }

        i++;
    }

    // Nie może być więcej niż 1 kropka lub litera e, ale któryś z tych znaków
    // musi wystąpić. Ponadto w liczbie musi być jakaś cyfra.
    return !(countE > 1 || countDots > 1
            || countE + countDots == 0 || !wasAnyNumber);
}

/**
 * Funkcja rozpoznająca czy dany ciąg znaków jest liczbą szesnastkową.
 * word - ciąg znaków składający się w słowo
 * size - ilość znaków w ciągu word
 */
static bool recognizeHex(char *word, size_t size) {
    size_t i = 0;

    // Liczba musi się zaczynać na "0x"
    if (size < 2 || word[0] != '0' || word[1] != 'x')
        return false;
    else
        i = i + 2;

    while (i < size) {
        // Dalej mogą występować same cyfry lub litery od "a" do "f"
        if ((word[i] < '0' || word[i] > '9') && (word[i] < 'a' || word[i] > 'f'))
            return false;

        i++;
    }

    return true;
}

/**
 * Funkcja rozpoznająca czy dany ciąg znaków jest liczbą całkowitą dodatnią.
  * word - ciąg znaków składający się w słowo
 * size - ilość znaków w ciągu word
 */
static bool recognizeUnsigInt(char *word, size_t size) {
    size_t i = 0;

    // Liczba oprócz samych cyfr, może mieć na początku plus
    if (word[0] == '+' && size > 1)
        i++;

    while (i < size) {
        // Dalej mogą występować tylko cyfry
        if (word[i] < '0' || word[i] > '9')
            return false;

        i++;
    }

    return true;
}

/**
 * Funkcja sprawdzająca czy dany ciąg znaków jest liczbą całkowitą ujemną.
 * word - ciąg znaków składający się w słowo
 * size - ilość znaków w ciągu word
 */
static bool recognizeSigInt(char *word, size_t size) {
    size_t i = 1;

    // Pierwszym znakiem ciągu musi być minus
    if (word[0] != '-' || size == 1)
        return false;

    while (i < size) {
        // Dalej mogą występować tylko cyfry
        if (word[i] < '0' || word[i] > '9')
            return false;

        i++;
    }

    return true;
}

/**
 * Funkcja sprawdzająca czy dany ciąg znaków jest liczbą ósemkową.
 * word - ciąg znaków składający się w słowo
 * size - ilość znaków w ciągu word
 */
static bool recognizeOctal(char *word, size_t size) {
    size_t i = 1;

    // Na początku musi wystąpić "0"
    if (word[0] != '0')
        return false;

    while (i < size) {
        // Następnie mogą się pojawiać cygry tylko od 0 do 7
        if (word[i] < '0' || word[i] > '7')
            return false;

        i++;
    }

    return true;
}

/**
 * Funkcja, która dostając nieliczbę, zwraca multizbiór z nią w środku.
 * set - multizbiór, w którym chcę umieścić słowo
 * word - ciąg znaków składający się w słowo
 * size - ilość znaków w ciągu word
 */
static multiset processNotNumber(multiset set, char *word, size_t size) {
    char *x = malloc((size + 1) * sizeof(char));

    // Awaryjne wyjście z programu w przypadku braku pamięci
    if (x == NULL)
        exit(1);

    size_t i;

    // Kopiuję słowo ze względu na charakter funkcji strtok
    for (i = 0; i < size; i++)
        x[i] = word[i];

    x[i] = '\0';

    set.notNumbers = expand(set.notNumbers, sizeof(set.notNumbers),
                            set.sizeNotNumbers, &set.maxSizeNotNumbers);

    set.notNumbers[set.sizeNotNumbers] = x;
    ++set.sizeNotNumbers;

    return set;
}

/**
 * Funkcja, która dostając liczbę przecinkową zwraca multizbiór z nią w środku.
 * set - multizbiór, w którym chcę umieścić słowo
 * word - ciąg znaków składający się w słowo
 * size - ilość znaków w ciągu word
 */
static multiset processAnyFloat(multiset set, char *word, size_t size) {
    char *ptr;
    long double x = 0;

    // Sprawdzam czy liczba jest nieskończonością i przypisuję jej wartość
    if (recognizeInfinity(word, size)) {
        if (word[0] == '-')
            x = -INFINITY;
        else
            x = INFINITY;
    }
    else {
        // Konwertuję słowo na long double
        x = strtold(word, &ptr);
    }

    if (x >= 0) {
        // Jeśli liczba zapisana zmiennoprzecinkowo jest całkowita nieujemna
        if (x - (unsigned long long) x == 0) {
            set.unsigInts = expand(set.unsigInts, sizeof(unsigned long long),
                                   set.sizeUnsigInts, &set.maxSizeUnsigInts);

            set.unsigInts[set.sizeUnsigInts] = (unsigned long long) x;
            ++set.sizeUnsigInts;

            return set;
        }
    }
    else {
        // Jeśli liczba zapisana zmiennoprzecinkowo jest całkowita ujemna
        if (x - (long long) x == 0) {
            set.sigInts = expand(set.sigInts, sizeof(long long),
                                 set.sizeSigInts, &set.maxSizeSigInts);

            set.sigInts[set.sizeSigInts] = (long long) x;
            ++set.sizeSigInts;

            return set;
        }
    }

    set.anyFloats = expand(set.anyFloats, sizeof(long double),
                           set.sizeAnyFloats, &set.maxSizeAnyFloats);

    set.anyFloats[set.sizeAnyFloats] = x;
    ++set.sizeAnyFloats;

    return set;
}

/**
 * Funkcja, która dostając liczbę całkowitą dodatnią, zwraca multizbiór z nią.
 * set - multizbiór, w którym chcę umieścić słowo
 * word - ciąg znaków składający się w słowo
 * size - ilość znaków w ciągu word
 */
static multiset processUnsigInt(multiset set, char *word, int base) {
    char *ptr;
    // Konwertuję słowo na unsigned long long
    unsigned long long x = strtoull(word, &ptr, base);

    set.unsigInts = expand(set.unsigInts, sizeof(unsigned long long),
                           set.sizeUnsigInts, &set.maxSizeUnsigInts);

    set.unsigInts[set.sizeUnsigInts] = x;
    ++set.sizeUnsigInts;

    return set;
}

/**
 * Liczba, która dostając liczbę całkowitą ujemną, zwraca multizbiór z nią.
 * set - multizbiór, w którym chcę umieścić słowo
 * word - ciąg znaków składający się w słowo
 * size - ilość znaków w ciągu word
 */
static multiset processSigInt(multiset set, char *word) {
    char *ptr;
    // Konwertuję słowo na long long
    long long x = strtoll(word, &ptr, 10);

    if (x == 0) {
        // Jeśli słowo jest zerem, traktuję jako nieujemną
        set.unsigInts = expand(set.unsigInts, sizeof(unsigned long long),
                               set.sizeUnsigInts, &set.maxSizeUnsigInts);

        set.unsigInts[set.sizeUnsigInts] = (unsigned long long) x;
        ++set.sizeUnsigInts;
    }
    else {
        set.sigInts = expand(set.sigInts, sizeof(long long),
                             set.sizeSigInts, &set.maxSizeSigInts);

        set.sigInts[set.sizeSigInts] = x;
        ++set.sizeSigInts;
    }

    return set;
}

/**
 * Funkcja, która dany ciąg znaków przetwarza w słowo, zamieszcza w odpowiednie
 * miejsce w multizbiorze i zwraca multizbiór z nią.
 * set - multizbiór, w którym chcę umieścić słowo
 * word - ciąg znaków składający się w słowo
 * size - ilość znaków w ciągu word
 */
multiset processWord(multiset set, char *word, size_t wordSize) {
    if (recognizeOctal(word, wordSize)) {
        return processUnsigInt(set, word, BASE_OCTAL);
    }
    else if (recognizeUnsigInt(word, wordSize)) {
        return processUnsigInt(set, word, BASE_DECIMAL);
    }
    else if (recognizeSigInt(word, wordSize)) {
        return processSigInt(set, word);
    }
    else if (recognizeHex(word, wordSize)) {
        return processUnsigInt(set, word, BASE_HEXADECIMAL);
    }
    else if (recognizeAnyFloat(word, wordSize)) {
        return processAnyFloat(set, word, wordSize);
    }
    else {
        return processNotNumber(set, word, wordSize);
    }
}
