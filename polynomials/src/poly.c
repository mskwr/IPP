/** @file
 *  Implementacja operacji na wielomianach rzadkich wielu zmiennych.
 *
 *  @author Michał Skwarek
 *  @date 2021
 */

#include "poly.h"
#include <stdlib.h>
#include <string.h>

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


/** Początkowy rozmiar dynamicznych struktur danych. */
#define DEFAULT_SIZE 4

/**
 * Wskazuje większy z dwóch wykładników.
 * @param[in] a : wykładnik
 * @param[in] b : wykładnik
 * @return większy wykładnik
 */
static poly_exp_t max(poly_exp_t a, poly_exp_t b) {
    if (a > b)
        return a;
    else
        return b;
}

/**
 * Podnosi liczbę do danej potęgi.
 * @param[in] a : liczba
 * @param[in] exp : potęga
 * @return potęga liczby
 */
static poly_coeff_t power(poly_coeff_t a, poly_exp_t exp) {
    if (exp == 0)
        return 1;
    else if (exp == 1)
        return a;
    else if (exp % 2 == 0)
        return power(a * a, exp / 2);
    else
        return a * power(a * a, (exp - 1) / 2);
}

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
 * Porównuje dwa jednomiany pod względem wielkości ich wykładników.
 * @param[in] a : jednomian
 * @param[in] b : jednomian
 * @return jednomian z większym wykładnikiem
 */
static int CompareMonos(const void *a, const void *b) {
    const Mono *x = (Mono *)a;
    const Mono *y = (Mono *)b;

    if (x->exp < y->exp)
        return -1;
    else if (x->exp == y->exp)
        return 0;
    else
        return 1;
}

/**
 * Sortuje tablicę jednomianów względem wykładników.
 * @param[in] arr : tablica jednomianóœ
 * @param[in] size : rozmiar tablicy
 * @return posortowana tablica
 */
static Mono *SortMonos(Mono *arr, size_t size) {
    qsort(arr, size, sizeof(Mono), CompareMonos);

    return arr;
}

void PolyDestroy(Poly *p) {
    if (!PolyIsCoeff(p)) {
        for (size_t i = 0; i < PolyLength(p); ++i)
            MonoDestroy(&p->arr[i]);

        free(p->arr);
        p->arr = NULL;
    }
}

Poly PolyClone(const Poly *p) {
    assert(p != NULL);

    if (PolyIsCoeff(p)) {
        return (Poly) {.coeff = p->coeff, .arr = NULL};
    }
    else {
        Mono *monos = malloc(PolyLength(p) * sizeof(Mono));
        CHECK_PTR(monos);

        for (size_t i = 0; i < PolyLength(p); ++i)
            monos[i] = MonoClone(&p->arr[i]);

        return (Poly) {.size = PolyLength(p), .arr = monos};
    }
}

