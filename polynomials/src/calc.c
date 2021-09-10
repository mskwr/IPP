/** @file
 *  Implementacja kalkulatora działającego na wielomianach.
 *
 *  @author Michał Skwarek
 *  @date 2021
 */

/** Flaga potrzebna do poprawnego działania funkcji getline */
#define _GNU_SOURCE

#include "poly.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/**
 * Makro sprawdzające dany wskaźnik, czy nie ma błędu braku pamięci.
 * Jeśli jej brakuje, to awaryjnie kończy program kodem 1.
 * @param[in] p : wskaźnik wskazujący na pamięć do sprawdzenia
 */
#define CHECK_PTR(p)  \
  do {                \
    if (p == NULL) {  \
      exit(1);        \
    }                 \
  } while (0)

/** Minimalny rozmiar stosu do sprawdzenia czy wymaga zmniejszenia. */
#define REDUCE_MIN 16

/** Początkowy rozmiar dynamicznych struktur danych z wielomianami. */
#define DEFAULT_SIZE_POLY 16

/** Początkowy rozmiar dynamicznych struktur danych z typem char. */
#define DEFAULT_SIZE_CHAR 128

/** Stała zwracana przez getline po wczytaniu ostatniego wiersza. */
#define END (-1)

/** Liczba wielomianów potrzebna do działania 1-argumentowego. */
#define ONE_ARG_OP 1

/** Liczba wielomianów potrzebna do działania 2-argumentowego. */
#define TWO_ARG_OP 2

/** Stała zwracana przez strcmp, jeśli dwa stringi są identyczne. */
#define IDENTICAL 0

/**
 * To jest struktura przechowująca stos.
 * Stos jest oparty o implementację tablicową.
 */
typedef struct Stack {
    Poly *arr; ///< tablica z elementami stosu
    size_t size; ///< rozmiar stosu
    size_t maxSize; ///< zaalokowany rozmiar stosu
} Stack;

/**
 * Realokuje pamięć wskazywaną przez wskaźnik tylko wtedy, gdy potrzeba.
 * @param[in] x : wskaźnik
 * @param[in] type : rozmiar przechowywanych danych
 * @param[in] size : ilość przechowywanych danych
 * @param[in] max : przydzielona pamięć
 * @return wskaźnik po realokacji
 */
static void *SmartRealloc(void *x, size_t type, size_t size, size_t *max) {
    if (size >= *max) {
        *max = *max * 2;
        x = realloc(x, *max * type);
    }

    CHECK_PTR(x);
    return x;
}

/**
 * Tworzy stos i przydziela mu początkową pamięć.
 * @return stos
 */
static Stack *CreateS() {
    Stack *stack = malloc(sizeof(Stack));
    CHECK_PTR(stack);
    stack->size = 0;
    stack->maxSize = DEFAULT_SIZE_POLY;
    // tablica na wielomiany przechowywane przez stos
    stack->arr = malloc(sizeof(Poly) * DEFAULT_SIZE_POLY);
    CHECK_PTR(stack->arr);
    return stack;
}

/**
 * Usuwa z pamięci stos.
 * @param[in] s : stos
 */
static void DestroyS(Stack *s) {
    assert(s != NULL);

    while (s->size > 0) {
        PolyDestroy(&s->arr[s->size - 1]);
        --s->size;
    }

    free(s->arr);
    free(s);
}

/**
 * Wkłada na stos wielomian.
 * @param[in] s : stos
 * @param[in] x : wielomian
 */
static void PushS(Stack *s, Poly x) {
    assert(s != NULL);

    s->arr = SmartRealloc(s->arr, sizeof(Poly), s->size, &s->maxSize);
    s->arr[s->size] = x;
    ++s->size;
}

/**
 * Zdejmuje ze stosu wielomian.
 * @param[in] s : stos
 * @return wielomian
 */
static Poly PopS(Stack *s) {
    assert(s != NULL);

    --s->size;
    return s->arr[s->size];
}

/**
 * Drukuje wielomian.
 * @param[in] p : wielomian
 */
static void PrintPoly(const Poly *p);

/**
 * Drukuje jednomian.
 * @param[in] m : jednomian
 */
