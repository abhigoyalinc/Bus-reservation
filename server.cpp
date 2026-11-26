// ============================================================
//  BUS TICKET RESERVATION SYSTEM - BACKEND (C++ Web Server)
//  Industrial Training Project - C++ Programming
//
//  What this file does:
//   1. The Bus class holds the data and logic for ONE bus
//      (2D seat array, booking, cancelling, file handling, chatbot).
//   2. main() creates SEVERAL Bus objects and stores them in a
//      vector<Bus>, so the system supports multiple buses.
//   3. Starts a web server (cpp-httplib library) with API routes
//      like /api/book. The frontend calls them and gets JSON back.
//   4. Also serves the frontend files (HTML/CSS/JS) from "public".
//
//  Concepts used: classes & objects, vector of objects,
//  encapsulation, constructor, 2D array, file handling,
//  string processing, mutex (thread safety), HTTP and JSON.
// ============================================================

#include "httplib.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <mutex>
#include <cstdlib>
#include <cctype>
using namespace std;

const int ROWS = 10;                 // 10 rows in every bus
const int COLS = 4;                  // 4 seats per row (2 + aisle + 2)

// ======================= HELPER FUNCTIONS =======================

// Convert text to lowercase so "FARE" and "fare" match the same way
string toLower(string s) {
    for (char& ch : s) ch = tolower((unsigned char)ch);
    return s;
}

// True if the message contains ANY of the given keywords
bool containsAny(const string& msg, const vector<string>& words) {
    for (const string& w : words)
        if (msg.find(w) != string::npos) return true;
    return false;
}

// Find the first number in the message ("is seat 12 free" -> 12). Returns -1 if none.
int findNumber(const string& msg) {
    for (size_t i = 0; i < msg.size(); i++) {
        if (isdigit((unsigned char)msg[i])) {
            int num = 0;
            while (i < msg.size() && isdigit((unsigned char)msg[i]) && num < 1000) {
                num = num * 10 + (msg[i] - '0');
                i++;
            }
            return num;
        }
    }
    return -1;
}

// Make text safe to put inside JSON
string jsonEscape(const string& s) {
    string out;
    for (char ch : s) {
        if (ch == '"')       out += "\\\"";
        else if (ch == '\\') out += "\\\\";
        else if (ch == '\n') out += "\\n";
        else if ((unsigned char)ch < 0x20) continue;   // skip control characters
        else out += ch;
    }
    return out;
}

// Builds a small reply like {"ok":true,"message":"..."}
string reply(bool ok, const string& message) {
    return string("{\"ok\":") + (ok ? "true" : "false") +
           ",\"message\":\"" + jsonEscape(message) + "\"}";
}

// Trim spaces from both ends of a string
string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// ========================= THE BUS CLASS =========================
class Bus {
private:
    int id;
    string busNumber;
    string route;                    // e.g. "Jaipur to Delhi"
    string departure;                // e.g. "08:00 AM"
    string journeyTime;              // e.g. "about 5 hours"
    int fare;
    string dataFile;                 // each bus has its own file
    string seats[ROWS][COLS];        // "" = free seat, otherwise passenger name

    int getRow(int seatNo) { return (seatNo - 1) / COLS; }
    int getCol(int seatNo) { return (seatNo - 1) % COLS; }
    bool isValidSeat(int seatNo) { return seatNo >= 1 && seatNo <= ROWS * COLS; }

public:
    // Constructor: set the bus details and load its old bookings
    Bus(int busId, string number, string r, string dep, string jt, int price) {
        id = busId;
        busNumber = number;
        route = r;
        departure = dep;
        journeyTime = jt;
        fare = price;
        dataFile = "bookings_" + to_string(id) + ".txt";
        loadFromFile();
    }

    // Getters (read-only access to private data)
    int getId() { return id; }
    string getNumber() { return busNumber; }
    string getRoute() { return route; }
    string getDeparture() { return departure; }
    int getFare() { return fare; }

    // Destination = the part of the route after " to "  ("Jaipur to Delhi" -> "Delhi")
    string getDestination() {
        size_t pos = route.find(" to ");
        return pos == string::npos ? route : route.substr(pos + 4);
    }

    int bookedCount() {
        int count = 0;
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                if (seats[r][c] != "") count++;
        return count;
    }
    int availableCount() { return ROWS * COLS - bookedCount(); }

    // One-line summary, used by the chatbot
    string summary() {
        return busNumber + " (" + route + "), departs " + departure + ", fare Rs. " +
               to_string(fare) + ", " + to_string(availableCount()) + " seats free";
    }