Poly PolyAdd(const Poly *p, const Poly *q) {
    assert(p != NULL && q != NULL);

    if (PolyIsCoeff(p) && PolyIsCoeff(q)) {
        return PolyFromCoeff(p->coeff + q->coeff);
    }
    else if (PolyIsCoeff(p)) {
        return PolyAdd(q, p);
    }
    else if (PolyIsCoeff(q)) {
        if (PolyIsZero(q))
            return PolyClone(p);

        // jednomiany sumy wielomianów
        Mono *sumMonos = malloc(sizeof(Mono) * (PolyLength(p) + 1));
        CHECK_PTR(sumMonos);

        for (size_t i = 0; i < PolyLength(p); ++i)
            sumMonos[i] = MonoClone(&p->arr[i]);
        // do jednomianów wielomianu p doklejam współczynnik q
        sumMonos[PolyLength(p)] = MonoFromCoeff(q->coeff);

        Poly sol = PolyAddMonos(PolyLength(p) + 1, sumMonos);
        free(sumMonos);
        return sol;
    }
    else {
        // jednomiany sumy wielomianów
        Mono *sumMonos = malloc(DEFAULT_SIZE * sizeof(Mono));
        CHECK_PTR(sumMonos);
        // rozmiar tworzonej dynamicznej tablicy i indeksy do przechodzenia
        // po jednomianach obu wielomianów
        size_t i = 0, maxi = DEFAULT_SIZE, p_i = 0, q_i = 0;

        // przechodzę po jednomianach obu wielomianów
        for (; p_i < PolyLength(p) && q_i < PolyLength(q); ++i) {
            // jeśli jednomiany mają równe wykładniki to doklejam ich sumę
            if (p->arr[p_i].exp == q->arr[q_i].exp) {
                sumMonos = SmartRealloc(sumMonos, sizeof(Mono), i, &maxi);
                sumMonos[i] = (Mono)
                        {.exp = p->arr[p_i].exp,
                         .p = PolyAdd(&p->arr[p_i].p, &q->arr[q_i].p)};
                ++p_i;
                ++q_i;
            }
            else if (p->arr[p_i].exp < q->arr[q_i].exp) {
                sumMonos = SmartRealloc(sumMonos, sizeof(Mono), i, &maxi);
                sumMonos[i] = MonoClone(&p->arr[p_i]);
                ++p_i;
            }
            else {
                sumMonos = SmartRealloc(sumMonos, sizeof(Mono), i, &maxi);
                sumMonos[i] = MonoClone(&q->arr[q_i]);
                ++q_i;
            }
        }

        // doklejam resztę jednomianów któregoś z wielomianów
        for (; p_i < PolyLength(p); ++p_i, ++i) {
            sumMonos = SmartRealloc(sumMonos, sizeof(Mono), i, &maxi);
            sumMonos[i] = MonoClone(&p->arr[p_i]);
        }
        for (; q_i < PolyLength(q); ++q_i, ++i) {
            sumMonos = SmartRealloc(sumMonos, sizeof(Mono), i, &maxi);
            sumMonos[i] = MonoClone(&q->arr[q_i]);
        }

        Poly sol = PolyAddMonos(i, sumMonos);
        free(sumMonos);
        return sol;
    }
}

Poly PolyOwnMonos(size_t count, Mono *monos) {
    if (count == 0 || monos == NULL) {
        if (monos != NULL)
            free(monos);

        return PolyFromCoeff(0);
    }

    monos = SortMonos(monos, count);
    // dynamiczna tablica z uporządkowanymi jednomianami powstałego wielomianu
    Mono *newMonos = malloc(DEFAULT_SIZE * sizeof(Mono));
    CHECK_PTR(newMonos);
    // obecnie pamiętany jednomian i rozmiar nowych jednomianów
    Mono cur = monos[0];
    Mono temp;
    // zmienna informująca, czy ostatnie 2 jednomiany miały ten sam wykładnik
    bool similarExps = false;
    size_t size = 0, maxSize = DEFAULT_SIZE;

    for (size_t i = 1; i < count; ++i) {
        // jeśli wykładniki są równe, to sumuję jednomiany
        if (monos[i].exp == cur.exp) {
            if (i != 1 && similarExps)
                temp = cur;

            cur = (Mono)
                    {.p = PolyAdd(&cur.p, &monos[i].p), .exp = monos[i].exp};
            // mając sumę, mogę już je usunąć z pamięci
            MonoDestroy(&monos[i - 1]);
            MonoDestroy(&monos[i]);

            if (i != 1 && similarExps)
                MonoDestroy(&temp);

            similarExps = true;
        }
            // jeśli jednomian jest zerowy, to go ignoruję i usuwam z pamięci
        else if (PolyIsZero(&cur.p)) {
            MonoDestroy(&cur);
            cur = monos[i];
            similarExps = false;
        }
            // jeśli wykładniki są różne i jednomian nie jest zerowy, zapisuję go
        else {
            newMonos = SmartRealloc(newMonos, sizeof(Mono), size, &maxSize);
            newMonos[size] = cur;
            ++size;
            cur = monos[i];
            similarExps = false;
        }
    }

    if (PolyIsZero(&cur.p)) {
        MonoDestroy(&cur);
    }
    else {
        newMonos = SmartRealloc(newMonos, sizeof(Mono), size, &maxSize);
        newMonos[size] = cur;
        ++size;
    }

    free(monos);

    // brak jednomianów oznacza wielomian zerowy
    if (size == 0) {
        free(newMonos);
        return PolyFromCoeff(0);
    }
    // jeśli jest tylko jeden jednomian i jest współczynnikiem, to wielomian
    // też jest współczynnikiem, a jednomian mogę usunąć z pamięci
    else if (size == 1 && newMonos[0].exp == 0
             && PolyIsCoeff(&newMonos[0].p)) {
        Poly sol = newMonos[0].p;
        free(newMonos);
        return sol;
    }
    else {
        return (Poly) {.size = size, .arr = newMonos};
    }
}