static void PrintMono(const Mono *m) {
    assert(m != NULL);

    // każdy jednomian powinien być otoczony nawiasami
    printf("(");
    PrintPoly(&m->p);
    printf(",%d)", m->exp);
}

static void PrintPoly(const Poly *p) {
    assert(p != NULL);

    if (PolyIsCoeff(p)) {
        printf("%ld", p->coeff);
    }
    else {
        for (size_t i = 0; i < PolyLength(p); ++i) {
            PrintMono(&p->arr[i]);

            // jednomiany wielomianu są oddzielane plusami
            if (i < PolyLength(p) - 1)
                printf("+");
        }
    }
}

/**
 * Sprawdza czy tablica znaków jest poprawnym numerem.
 * @param[in] line : tablica znaków
 * @param[in] size : rozmiar tablicy
 * @return prawda jeśli tablica jest numerem, fałsz wpp
 */
static bool IsNumber(char *line, size_t size) {
    size_t i = 0;

    // pusta tablica nie jest numerem
    if (size == 0 || line == NULL)
        return false;

    // numer może być poprzedzony minusem, ale nie plusem
    if (size > 0 && line[0] == '-')
        ++i;

    while (i < size) {
        if (line[i] < '0' || line[i] > '9')
            return false;

        ++i;
    }

    return true;
}

/**
 * Sprawdza czy znak jest znakiem białym.
 * @param[in] x : znak
 * @return prawda jeśli znak jest biały, fałsz wpp
 */
static bool IsWhitespace(char x) {
    if (x == ' ' || x == '\t' || x == '\v'
        || x == '\f' || x == '\r' || x == '\n')
        return true;
    else
        return false;
}

/**
 * Wstawia wielomian zerowy na szczyt stosu.
 * @param[in] s : stos
 */
static void ProcessZero(Stack *s) {
    assert(s != NULL);

    Poly zero = PolyFromCoeff(0);
    PushS(s, zero);
}

/**
 * Sprawdza czy wielomian na szczycie stosu jest współczynnikiem.
 * @param[in] s : stos
 * @param[in] count : numer linii
 */
static void ProcessIsCoeff(Stack *s, size_t count) {
    assert(s != NULL);

    if (s->size < ONE_ARG_OP)
        fprintf(stderr,"ERROR %zu STACK UNDERFLOW\n", count);
    else if (PolyIsCoeff(&s->arr[s->size - 1]))
        printf("1\n");
    else
        printf("0\n");
}

/**
 * Sprawdza czy wielomian na szczycie stosu jest zerowy.
 * @param[in] s : stos
 * @param[in] count : numer linii
 */
static void ProcessIsZero(Stack *s, size_t count) {
    assert(s != NULL);

    if (s->size < ONE_ARG_OP)
        fprintf(stderr,"ERROR %zu STACK UNDERFLOW\n", count);
    else if (PolyIsZero(&s->arr[s->size - 1]))
        printf("1\n");
    else
        printf("0\n");
}

/**
 * Tworzy kopię wielomianu ze szczytu stosu i wrzuca ją na stos.
 * @param[in] s : stos
 * @param[in] count : numer linii
 */
static void ProcessClone(Stack *s, size_t count) {
    assert(s != NULL);

    if (s->size < ONE_ARG_OP) {
        fprintf(stderr,"ERROR %zu STACK UNDERFLOW\n", count);
    }
    else {
        Poly copy = PolyClone(&s->arr[s->size - 1]);
        PushS(s, copy);
    }
}

/**
 * Dodaje dwa wielomiany ze szczytu stosu.
 * @param[in] s : stos
 * @param[in] count : numer linii
 */
static void ProcessAdd(Stack *s, size_t count) {
    assert(s != NULL);

    if (s->size < TWO_ARG_OP) {
        fprintf(stderr,"ERROR %zu STACK UNDERFLOW\n", count);
    }
    else {
        Poly p1 = PopS(s);
        Poly p2 = PopS(s);
        Poly sum = PolyAdd(&p1, &p2);
        PolyDestroy(&p1);
        PolyDestroy(&p2);
        PushS(s, sum);
    }
}

/**
 * Mnoży dwa wielomiany ze szczytu stosu.
 * @param[in] s : stos
 * @param[in] count : numer linii
 */
