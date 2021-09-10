#!/bin/bash
# Skrypt testujący małe zadanie z IPP pod wzgledem poprawności i braku
# wycieków pamięci za pomocą programu valgrind. Przyjmuje 2 argumenty:
# $1 - nazwa programu wykonywalnego
# $2 - katalog z plikami do testowania
# Autor: Michał Skwarek

PROGRAM=$1
DIRECTORY=$2
RED='\033[0;31m'
GREEN='\033[0;32m'
BLANK='\033[0m'
VALGRINDFLAGS="--error-exitcode=123 --leak-check=full --show-leak-kinds=all\
 --errors-for-leak-kinds=all"

ALL=0
CORRECT=0
WRONG=0
LEAK=0

for f in $DIRECTORY/*.in; do
    
    NAME=${f#$DIRECTORY/};
    OUTCOME="$(mktemp)";

    echo "Sprawdzam poprawnosc programu dla testu $NAME..."

    ./$PROGRAM <"$f" 1>$OUTCOME.out 2>$OUTCOME.err

    diff "${f%.in}".out $OUTCOME.out &>/dev/null
    OUTPUT_CHECK=$?

    diff "${f%.in}".err $OUTCOME.err &>/dev/null
    ERROR_CHECK=$?
    
    if [ $OUTPUT_CHECK -eq 0 -a $ERROR_CHECK -eq 0 ];
    then
	echo -e "${GREEN}Wynik poprawny!${BLANK}";
	((CORRECT+=1));
	((ALL+=1));
    else
	echo -e "${RED}Bledny wynik programu.${BLANK}";
	((WRONG+=1));
	((ALL+=1));
    fi

    echo "Sprawdzam potencjalne wycieki pamieci...";

    valgrind $VALGRINDFLAGS ./$PROGRAM <"$f" 1>/dev/null 2>/dev/null
    VALGRIND_CHECK=$?

    if [ $VALGRIND_CHECK -eq 0 ];
    then
	echo -e "${GREEN}Nie wykryto wyciekow pamieci!${BLANK}";
    else
	echo -e "${RED}Wykryto wycieki pamieci.${BLANK}";
	((LEAK+=1));
    fi

    rm -f $OUTCOME.out $OUTCOME.err
    echo ""

done;

echo "Liczba wszystkich testow: $ALL";
echo "Liczba poprawnych wynikow: $CORRECT";
echo "Liczba blednych wynikow: $WRONG";
echo "Liczba testow z wyciekami pamieci: $LEAK";
