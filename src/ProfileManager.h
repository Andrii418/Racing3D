#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <set>

/**
 * @file ProfileManager.h
 * @brief Prosty menedżer profilu gracza: saldo, odblokowane samochody i zapis/odczyt z pliku.
 */

 /**
  * @brief Zarządza stanem profilu gracza (saldo, odblokowane auta, aktualny wybór) i zapisuje go do pliku.
  *
  * Dane są zapisywane do `save.txt` w formacie:
  * - linia 1: `balance`
  * - linia 2: `currentCarId`
  * - kolejne tokeny: identyfikatory z `unlockedCars` (oddzielone spacjami)
  */
class ProfileManager {
public:
    /** @brief Aktualny stan konta gracza. */
    int balance = 0;

    /** @brief Zbiór identyfikatorów odblokowanych samochodów. */
    std::set<std::string> unlockedCars;

    /** @brief Id aktualnie wybranego samochodu. */
    std::string currentCarId = "race";

    /**
     * @brief Zapisuje aktualny stan profilu do pliku `save.txt`.
     */
    void save() {
        std::ofstream file("save.txt");
        if (file.is_open()) {
            file << balance << "\n";
            file << currentCarId << "\n";
            for (const auto& id : unlockedCars) {
                file << id << " ";
            }
            file.close();
        }
    }

    /**
     * @brief Wczytuje stan profilu z pliku `save.txt`.
     *
     * Jeśli plik nie istnieje, inicjalizuje profil wartościami domyślnymi i od razu go zapisuje.
     */
    void load() {
        std::ifstream file("save.txt");
        if (file.is_open()) {
            file >> balance;
            file >> currentCarId;
            std::string id;
            unlockedCars.clear();
            while (file >> id) {
                unlockedCars.insert(id);
            }
            file.close();
        }
        else {
            balance = 500;
            currentCarId = "race";
            unlockedCars.insert("race");
            save();
        }
    }

    /**
     * @brief Dodaje pieniądze do salda i zapisuje profil.
     * @param amount Kwota do dodania.
     */
    void addMoney(int amount) {
        balance += amount;
        save();
    }

    /**
     * @brief Sprawdza, czy samochód jest odblokowany.
     * @param id Id samochodu.
     * @return `true` jeśli samochód jest odblokowany; inaczej `false`.
     */
    bool isUnlocked(std::string id) {
        return unlockedCars.find(id) != unlockedCars.end();
    }

    /**
     * @brief Próbuje kupić samochód.
     *
     * Zakup jest możliwy, gdy saldo jest wystarczające i samochód nie jest jeszcze odblokowany.
     *
     * @param id Id samochodu.
     * @param price Cena samochodu.
     * @return `true` jeśli zakup się powiódł; inaczej `false`.
     */
    bool buyCar(std::string id, int price) {
        if (balance >= price && !isUnlocked(id)) {
            balance -= price;
            unlockedCars.insert(id);
            save();
            return true;
        }
        return false;
    }
};