static void ProcessMul(Stack *s, size_t count) {
    assert(s != NULL);

    if (s->size < TWO_ARG_OP) {
        fprintf(stderr,"ERROR %zu STACK UNDERFLOW\n", count);
    }
    else {
        Poly p1 = PopS(s);
        Poly p2 = PopS(s);
        Poly mul = PolyMul(&p1, &p2);
        PolyDestroy(&p1);
        PolyDestroy(&p2);
        PushS(s, mul);
    }
}

/**
 * Neguje wielomian ze szczytu stosu.
 * @param[in] s : stos
 * @param[in] count : numer linii
 */
static void ProcessNeg(Stack *s, size_t count) {
    assert(s != NULL);

    if (s->size < ONE_ARG_OP) {
        fprintf(stderr,"ERROR %zu STACK UNDERFLOW\n", count);
    }
    else {
        Poly p = PopS(s);
        Poly neg = PolyNeg(&p);
        PolyDestroy(&p);
        PushS(s, neg);
    }
}

/**
 * Odejmuje dwa wielomiany ze szczytu stosu.
 * @param[in] s : stos
 * @param[in] count : numer linii
 */
static void ProcessSub(Stack *s, size_t count) {
    assert(s != NULL);

    if (s->size < TWO_ARG_OP) {
        fprintf(stderr,"ERROR %zu STACK UNDERFLOW\n", count);
    }
    else {
        Poly p1 = PopS(s);
        Poly p2 = PopS(s);
        Poly sub = PolySub(&p1, &p2);
        PolyDestroy(&p1);
        PolyDestroy(&p2);
        PushS(s, sub);
    }
}

/**
 * Porównuje dwa wielomiany ze szczytu stosu.
 * @param[in] s : stos
 * @param[in] count : numer linii
 */
static void ProcessIsEq(Stack *s, size_t count) {
    assert(s != NULL);

    if (s->size < TWO_ARG_OP)
        fprintf(stderr,"ERROR %zu STACK UNDERFLOW\n", count);
    else if (PolyIsEq(&s->arr[s->size - 1], &s->arr[s->size - 2]))
        printf("1\n");
    else
        printf("0\n");
}

/**
 * Liczy stopień wielomianu ze szczytu stosu.
 * @param[in] s : stos
 * @param[in] count : numer linii
 */
static void ProcessDeg(Stack *s, size_t count) {
    assert(s != NULL);

    if (s->size < ONE_ARG_OP)
        fprintf(stderr,"ERROR %zu STACK UNDERFLOW\n", count);
    else
        printf("%d\n", PolyDeg(&s->arr[s->size - 1]));
}

/**
 * Liczy stopień wielomianu ze szczytu stosu dla danej zmiennej.
 * @param[in] s : stos
 * @param[in] line : tablica z numerem zmiennej
 * @param[in] size : rozmiar tablicy
 * @param[in] count : numer linii
 */
static void ProcessDegBy(Stack *s, char *line, size_t size, size_t count) {
    assert(s != NULL);

    // jeśli znak po znakach DEG_BY nie jest biały, to jest to zła komenda
    // jeśli jest biały, ale nie jest to spacja, to jest to zła zmienna
    if (size > 6 && !IsWhitespace(line[6])) {
        fprintf(stderr, "ERROR %zu WRONG COMMAND\n", count);
    }
    else if (size <= 7 || (line[6] != ' ' && IsWhitespace(line[6]))
             || (size > 7 && line[7] == '-')) {
        fprintf(stderr,"ERROR %zu DEG BY WRONG VARIABLE\n", count);
    }
    else {
        // tablica na numer zmiennej
        char *num = malloc(sizeof(char) * (size - 6));
        CHECK_PTR(num);

        // numer rozpoczyna się po siedmiu znakach "DEG_BY "
        for (size_t i = 0; i + 7 < size; ++i)
            num[i] = line[i + 7];

        // dodanie znaku NULL, ponieważ strtold przyjmuje jako argument string
        num[size - 7] = 0;
        bool IsNum = IsNumber(num, size - 7);
        long double x = strtold(num, NULL);
        free(num);

        if (!IsNum || x > ULONG_MAX
            // dla dobrego działania programu pod okiem valgrinda
            || (x == ULLONG_MAX && line[size - 1] > '5')) {
            fprintf(stderr,"ERROR %zu DEG BY WRONG VARIABLE\n", count);
        }
        else {
            if (s->size < ONE_ARG_OP) {
                fprintf(stderr, "ERROR %zu STACK UNDERFLOW\n", count);
            }
            else {
                size_t idx = (size_t)x;
                printf("%d\n", PolyDegBy(&s->arr[s->size - 1], idx));
            }
        }
    }
}

