# Bus Ticket Reservation System (C++ Web App)

A web-based bus ticket booking system. The **backend is written in C++** and the
**frontend uses HTML, CSS and JavaScript**.

## Folder structure

```
bus-reservation-web/
├── server.cpp        ← BACKEND: C++ web server + Bus class (booking logic)
├── httplib.h         ← cpp-httplib library (lets C++ act as a web server)
├── public/           ← FRONTEND
│   ├── index.html    ← page structure
│   ├── style.css     ← look and colours
│   └── script.js     ← talks to the backend using fetch()
├── Dockerfile        ← build recipe used by Render for hosting
└── console-version/  ← original terminal version of the project
```

## How frontend and backend connect

```
Browser (index.html + script.js)
      │   fetch("/api/book", bus=1, seat=5, name=Abhishek)    ← HTTP request
      ▼
C++ server (server.cpp)  →  vector<Bus>  →  bookings_N.txt
      │   {"ok":true,"message":"Seat 5 booked..."}       ← JSON response
      ▼
script.js shows the message and redraws the seat map
```

| Route              | Method | What it does                    |
|--------------------|--------|---------------------------------|
| `/api/buses`       | GET    | List of all buses                          |
| `/api/seats`       | GET    | 40 seats of one bus (`?bus=1`)             |
| `/api/book`        | POST   | Book a seat (`bus`, `seat`, `name`)        |
| `/api/cancel`      | POST   | Cancel a seat (`bus`, `seat`)              |
| `/api/search`      | GET    | Find a passenger on ALL buses (`?name=`)   |
| `/api/passengers`  | GET    | Bookings of one bus + total (`?bus=1`)     |
| `/api/chat`        | POST   | Chatbot reply (`bus`, `message`)           |

## Multiple buses

`main()` stores the buses in a `vector<Bus>`. Each bus is a separate object with
its own route, timing, fare, seat array and file (`bookings_1.txt`, `bookings_2.txt`, ...).
To add a bus, add one line in `main()`:

```cpp
buses.push_back(Bus(5, "RJ-14-PE-9999", "Jaipur to Agra", "06:00 AM", "about 4 hours", 450));
```

## The chatbot (Bus Assistant)

The 💬 button opens a **rule-based chatbot written in C++** (`chatReply()` in `server.cpp`).

How it works:
1. The question is converted to lowercase (`toLower`).
2. If it has a number plus the word "seat", it checks that seat in the 2D array.
3. Otherwise it looks for keywords (`containsAny`), e.g. "fare", "available", "time", "cancel".
4. It answers using **live data** from the Bus object, so it always matches the seat map.
5. If nothing matches, it suggests example questions.

No external AI service or API key is needed, so it is free and always works.
**Future scope:** connect an AI service (such as Gemini or Claude) for open-ended questions.

## Run on your own computer

Linux / Mac / Windows (MSYS2 or WSL):

```
g++ -std=c++17 -O2 -o server server.cpp -pthread
./server
```

Then open **http://localhost:8080** in the browser.

> Windows with MinGW: add `-lws2_32` at the end of the g++ command.
> If it fails on Windows, use WSL or just use the online (Render) version.

## Publish online (free) with GitHub + Render

1. Make a free account on **github.com**, then create a new **public** repository
   called `bus-reservation`.
2. Upload all the files: open the repo, click **"uploading an existing file"**, drag the
   whole folder's contents in (keep the `public` folder), then **Commit changes**.
3. Make a free account on **render.com** (sign up with GitHub).
4. Click **New + → Web Service**, then choose your `bus-reservation` repository.
5. Settings: **Language/Runtime = Docker**, **Instance type = Free**. Leave the rest as default.
6. Click **Deploy Web Service**. The first build takes about 5–10 minutes.
7. When the log says `Server running...` and shows **Live**, open the link at the top,
   for example `https://bus-reservation-xxxx.onrender.com`. Share it with anyone.

### Limitations of the free plan (be honest about these in the viva)
- The free server **sleeps after 15 minutes** with no visitors. The first visit after
  that takes about 50 seconds to wake it up. **Open the link 2 minutes before your
  presentation.**
- The free server's disk is temporary, so bookings reset when the server restarts.
  A real product would use a database such as MySQL.