Poly PolyAddMonos(size_t count, const Mono monos[]) {
    if (count == 0 || monos == NULL)
        return PolyFromCoeff(0);

    Mono *m = malloc(count * sizeof(Mono));
    CHECK_PTR(m);
    // płytkie kopiowanie wszystkich wskaźników z głębi monos[]
    memcpy(m, monos, count * sizeof(Mono));
    CHECK_PTR(m);
    return PolyOwnMonos(count, m);
}

Poly PolyCloneMonos(size_t count, const Mono monos[]) {
    if (count == 0 || monos == NULL)
        return PolyFromCoeff(0);

    Mono *m = malloc(sizeof(Mono) * DEFAULT_SIZE);
    CHECK_PTR(m);
    size_t size = 0, maxSize = DEFAULT_SIZE;

    // klonuję tylko niezerowe jednomiany
    for (size_t i = 0; i < count; ++i) {
        if (!PolyIsZero(&monos[i].p)) {
            m = SmartRealloc(m, sizeof(Mono), size, &maxSize);
            m[size] = MonoClone(&monos[i]);
            ++size;
        }
    }

    return PolyOwnMonos(size, m);
}

Poly PolyMul(const Poly *p, const Poly *q) {
    assert(p != NULL && q != NULL);

    if (PolyIsZero(p) || PolyIsZero(q)) {
        return PolyFromCoeff(0);
    }
    else if (PolyIsCoeff(p) && PolyIsCoeff(q)) {
        return PolyFromCoeff(p->coeff * q->coeff);
    }
    else if (PolyIsCoeff(p)) {
        return PolyMul(q, p);
    }
    else if (PolyIsCoeff(q)) {
        // tablica z jednomianami iloczynu
        Mono *prodMonos = malloc(sizeof(Mono) * PolyLength(p));
        CHECK_PTR(prodMonos);

        // przepisuję jednomiany p, każdy przemnożony przez współczynnik q
        for (size_t i = 0; i < PolyLength(p); ++i)
            prodMonos[i] = (Mono)
                    {.exp = p->arr[i].exp, .p = PolyMul(&p->arr[i].p, q)};

        // jeśli powstał tylko jeden jednomian, który jest współczynnikiem
        // to powstały wielomian też jest współczynnikiem
        if (PolyLength(p) == 1 && PolyIsZero(&prodMonos[0].p)) {
            MonoDestroy(&prodMonos[0]);
            free(prodMonos);
            return PolyFromCoeff(0);
        }

        Poly sol = PolyAddMonos(PolyLength(p), prodMonos);
        free(prodMonos);
        return sol;
    }
    else {
        // tablica z jednomianami iloczynu
        Mono *prodMonos =
                malloc(sizeof(Mono) * (PolyLength(p) * PolyLength(q)));
        CHECK_PTR(prodMonos);
        size_t size = 0;

        for (size_t i = 0; i < PolyLength(p); ++i) {
            for (size_t j = 0; j < PolyLength(q); ++j, ++size)
                // każdy jednomian przemnażam z każdym innym
                prodMonos[size] = (Mono)
                        {.exp = p->arr[i].exp + q->arr[j].exp,
                         .p = PolyMul(&p->arr[i].p, &q->arr[j].p)};
        }

        Poly sol = PolyAddMonos(PolyLength(p) * PolyLength(q), prodMonos);
        free(prodMonos);
        return sol;
    }
}