/**
 * Wylicza wartość wielomianu w danym punkcie.
 * @param[in] s : stos
 * @param[in] line : tablica z wartością zmiennej
 * @param[in] size : rozmiar tablicy
 * @param[in] count : numer linii
 */
static void ProcessAt(Stack *s, char *line, size_t size, size_t count) {
    assert(s != NULL);

    // jeśli znak po znakach AT nie jest biały, to jest to zła komenda
    // jeśli jest biały, ale nie jest to spacja, to jest to zła zmienna
    if (size > 2 && !IsWhitespace(line[2])) {
        fprintf(stderr, "ERROR %zu WRONG COMMAND\n", count);
    }
    else if (size <= 3 || (line[2] != ' ' && IsWhitespace(line[2]))
             || (size == 4 && line[3] == '-')) {
        fprintf(stderr, "ERROR %zu AT WRONG VALUE\n", count);
    }
    else {
        // tablica na wartość zmiennej
        char *num = malloc(sizeof(char) * (size - 2));
        CHECK_PTR(num);

        // numer rozpoczyna się po trzech znakach "AT "
        for (size_t i = 0; i + 3 < size; ++i)
            num[i] = line[i + 3];

        // dodanie znaku NULL, ponieważ strtold przyjmuje jako argument string
        num[size - 3] = 0;
        bool IsNum = IsNumber(num, size - 3);
        long double x = strtold(num, NULL);
        free(num);

        if (!IsNum || x < LLONG_MIN || x > LLONG_MAX) {
            fprintf(stderr, "ERROR %zu AT WRONG VALUE\n", count);
        }
        else {
            if (s->size < ONE_ARG_OP) {
                fprintf(stderr, "ERROR %zu STACK UNDERFLOW\n", count);
            }
            else {
                poly_coeff_t c = (poly_coeff_t)x;
                Poly p = PopS(s);
                Poly at = PolyAt(&p, c);
                PolyDestroy(&p);
                PushS(s, at);
            }
        }
    }
}

/**
 * Drukuje wielomian ze szczytu stosu.
 * @param[in] s : stos
 * @param[in] count : numer linii
 */
static void ProcessPrint(Stack *s, size_t count) {
    assert(s != NULL);

    if (s->size < ONE_ARG_OP) {
        fprintf(stderr, "ERROR %zu STACK UNDERFLOW\n", count);
    }
    else {
        PrintPoly(&s->arr[s->size - 1]);
        printf("\n");
    }
}

/**
 * Usuwa wielomian ze szczytu stosu.
 * @param[in] s : stos
 * @param[in] count : numer linii
 */
static void ProcessPop(Stack *s, size_t count) {
    assert(s != NULL);

    if (s->size < ONE_ARG_OP) {
        fprintf(stderr, "ERROR %zu STACK UNDERFLOW\n", count);
    }
    else {
        Poly p = PopS(s);
        PolyDestroy(&p);
    }
}

/**
 * Składa wielomian ze szczytu stosu z innymi wielomianami na stosie.
 * @param[in] s : stos
 * @param[in] line : tablica z liczbą wielomianów do składania
 * @param[in] size : rozmiar tablicy
 * @param[in] count : numer linii
 */
