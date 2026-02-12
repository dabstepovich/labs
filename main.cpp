#include <iostream>
#include <sstream>
#include <string>


using namespace std;


struct FuelPrice {
    string fuelType;
    string date;
    double price;
    
    void parse(string input) {
        istringstream iss(input);
        string token;
        
        iss >> token;
        fuelType = token.substr(1);
        while (fuelType.back() != '"') {
            iss >> token;
            fuelType += " " + token;
        }
        fuelType.pop_back();
        
        iss >> date >> price;

    }
    
    void display() const {
        cout << "Тип топлива: " << fuelType << endl;
        cout << "Дата: " << date << endl;
        cout << "Цена: " << fixed  << price << " руб." << endl;
        cout << "-------------------" << endl;
    }
};

int main() {
    string testCases[] = {
        "\"АИ-95\" 2024.01.15 55.40",
        "\"АИ-92\" 2024.01.10 52.30",
        "\"АИ-98\" 2024.01.25 62.80",
        "\"ДТ\" 2024.02.01 59.20",
        "\"АИ-100\" 2024.02.05 68.90",
    };        

    for (int i = 0; i < 5; i++) {
        cout << "Тест " << (i + 1) << ": " << testCases[i] << endl;
        FuelPrice fp;
        fp.parse(testCases[i]);
        fp.display();
        cout << endl;
    }

        
    return 0;
}
