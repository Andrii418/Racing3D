#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <set>

class ProfileManager {
public:
    int balance = 0;
    std::set<std::string> unlockedCars;
    std::string currentCarId = "race";

    // «бер≥гаЇ стан у файл save.txt у папц≥ з проЇктом
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

    // «авантажуЇ дан≥ при старт≥ гри
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
            // якщо файлу немаЇ (перший запуск), даЇмо стартов≥ ресурси
            balance = 500;
            currentCarId = "race";
            unlockedCars.insert("race");
            save();
        }
    }

    void addMoney(int amount) {
        balance += amount;
        save();
    }

    bool isUnlocked(std::string id) {
        return unlockedCars.find(id) != unlockedCars.end();
    }

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