static void ProcessCompose(Stack *s, char *line, size_t size, size_t count) {
    assert(s != NULL);

    // jeśli znak po znakach COMPOSE nie jest biały, to jest to zła komenda
    // jeśli jest biały, ale nie jest to spacja, to jest to zła zmienna
    if (size > 7 && !IsWhitespace(line[7])) {
        fprintf(stderr, "ERROR %zu WRONG COMMAND\n", count);
    }
    else if (size <= 8 || (line[7] != ' ' && IsWhitespace(line[7]))
             || (size > 8 && line[8] == '-')) {
        fprintf(stderr, "ERROR %zu COMPOSE WRONG PARAMETER\n", count);
    }
    else {
        // tablica na liczbę wielomianów
        char *num = malloc(sizeof(char) * (size - 7));
        CHECK_PTR(num);

        // numer rozpoczyna się po siedmiu znakach "COMPOSE "
        for (size_t i = 0; i + 8 < size; ++i)
            num[i] = line[i + 8];

        // dodanie znaku NULL, ponieważ strtold przyjmuje jako argument string
        num[size - 8] = 0;
        bool IsNum = IsNumber(num, size - 8);
        long double x = strtold(num, NULL);
        free(num);

        if (!IsNum || x > ULONG_MAX
            // dla dobrego działania programu pod okiem valgrinda
            || (x == ULONG_MAX && line[size - 1] > '5')) {
            fprintf(stderr,"ERROR %zu COMPOSE WRONG PARAMETER\n", count);
        }
        else {
            size_t k = (size_t)x;

            if (s->size <= k) {
                fprintf(stderr, "ERROR %zu STACK UNDERFLOW\n", count);
            }
            else {
                // tablica dla wielomianów do złożenia
                Poly *poly = malloc(sizeof(Poly) * k);
                CHECK_PTR(poly);
                Poly p = PopS(s);

                for (size_t i = k; i > 0; --i)
                    poly[i - 1] = PopS(s);

                Poly sol = PolyCompose(&p, k, poly);

                for (size_t i = 0; i < k; ++i)
                    PolyDestroy(&poly[i]);

                PolyDestroy(&p);
                free(poly);
                PushS(s, sol);
            }
        }
    }
}

/**
 * Rozpoznaje, przetwarza i wykonuje komendę.
 * @param[in] s : stos
 * @param[in] line : tablica znaków z komendą
 * @param[in] size : rozmiar tablicy
 * @param[in] count : numer linii
 */
static void ProcessCommand(Stack *s, char *line, size_t size, size_t count) {
    assert(line != NULL);

    if (strcmp(line, "ZERO\n") == IDENTICAL)
        ProcessZero(s);
    else if (strcmp(line, "IS_COEFF\n") == IDENTICAL)
        ProcessIsCoeff(s, count);
    else if (strcmp(line, "IS_ZERO\n") == IDENTICAL)
        ProcessIsZero(s, count);
    else if (strcmp(line, "CLONE\n") == IDENTICAL)
        ProcessClone(s, count);
    else if (strcmp(line, "ADD\n") == IDENTICAL)
        ProcessAdd(s, count);
    else if (strcmp(line, "MUL\n") == IDENTICAL)
        ProcessMul(s, count);
    else if (strcmp(line, "NEG\n") == IDENTICAL)
        ProcessNeg(s, count);
    else if (strcmp(line, "SUB\n") == IDENTICAL)
        ProcessSub(s, count);
    else if (strcmp(line, "IS_EQ\n") == IDENTICAL)
        ProcessIsEq(s, count);
    else if (strcmp(line, "DEG\n") == IDENTICAL)
        ProcessDeg(s, count);
    else if (size > 5 && line[0] == 'D' && line[1] == 'E' && line[2] == 'G'
             && line[3] == '_' && line[4] == 'B' && line[5] == 'Y')
        ProcessDegBy(s, line, size, count);
    else if (size > 1 && line[0] == 'A' && line[1] == 'T')
        ProcessAt(s, line, size, count);
    else if (strcmp(line, "PRINT\n") == IDENTICAL)
        ProcessPrint(s, count);
    else if (strcmp(line, "POP\n") == IDENTICAL)
        ProcessPop(s, count);
    else if (size > 6 && line[0] == 'C' && line[1] == 'O' && line[2] == 'M'
             && line[3] == 'P' && line[4] == 'O' && line[5] == 'S'
             && line[6] == 'E')
        ProcessCompose(s, line, size, count);
    else
        fprintf(stderr, "ERROR %zu WRONG COMMAND\n", count);
}

/**
 * Sprawdza czy tablica znaków jest poprawnym współczynnikiem.
 * @param[in] line : tablica znaków ze współczynnikiem
 * @param[in] size : rozmiar tablicy
 * @return prawda jeśli jest poprawnym współczynnikiem, fałsz wpp
 */
