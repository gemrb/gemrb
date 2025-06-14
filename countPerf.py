#! /usr/bin/env python3

import re
from collections import defaultdict

def przetworz_plik(nazwa_pliku):
    tagi_us = defaultdict(int)  # Używamy defaultdict, aby automatycznie inicjować sumy mikrosekund
    tagi_count = defaultdict(int)  # Używamy defaultdict, aby automatycznie inicjować sumy mikrosekund

    # Wyrażenie regularne do dopasowania tagów i mikrosekund
    wzorzec = r'}}\s*(\w+)\s*(\d+)us'

    # Otwieramy plik
    with open(nazwa_pliku, 'r', encoding='utf-8') as plik:
        for linia in plik:
            # Szukamy pasujących wzorców w każdej linii
            dopasowanie = re.search(wzorzec, linia)
            if dopasowanie:
                tag = dopasowanie.group(1)  # Pierwsza grupa to tag
                us = int(dopasowanie.group(2))  # Druga grupa to liczba mikrosekund
                tagi_us[tag] += us  # Dodajemy mikrosekundy do odpowiedniego tagu
                tagi_count[tag] += 1

    # Zwracamy wynik
    return (tagi_us, tagi_count)

def wyswietl_wyniki(tag_us, tagi_count):
    for tag, suma_us in tag_us.items():
        print(f"Tag: {tag},\tTime: {suma_us} us | {(suma_us / 1000) : .2f} ms\tAvg: {suma_us / tagi_count[tag]} us | {((suma_us / 1000) / tagi_count[tag]) : .2f} ms")
    try:
        caches = ["cacheBase", "cacheImpr"]
        print(f"\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> % cacheBase to cacheImpr: { tag_us[caches[0]] * 100 / tag_us[caches[1]]  : .2f}%\n")
        for cache in caches:
            print(f"> Cache: {cache}")
            sum_us = tag_us[cache] + tag_us['improved']
            print(f"improved + cache: {sum_us} us | {sum_us / 1000 : .2f} ms")
            difference = tag_us['original'] - sum_us
            print(f"overall improvement: {difference} us | {difference / 1000 : .2f} ms")
            # x% - improved
            # 100% - original
            print(f"% improvement without cache: {tag_us['original'] * 100 / tag_us['improved'] : .2f}%")
            print(f"% improvement with cache: {tag_us['original'] * 100 / sum_us : .2f}%")
    except:
        print("...not counting %, some exception happened")

# Przykład użycia
nazwa_pliku = 'perf.txt'  # Podaj nazwę pliku, który chcesz przetworzyć
tagi_us, tagi_count = przetworz_plik(nazwa_pliku)
wyswietl_wyniki(tagi_us, tagi_count)