Poly PolyNeg(const Poly *p) {
    assert(p != NULL);

    // negacja jako przemnożenie wielomianu przez -1
    Poly neg = PolyFromCoeff(-1);
    Poly sol = PolyMul(p, &neg);
    PolyDestroy(&neg);
    return sol;
}

Poly PolySub(const Poly *p, const Poly *q) {
    assert(p != NULL && q != NULL);

    // róznica jako odjęcie zanegowanego wielomianu
    Poly negQ = PolyNeg(q);
    Poly sol = PolyAdd(p, &negQ);
    PolyDestroy(&negQ);
    return sol;
}

poly_exp_t PolyDegBy(const Poly *p, size_t var_idx) {
    assert(p != NULL);

    if (PolyIsZero(p)) {
        return -1;
    }
    else if (PolyIsCoeff(p)) {
        return 0;
    }
    else {
        poly_exp_t deg = 0;

        if (var_idx == 0) {
            // odpowiednia zmienna, czyli też głębokość rekurencji
            for (size_t i = 0; i < PolyLength(p); ++i) {
                deg = max(deg, p->arr[i].exp);
            }
        }
        else {
            // przechodzę do dalszej zmiennej
            for (size_t i = 0; i < PolyLength(p); ++i)
                deg = max(deg, PolyDegBy(&p->arr[i].p, var_idx - 1));
        }

        return deg;
    }
}

poly_exp_t PolyDeg(const Poly *p) {
    assert(p != NULL);

    if (PolyIsZero(p)) {
        return -1;
    }
    else if (PolyIsCoeff(p)) {
        return 0;
    }
    else {
        poly_exp_t deg = 0;

        for (size_t i = 0; i < PolyLength(p); ++i)
            //dodaję wykładniki w każdym z jednomianów
            deg = max(deg, p->arr[i].exp + PolyDeg(&p->arr[i].p));

        return deg;
    }
}

bool PolyIsEq(const Poly *p, const Poly *q) {
    assert(p != NULL && q != NULL);

    if (PolyIsCoeff(p) && PolyIsCoeff(q)) {
        return p->coeff == q->coeff;
    }
    else if (PolyIsCoeff(p) || PolyIsCoeff(q)) {
        return false;
    }
    else {
        // ilość jednomianów i każdy z nich muszą być takie same
        if (p->size != q->size) {
            return false;
        }
        else {
            for (size_t i = 0; i < p->size; ++i) {
                if (p->arr[i].exp != q->arr[i].exp
                    || !PolyIsEq(&p->arr[i].p, &q->arr[i].p))
                    return false;
            }

            return true;
        }
    }
}

Poly PolyAt(const Poly *p, poly_coeff_t x) {
    assert(p != NULL);

    if (PolyIsCoeff(p)) {
        return PolyFromCoeff(p->coeff);
    }
    else {
        Poly sol = PolyFromCoeff(0);

        for (size_t i = 0; i < PolyLength(p); ++i) {
            // tworzę wielomian będący współczynnikiem równym zmiennej
            // po podniesieniu do potęgi, przemnażam go z każdym jednomianem
            // a następnie tworzę z nich szereg
            Poly c = PolyFromCoeff(power(x, p->arr[i].exp));
            Poly mul = PolyMul(&p->arr[i].p, &c);
            Poly pom = PolyAdd(&sol, &mul);
            PolyDestroy(&sol);
            sol = PolyClone(&pom);
            PolyDestroy(&mul);
            PolyDestroy(&pom);
        }

        return sol;
    }
}