static bool CorrectCoeff(char *line, size_t size) {
    // współczynnik może zawierać minus, ale nie plus
    if (size == 1 && line[0] == '-')
        return false;

    if (!IsNumber(line, size))
        return false;

    // tablica na współczynnik
    char *num = malloc(sizeof(char) * (size + 1));
    CHECK_PTR(num);

    for (size_t i = 0; i < size; ++i)
        num[i] = line[i];

    // dodanie znaku NULL, ponieważ strtold przyjmuje jako argument string
    num[size] = '\0';
    long double x = strtold(num, NULL);
    free(num);

    if ((x < LLONG_MIN || x > LLONG_MAX)
        // dla dobrego działania programu pod okiem valgrinda
        || (x == LLONG_MIN && line[size - 1] == '9')
        || (x == LLONG_MAX && line[size - 1] > '7'))
        return false;
    else
        return true;
}

/**
 * Sprawdza czy tablica znaków jest poprawnym wykładnikiem.
 * @param[in] line : tablica znaków z wykładnikiem
 * @param[in] size : rozmiar tablicy
 * @return prawda jeśli jest poprawnym wykładnikiem, fałsz wpp
 */
static bool CorrectExp(char *line, size_t size) {
    if (!IsNumber(line, size))
        return false;

    // tablica znaków zwracana do tej funkcji zawsze kończy się '\0'
    long double x = strtold(line, NULL);

    if (x < 0 || x > INT_MAX)
        return false;
    else
        return true;
}

/**
 * Sprawdza czy wielomian jest poprawny.
 * @param[in] line : tablica znaków z wielomianem
 * @param[in] size : rozmiar tablicy
 * @return prawda jeśli wielomian jest poprawny, fałsz wpp
 */
static bool CorrectPoly(char *line, size_t size);

/**
 * Sprawdza czy jednomian jest poprawny.
 * @param[in] line : tablica znaków z jednomianem
 * @param[in] size : rozmiar tablicy
 * @return prawda jeśli jednomian jest poprawny, fałsz wpp
 */
static bool CorrectMono(char *line, size_t size) {
    // tablica na wielomian tego jednomianu
    char *p = malloc(sizeof(char) * DEFAULT_SIZE_CHAR);
    CHECK_PTR(p);
    size_t curSize = 0, maxSize = DEFAULT_SIZE_CHAR;
    size_t i = 1;
    size_t comas = 0;

    // przewijanie do przecinka oddzielającego wielomian i wykładnik
    for (; i < size && (comas != 0 || line[i] != ','); ++i) {
        if (line[i] == '(')
            ++comas;
        else if (line[i] == ',')
            --comas;

        p = SmartRealloc(p, sizeof(char), curSize, &maxSize);
        // pominięcie nawiasów z dwóch końców
        p[i - 1] = line[i];
        ++curSize;
    }

    // brak przecinka to zły jednomian
    if (i >= size - 1) {
        free(p);
        return false;
    }

    bool isPoly = CorrectPoly(p, curSize);
    free(p);

    if (!isPoly)
        return false;
    // od razu po wielomianie musi wystąpić przecinek
    if (line[i] != ',')
        return false;

    ++i;
    // tablica na wykładnik
    char *exp = malloc(sizeof(char) * DEFAULT_SIZE_CHAR);
    CHECK_PTR(exp);
    curSize = 0, maxSize = DEFAULT_SIZE_CHAR;

    for (size_t j = 0; i < size && line[i] != ')'; ++j, ++i) {
        exp = SmartRealloc(exp, sizeof(char), curSize, &maxSize);
        exp[j] = line[i];
        ++curSize;
    }

    exp = SmartRealloc(exp, sizeof(char), curSize, &maxSize);
    // dodanie znaku NULL, ponieważ strtold przyjmuje jako argument string
    exp[curSize] = 0;

    // brak nawiasu kończącego jednomian oznacza zły jednomian
    if (i == size) {
        free(exp);
        return false;
    }

    bool isExp = CorrectExp(exp, curSize);
    free(exp);

    if (!isExp)
        return false;

    return true;
}