    // Bus details as JSON
    string infoJson() {
        stringstream ss;
        ss << "{\"id\":" << id << ","
           << "\"busNumber\":\"" << jsonEscape(busNumber) << "\","
           << "\"route\":\"" << jsonEscape(route) << "\","
           << "\"departure\":\"" << jsonEscape(departure) << "\","
           << "\"journeyTime\":\"" << jsonEscape(journeyTime) << "\","
           << "\"fare\":" << fare << ","
           << "\"totalSeats\":" << ROWS * COLS << ","
           << "\"available\":" << availableCount() << "}";
        return ss.str();
    }

    // Every seat with true/false for booked (names are NOT shown here)
    string seatsJson() {
        stringstream ss;
        ss << "[";
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++) {
                int seatNo = r * COLS + c + 1;
                if (seatNo > 1) ss << ",";
                ss << "{\"seat\":" << seatNo << ",\"booked\":"
                   << (seats[r][c] != "" ? "true" : "false") << "}";
            }
        ss << "]";
        return ss.str();
    }

    string bookSeat(int seatNo, string name) {
        if (!isValidSeat(seatNo))
            return reply(false, "Invalid seat number. Choose between 1 and 40.");
        if (name == "")
            return reply(false, "Passenger name cannot be empty.");
        if (name.find('|') != string::npos)
            return reply(false, "Name cannot contain the | character.");
        int r = getRow(seatNo), c = getCol(seatNo);
        if (seats[r][c] != "")
            return reply(false, "Sorry, seat " + to_string(seatNo) + " is already booked.");
        seats[r][c] = name;
        saveToFile();
        return reply(true, "Seat " + to_string(seatNo) + " booked for " + name + " on " +
                           busNumber + " (" + route + "). Fare: Rs. " + to_string(fare));
    }

    string cancelSeat(int seatNo) {
        if (!isValidSeat(seatNo))
            return reply(false, "Invalid seat number.");
        int r = getRow(seatNo), c = getCol(seatNo);
        if (seats[r][c] == "")
            return reply(false, "Seat " + to_string(seatNo) + " is not booked on this bus.");
        string name = seats[r][c];
        seats[r][c] = "";
        saveToFile();
        return reply(true, "Booking of " + name + " (seat " + to_string(seatNo) +
                           ") cancelled. Refund: Rs. " + to_string(fare));
    }

    // Returns the seat numbers booked under this name, like "5, 12" ("" if none)
    string seatsOf(string name) {
        string found = "";
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                if (toLower(seats[r][c]) == toLower(name) && name != "") {
                    if (found != "") found += ", ";
                    found += to_string(r * COLS + c + 1);
                }
        return found;
    }

    string passengersJson() {
        stringstream ss;
        ss << "{\"passengers\":[";
        bool first = true;
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                if (seats[r][c] != "") {
                    if (!first) ss << ",";
                    first = false;
                    ss << "{\"seat\":" << r * COLS + c + 1
                       << ",\"name\":\"" << jsonEscape(seats[r][c]) << "\"}";
                }
        ss << "],\"total\":" << bookedCount() * fare << "}";
        return ss.str();
    }

    // ======================= CHATBOT =======================
    // Rule-based chatbot for THIS bus: looks for keywords in the
    // question and answers using LIVE data from this object.
    // The order of the checks matters: specific questions come first.
    string chatReply(string message) {
        string msg = toLower(message);
        int available = availableCount();

        // 1. A question about a specific seat ("is seat 12 free?")
        int seatNo = findNumber(msg);
        if (seatNo != -1 && containsAny(msg, {"seat"})) {
            if (!isValidSeat(seatNo))
                return reply(true, "Each bus has seats 1 to 40 only. Seat " +
                                   to_string(seatNo) + " does not exist.");
            if (seats[getRow(seatNo)][getCol(seatNo)] == "")
                return reply(true, "Yes! Seat " + to_string(seatNo) + " is available on " +
                                   busNumber + ". Click it on the seat map to book it.");
            return reply(true, "Sorry, seat " + to_string(seatNo) + " is already booked on " +
                               busNumber + ".");
        }

        // 2. Greetings (only if the message STARTS with one, so "delhi" doesn't count as "hi")
        bool greeting = false;
        for (string g : {"hello", "hi", "hey", "namaste", "good morning", "good evening"})
            if (msg.rfind(g, 0) == 0 &&
                (msg.size() == g.size() || !isalpha((unsigned char)msg[g.size()])))
                greeting = true;
        if (greeting)
            return reply(true, "Hello! I'm the Bus Assistant. Ask me about fare, timing, "
                               "available seats, a seat number, all buses, booking or cancellation.");

        // 3. Fare / price
        if (containsAny(msg, {"fare", "price", "cost", "how much", "charge", "rupees"}))
            return reply(true, "The fare for " + busNumber + " (" + route + ") is Rs. " +
                               to_string(fare) + " per seat.");

        // 4. Available seats
        if (containsAny(msg, {"available", "free", "vacant", "empty", "how many seat", "left"})) {
            if (available == 0) return reply(true, "Sorry, this bus is fully booked.");
            string list = "";
            int shown = 0;
            for (int s = 1; s <= ROWS * COLS && shown < 8; s++)
                if (seats[getRow(s)][getCol(s)] == "") {
                    if (shown > 0) list += ", ";
                    list += to_string(s);
                    shown++;
                }
            return reply(true, to_string(available) + " of 40 seats are available on " +
                               busNumber + ". Some free seats: " + list +
                               (available > 8 ? "..." : "."));
        }

        // 5. Timing
        if (containsAny(msg, {"time", "when", "depart", "leave", "start", "reach", "arrive", "duration"}))
            return reply(true, busNumber + " departs at " + departure +
                               " and the journey takes " + journeyTime + ".");

        // 6. Route / bus details
        if (containsAny(msg, {"route", "where", "destination", "going", "go to"}))
            return reply(true, "This bus runs on the route: " + route +
                               ". Type 'show all buses' to see other routes.");
        if (containsAny(msg, {"bus number", "bus no", "which bus", "number plate"}))
            return reply(true, "The selected bus number is " + busNumber + ".");

        // 7. Cancellation / refund (before "book" so "cancel my booking" lands here)
        if (containsAny(msg, {"cancel", "refund"}))
            return reply(true, "To cancel, enter your seat number in the 'Cancel a Ticket' box. "
                               "You get a full refund of the fare.");

        // 8. How to book
        if (containsAny(msg, {"book", "reserve", "ticket"}))
            return reply(true, "To book: choose a bus at the top, click a green seat, "
                               "type your name and press 'Book Ticket'. That's it!");

        // 9. Find my booking
        if (containsAny(msg, {"my seat", "find", "search", "which seat"}))
            return reply(true, "Use the 'Search Passenger' box and type your full name. "
                               "I'll search all buses for you.");

        // 10. Thanks / bye
        if (containsAny(msg, {"thank", "bye", "okay"}) || msg == "ok")
            return reply(true, "You're welcome! Have a safe journey. 🚌");

        // 11. Anything we don't understand
        return reply(true, "Sorry, I didn't understand that. Try asking: "
                           "'Show all buses', 'What is the fare?', 'Is seat 12 free?' "
                           "or 'Is there a bus to Udaipur?'");
    }

    // Save each booking as:  seatNumber|name
    void saveToFile() {
        ofstream fout(dataFile);
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                if (seats[r][c] != "")
                    fout << r * COLS + c + 1 << "|" << seats[r][c] << "\n";
        fout.close();
    }

    // Read the file again when the server starts
    void loadFromFile() {
        ifstream fin(dataFile);
        if (!fin) return;
        string line;
        while (getline(fin, line)) {
            size_t pos = line.find('|');
            if (pos == string::npos) continue;
            int seatNo = atoi(line.substr(0, pos).c_str());
            if (isValidSeat(seatNo))
                seats[getRow(seatNo)][getCol(seatNo)] = line.substr(pos + 1);
        }
        fin.close();
    }
};