/**
 * Liczy głębokość wielomianu - ilość zmiennych.
 * @param[in] p : wielomian
 * @return ilość zmiennych
 */
static size_t PolyDepth(const Poly *p) {
    assert(p != NULL);

    if (PolyIsCoeff(p))
        return 0;

    size_t max = 0;

    for (size_t i = 0; i < PolyLength(p); ++i) {
        size_t temp = 1 + PolyDepth(&p->arr[i].p);

        if (temp > max)
            max = temp;
    }

    return max;
}

/**
 * Podnosi wielomian do danej potęgi.
 * @param[in] p : wielomian
 * @param[in] exp : potęga
 * @return potęga wielomianu
 */
static Poly PolyPower(const Poly *p, poly_exp_t exp) {
    assert(p != NULL);

    if (exp == 0) {
        return PolyFromCoeff(1);
    }
    else if (exp == 1) {
        return PolyClone(p);
    }
    else if (exp % 2 == 0) {
        Poly sq = PolyMul(p, p);
        Poly sol = PolyPower(&sq, exp / 2);
        PolyDestroy(&sq);
        return sol;
    }
    else {
        Poly sq = PolyMul(p, p);
        Poly pow = PolyPower(&sq, (exp - 1) / 2);
        PolyDestroy(&sq);
        Poly sol = PolyMul(p, &pow);
        PolyDestroy(&pow);
        return sol;
    }
}

/**
 * Wstawia wielomian pod zmienną o danej głębokości.
 * @param[in] p : wielomian, do którego będzie wstawiany inny wielomian
 * @param[in] q : wielomian wstawiany pod zmienną
 * @param[in] k : numer zmiennej
 * @return wielomian po złożeniu
 */
static Poly PolySingleCompose(const Poly *p, const Poly *q, size_t k) {
    assert(p != NULL && q != NULL);

    if (PolyIsCoeff(p)) {
        return PolyFromCoeff(p->coeff);
    }
    // zła głębokość wielomianu
    else if (k > 0) {
        Mono *monos = malloc(sizeof(Mono) * PolyLength(p));
        CHECK_PTR(monos);

        // przejście do dalszych zmiennych
        for (size_t i = 0; i < PolyLength(p); ++i)
            monos[i] = (Mono)
                    {.p = PolySingleCompose(&p->arr[i].p, q, k - 1),
                     .exp = p->arr[i].exp};

        return (Poly) {.arr = monos, .size = PolyLength(p)};
    }
    // głębokość zmiennej, która ma być podmieniona
    else {
        Poly sol = PolyFromCoeff(0);

        for (size_t i = 0; i < PolyLength(p); ++i) {
            Poly power = PolyPower(q, p->arr[i].exp);
            Poly mul = PolyMul(&p->arr[i].p, &power);
            PolyDestroy(&power);
            Poly temp = PolyAdd(&sol, &mul);
            PolyDestroy(&mul);
            PolyDestroy(&sol);
            sol = temp;
        }

        return sol;
    }
}

Poly PolyCompose(const Poly *p, size_t k, const Poly q[]) {
    assert(p != NULL);

    size_t depth = PolyDepth(p) + 1;
    Poly sol = PolyClone(p);
    Poly zero = PolyFromCoeff(0);

    // po kolei uzupełniam zmienne danymi wielomianami od końca
    for (size_t i = depth; i > 0; --i) {
        if (i <= k) {
            Poly temp = PolySingleCompose(&sol, &q[i - 1], i - 1);
            PolyDestroy(&sol);
            sol = temp;
        }
        // zeruję nieokreślone zmienne
        else {
            Poly temp = PolySingleCompose(&sol, &zero, i - 1);
            PolyDestroy(&sol);
            sol = temp;
        }
    }

    return sol;
}