static bool CorrectPoly(char *line, size_t size) {
    if (size == 0)
        return false;

    // jeśli pierwszy znak nie jest nawiasem to musi to być współczynnik
    if (line[0] != '(') {
        return CorrectCoeff(line, size);
    }
    else {
        size_t i = 0, brackets = 1, pSize = 0;
        // tablica na wielomiany jednomianów wielomianu
        char *p = malloc(sizeof(char) * DEFAULT_SIZE_CHAR);
        CHECK_PTR(p);

        // przechodzę wszystkie jednomiany w wielomianie
        while (i < size) {
            brackets = 1;
            size_t curSize = 0, maxSize = DEFAULT_SIZE_CHAR;
            // zmienna mówiąca czy to pierwszy jednomian w wielomianie
            bool start = true;

            // przechodzę jednomian przewijając do zamykającego nawiasu
            for (; i < size && brackets != 0; ++i) {
                if (!start && line[i] == '(')
                    ++brackets;
                else if (line[i] == ')')
                    --brackets;

                p = SmartRealloc(p, sizeof(char), pSize, &maxSize);
                p[curSize] = line[i];
                ++curSize;
                ++pSize;
                start = false;
            }

            // jeśli liczba nawiasów się nie zgadza, to jest to zły wielomian
            if (brackets != 0) {
                free(p);
                return false;
            }
            if (!CorrectMono(p, curSize)) {
                free(p);
                return false;
            }
            // wielomian ma tylko jeden poprawny jednomian
            if (i == size) {
                free(p);
                return true;
            }
            // bezpośrednio po jednomianie musi wystąpić znak plus
            if (line[i] != '+' || (line[i] == '+' && i == size - 1)) {
                free(p);
                return false;
            }

            ++i;
        }

        free(p);
        return true;
    }
}

/**
 * Pobiera wielomian z tablicy znaków z poprawnym wielomianem.
 * @param[in] line : tablica znaków z poprawnym wielomianem
 * @param[in] size : rozmiar tablicy
 * @return wielomian
 */
static Poly LoadPoly(char *line, size_t size);

/**
 * Pobiera jednomian z tablicy znaków z poprawnym jednomianem.
 * @param[in] line : tablica znaków z poprawnym jednomianem
 * @param[in] size : rozmiar tablicy
 * @return jednomian
 */
static Mono LoadMono(char *line, size_t size) {
    // tablica na wielomian jednomianu
    char *p = malloc(sizeof(char) * DEFAULT_SIZE_CHAR);
    CHECK_PTR(p);
    size_t curSize = 0, maxSize = DEFAULT_SIZE_CHAR;
    size_t i = 1;
    size_t comas = 0;

    // przewijanie do przecinka oddzielającego wielomian i wykładnik
    for (; i < size && (comas != 0 || line[i] != ','); ++i) {
        if (line[i] == '(')
            ++comas;
        else if (line[i] == ',')
            --comas;

        p = SmartRealloc(p, sizeof(char), curSize, &maxSize);
        // pominięcie nawiasów z dwóch końców
        p[i - 1] = line[i];
        ++curSize;
    }

    Poly poly = LoadPoly(p, curSize);
    free(p);
    ++i;

    // tablica na wykładnik
    char *exp = malloc(sizeof(char) * DEFAULT_SIZE_CHAR);
    CHECK_PTR(exp);
    curSize = 0, maxSize = DEFAULT_SIZE_CHAR;

    for (size_t j = 0; i < size && line[i] != ')'; ++j, ++i) {
        exp = SmartRealloc(exp, sizeof(char), curSize, &maxSize);
        exp[j] = line[i];
        ++curSize;
    }

    exp = SmartRealloc(exp, sizeof(char), curSize, &maxSize);
    // dodanie znaku NULL, ponieważ strtold przyjmuje jako argument string
    exp[curSize] = 0;
    long double x = strtold(exp, NULL);
    free(exp);
    poly_exp_t exponent = (poly_exp_t)x;

    return (Mono) {.p = poly, .exp = exponent};
}