// ============================ MAIN ============================
int main() {
    // A vector of Bus objects: add a new line here to add a new bus
    vector<Bus> buses;
    buses.push_back(Bus(5, "RJ-14-PE-3310", "Jaipur to Delhi", "10:00 PM", "about 5 hours", 550));
    buses.push_back(Bus(6, "RJ-14-PF-8821", "Delhi to Jaipur", "06:30 AM", "about 5 hours", 500));
    buses.push_back(Bus(1, "RJ-14-PB-2026", "Jaipur to Delhi",   "08:00 AM", "about 5 hours",   500));
    buses.push_back(Bus(2, "RJ-14-PC-1180", "Jaipur to Udaipur", "09:30 PM", "about 7 hours",   650));
    buses.push_back(Bus(3, "RJ-14-PA-4521", "Jaipur to Ajmer",   "07:00 AM", "about 2.5 hours", 250));
    buses.push_back(Bus(4, "RJ-14-PD-7734", "Jaipur to Jodhpur", "02:00 PM", "about 6 hours",   550));

    mutex lock;                      // only one request can change data at a time
    httplib::Server server;
    const string JSON = "application/json";

    // Frontend files (index.html, style.css, script.js)
    server.set_mount_point("/", "./public");
    // Find the bus the request is asking for (?bus=2). Returns nullptr if it doesn't exist.
    auto findBus = [&](const httplib::Request& req) -> Bus* {
        int id = atoi(req.get_param_value("bus").c_str());
        for (Bus& b : buses)
            if (b.getId() == id) return &b;
        return nullptr;
    };
    const string NO_BUS = reply(false, "Please choose a valid bus.");

    // GET /api/buses -> list of all buses
    server.Get("/api/buses", [&](const httplib::Request&, httplib::Response& res) {
        lock_guard<mutex> guard(lock);
        string json = "[";
        for (size_t i = 0; i < buses.size(); i++) {
            if (i > 0) json += ",";
            json += buses[i].infoJson();
        }
        res.set_content(json + "]", JSON);
    });

    // GET /api/seats?bus=1 -> all 40 seats of that bus
    server.Get("/api/seats", [&](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> guard(lock);
        Bus* bus = findBus(req);
        res.set_content(bus ? bus->seatsJson() : NO_BUS, JSON);
    });

    // POST /api/book  (bus, seat, name)
    server.Post("/api/book", [&](const httplib::Request& req, httplib::Response& res) {
        int seat = atoi(req.get_param_value("seat").c_str());
        string name = trim(req.get_param_value("name"));
        lock_guard<mutex> guard(lock);
        Bus* bus = findBus(req);
        res.set_content(bus ? bus->bookSeat(seat, name) : NO_BUS, JSON);
    });

    // POST /api/cancel  (bus, seat)
    server.Post("/api/cancel", [&](const httplib::Request& req, httplib::Response& res) {
        int seat = atoi(req.get_param_value("seat").c_str());
        lock_guard<mutex> guard(lock);
        Bus* bus = findBus(req);
        res.set_content(bus ? bus->cancelSeat(seat) : NO_BUS, JSON);
    });

    // GET /api/search?name=...  -> searches ALL buses
    server.Get("/api/search", [&](const httplib::Request& req, httplib::Response& res) {
        string name = trim(req.get_param_value("name"));
        lock_guard<mutex> guard(lock);
        string result = "";
        for (Bus& b : buses) {
            string s = b.seatsOf(name);
            if (s != "") {
                if (result != "") result += "; ";
                result += "seat " + s + " on " + b.getNumber() + " (" + b.getRoute() + ")";
            }
        }
        if (result == "") res.set_content(reply(false, "No booking found for " + name + "."), JSON);
        else res.set_content(reply(true, name + " has " + result + "."), JSON);
    });

    // GET /api/passengers?bus=1 -> bookings of that bus + total collection
    server.Get("/api/passengers", [&](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> guard(lock);
        Bus* bus = findBus(req);
        res.set_content(bus ? bus->passengersJson() : NO_BUS, JSON);
    });

    // POST /api/chat  (bus, message) -> chatbot reply
    server.Post("/api/chat", [&](const httplib::Request& req, httplib::Response& res) {
        string message = trim(req.get_param_value("message"));
        if (message.size() > 300) message = message.substr(0, 300);   // limit long input
        string msg = toLower(message);
        lock_guard<mutex> guard(lock);

        // A. Questions about ALL buses ("show all buses", "how many buses")
        if (containsAny(msg, {"buses", "all bus", "other bus", "list"})) {
            string text = "We have " + to_string(buses.size()) + " buses: ";
            for (size_t i = 0; i < buses.size(); i++) {
                if (i > 0) text += " | ";
                text += to_string(i + 1) + ") " + buses[i].summary();
            }
            res.set_content(reply(true, text), JSON);
            return;
        }

        // B. A city name in the question ("is there a bus to udaipur?")
        if (findNumber(msg) == -1) {
            for (Bus& b : buses)
                if (msg.find(toLower(b.getDestination())) != string::npos) {
                    res.set_content(reply(true, "Yes! " + b.summary() +
                                    ". Select it at the top to book."), JSON);
                    return;
                }
        }

        // C. Everything else is answered by the currently selected bus
        Bus* bus = findBus(req);
        if (!bus) bus = &buses[0];
        res.set_content(bus->chatReply(message), JSON);
    });

    // Hosting sites (like Render) tell us which port to use with PORT
    int port = 8080;
    if (getenv("PORT")) port = atoi(getenv("PORT"));

    cout << "Server running at http://localhost:" << port << endl;
    server.listen("0.0.0.0", port);
    return 0;
}
