// ============================================================
//  BUS TICKET RESERVATION SYSTEM
//  Industrial Training Project - C++ Programming
//
//  Concepts used:
//   - Classes & Objects (Bus class)
//   - Encapsulation (private data, public functions)
//   - Constructor (loads saved bookings automatically)
//   - 2D Array (seat layout: 10 rows x 4 seats)
//   - File Handling (bookings saved in bookings.txt)
//   - Input validation
// ============================================================

#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <limits>
using namespace std;

const int ROWS = 10;                 // 10 rows in the bus
const int COLS = 4;                  // 4 seats per row (2 + aisle + 2)
const int FARE = 500;                // ticket price in rupees
const string DATA_FILE = "bookings.txt";

class Bus {
private:
    string busNumber;
    string route;
    string seats[ROWS][COLS];        // empty string = seat is free, else passenger name

    // Convert seat number (1-40) to row and column in the 2D array
    int getRow(int seatNo) { return (seatNo - 1) / COLS; }
    int getCol(int seatNo) { return (seatNo - 1) % COLS; }
    bool isValidSeat(int seatNo) { return seatNo >= 1 && seatNo <= ROWS * COLS; }

public:
    // Constructor: sets bus details and loads old bookings from file
    Bus(string number, string r) {
        busNumber = number;
        route = r;
        loadFromFile();
    }

    void showBusInfo() {
        cout << "\n Bus No : " << busNumber << "\n Route  : " << route
             << "\n Fare   : Rs. " << FARE << "\n";
    }

    // Prints the seat layout. Booked seats are shown as XX
    void showSeatMap() {
        cout << "\n        SEAT MAP  (XX = booked)\n";
        cout << "   -------------------------------\n";
        for (int r = 0; r < ROWS; r++) {
            cout << "   ";
            for (int c = 0; c < COLS; c++) {
                int seatNo = r * COLS + c + 1;
                if (seats[r][c] == "")
                    cout << "[" << setw(2) << setfill('0') << seatNo << "] ";
                else
                    cout << "[XX] ";
                if (c == 1) cout << "    ";   // aisle in the middle
            }
            cout << "\n";
        }
        cout << setfill(' ');
        cout << "   -------------------------------\n";
        cout << "   Booked: " << bookedCount() << "   Available: "
             << ROWS * COLS - bookedCount() << "\n";
    }

    bool bookSeat(int seatNo, string name) {
        if (!isValidSeat(seatNo)) {
            cout << " Invalid seat number! Choose between 1 and " << ROWS * COLS << ".\n";
            return false;
        }
        int r = getRow(seatNo), c = getCol(seatNo);
        if (seats[r][c] != "") {
            cout << " Sorry, seat " << seatNo << " is already booked.\n";
            return false;
        }
        seats[r][c] = name;
        saveToFile();
        cout << "\n ===== TICKET =====\n";
        cout << " Passenger : " << name << "\n Seat No   : " << seatNo
             << "\n Bus No    : " << busNumber << "\n Route     : " << route
             << "\n Fare      : Rs. " << FARE << "\n ==================\n";
        return true;
    }

    bool cancelSeat(int seatNo) {
        if (!isValidSeat(seatNo)) {
            cout << " Invalid seat number!\n";
            return false;
        }
        int r = getRow(seatNo), c = getCol(seatNo);
        if (seats[r][c] == "") {
            cout << " Seat " << seatNo << " is not booked.\n";
            return false;
        }
        cout << " Booking of " << seats[r][c] << " (seat " << seatNo << ") cancelled.\n";
        cout << " Refund: Rs. " << FARE << "\n";
        seats[r][c] = "";
        saveToFile();
        return true;
    }

    void searchPassenger(string name) {
        bool found = false;
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                if (seats[r][c] == name) {
                    cout << " " << name << " has seat number " << r * COLS + c + 1 << ".\n";
                    found = true;
                }
        if (!found) cout << " No booking found for " << name << ".\n";
    }

    void showPassengers() {
        if (bookedCount() == 0) {
            cout << " No bookings yet.\n";
            return;
        }
        cout << "\n " << left << setw(8) << "Seat" << "Passenger Name\n";
        cout << " ----------------------------\n";
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                if (seats[r][c] != "")
                    cout << " " << setw(8) << r * COLS + c + 1 << seats[r][c] << "\n";
        cout << right;
        cout << " Total collection: Rs. " << bookedCount() * FARE << "\n";
    }

    int bookedCount() {
        int count = 0;
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                if (seats[r][c] != "") count++;
        return count;
    }

    // Saves every booked seat as:  seatNumber|name
    void saveToFile() {
        ofstream fout(DATA_FILE);
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                if (seats[r][c] != "")
                    fout << r * COLS + c + 1 << "|" << seats[r][c] << "\n";
        fout.close();
    }

    // Reads the file line by line and fills the seats array again
    void loadFromFile() {
        ifstream fin(DATA_FILE);
        if (!fin) return;                       // no file yet = no bookings
        string line;
        while (getline(fin, line)) {
            size_t pos = line.find('|');
            if (pos == string::npos) continue;
            int seatNo = stoi(line.substr(0, pos));
            string name = line.substr(pos + 1);
            if (isValidSeat(seatNo))
                seats[getRow(seatNo)][getCol(seatNo)] = name;
        }
        fin.close();
    }
};

// Reads an integer safely (stops the program from breaking on letters)
int readInt() {
    int x;
    while (!(cin >> x)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << " Please enter a number: ";
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    return x;
}

string readName() {
    string name;
    getline(cin, name);
    while (name == "") {
        cout << " Name cannot be empty: ";
        getline(cin, name);
    }
    return name;
}

int main() {
    Bus bus("RJ-14-PB-2026", "Jaipur to Delhi");
    int choice;

    cout << "==========================================\n";
    cout << "     BUS TICKET RESERVATION SYSTEM\n";
    cout << "==========================================\n";

    do {
        cout << "\n----------- MENU -----------\n";
        cout << " 1. Show bus details\n";
        cout << " 2. View seat map\n";
        cout << " 3. Book a ticket\n";
        cout << " 4. Cancel a ticket\n";
        cout << " 5. Search passenger\n";
        cout << " 6. Show all passengers\n";
        cout << " 7. Exit\n";
        cout << " Enter your choice: ";
        choice = readInt();

        switch (choice) {
            case 1: bus.showBusInfo(); break;
            case 2: bus.showSeatMap(); break;
            case 3: {
                bus.showSeatMap();
                cout << " Enter seat number: ";
                int seatNo = readInt();
                cout << " Enter passenger name: ";
                string name = readName();
                bus.bookSeat(seatNo, name);
                break;
            }
            case 4: {
                cout << " Enter seat number to cancel: ";
                int seatNo = readInt();
                bus.cancelSeat(seatNo);
                break;
            }
            case 5: {
                cout << " Enter passenger name: ";
                bus.searchPassenger(readName());
                break;
            }
            case 6: bus.showPassengers(); break;
            case 7: cout << " Thank you! Bookings are saved.\n"; break;
            default: cout << " Invalid choice, try again.\n";
        }
    } while (choice != 7);

    return 0;
}