static Poly LoadPoly(char *line, size_t size) {
    // jeśli pierwszy znak nie jest nawiasem to musi to być współczynnik
    if (size > 0 && line[0] != '(') {
        char *num = malloc(sizeof(char) * (size + 1));
        CHECK_PTR(num);

        for (size_t i = 0; i < size; ++i)
            num[i] = line[i];

        // dodanie znaku NULL, ponieważ strtold przyjmuje jako argument string
        num[size] = 0;
        long double x = strtold(num, NULL);
        free(num);
        poly_coeff_t c = (poly_coeff_t)x;

        return PolyFromCoeff(c);
    }
    else {
        // tablica na jednomiany wielomianu
        Mono *monos = malloc(sizeof(Mono) * DEFAULT_SIZE_POLY);
        CHECK_PTR(monos);
        size_t monoSize = 0, monoMaxSize = DEFAULT_SIZE_POLY;
        size_t i = 0, brackets = 1, pSize = 0;
        // tablica na wielomiany jednomianów wielomianu
        char *p = malloc(sizeof(char) * DEFAULT_SIZE_CHAR);
        CHECK_PTR(p);

        // przechodzę wszystkie jednomiany w wielomianie
        while (i < size) {
            brackets = 1;
            size_t curSize = 0, maxSize = DEFAULT_SIZE_CHAR;
            // zmienna mówiąca czy to pierwszy jednomian w wielomianie
            bool start = true;

            // przechodzę jednomian przewijając do zamykającego nawiasu
            for (; i < size && brackets != 0; ++i) {
                if (!start && line[i] == '(')
                    ++brackets;
                else if (line[i] == ')')
                    --brackets;

                p = SmartRealloc(p, sizeof(char), pSize, &maxSize);
                p[curSize] = line[i];
                ++curSize;
                ++pSize;
                start = false;
            }

            monos = SmartRealloc(monos, sizeof(Mono), monoSize, &monoMaxSize);
            monos[monoSize] = LoadMono(p, curSize);
            ++monoSize;
            ++i;
        }

        free(p);
        // tworzę wielomian mając już kompletną tablicę jednomianów wielomianu
        Poly sol = PolyAddMonos(monoSize, monos);
        free(monos);
        return sol;
    }
}

/**
 * Sprawdza czy tablica znaków jest komendą.
 * @param[in] line : tablica znaków
 * @return prawda jeśli tablica znaków jest komendą, fałsz wpp
 */
static bool IsCommand(char *line) {
    // tablica znaków jest komendą jeśli zaczyna się od litery
    bool smallLetter = line[0] >= 'a' && line[0] <= 'z';
    bool bigLetter = line[0] >= 'A' && line[0] <= 'Z';
    return smallLetter || bigLetter;
}

/**
 * Sprawdza czy tablica znaków przedstawia linię, którą należy zignorować.
 * @param[in] line : tablica znaków
 * @param[in] size : rozmiar tablicy
 * @return prawda jeśli tablica znaków powinna być ignorowana, fałsz wpp
 */
static bool IgnoreLine(char *line, size_t size) {
    // linia której pierwszym znakiem jest '#' jest komentarzem
    if (size > 0 && (line[0] == '#'))
        return true;

    return size == 0;
}

/**
 * Wczytuje wejście i przetwarza na polecenia kalkulatora.
 * @param[in] stack : stos
 */
static void LoadInput(Stack *stack) {
    assert(stack != NULL);

    char *line = NULL;
    size_t buffSize, count = 1;
    ssize_t read;

    while ((read = getline(&line, &buffSize, stdin)) != END) {
        CHECK_PTR(line);

        // jeśli linia nie ma na końcu znaku nowej linii, to jest doklejany
        if (line[read - 1] != '\n') {
            ++read;
            strcat(line, "\n");
            CHECK_PTR(line);
        }

        if (!IgnoreLine(line, read - 1)) {
            if (IsCommand(line))
                ProcessCommand(stack, line, read - 1, count);
            else if (!CorrectPoly(line, read - 1))
                fprintf(stderr, "ERROR %zu WRONG POLY\n", count);
            else
                PushS(stack, LoadPoly(line, read - 1));
        }

        // jeśli połowa stosu jest pusta, to jest o połowę zmniejszany
        if (stack->size > REDUCE_MIN && stack->maxSize > 2 * stack->size) {
            stack->maxSize = stack->maxSize / 2;
            stack->arr =
                    realloc(stack->arr, stack->maxSize * sizeof(Poly));
            CHECK_PTR(stack->arr);
        }

        ++count;
    }

    free(line);
}

/**
 * Głowna funkcja - tworzy stos, wczytuje wejście i usuwa stos.
 */
int main() {
    Stack *stack = CreateS();
    LoadInput(stack);
    DestroyS(stack);
}