// ============================================================
//  FRONTEND LOGIC
//  This file talks to the C++ backend using fetch().
//  Every request also sends WHICH bus is selected (bus=2),
//  so the backend knows which Bus object to use.
// ============================================================

let currentBus = 1;          // id of the selected bus
let selectedSeat = null;

// ---------- Small helpers ----------

// Send a POST request with form data (bus=1&seat=5&name=Abhishek)
async function postData(url, data) {
  const res = await fetch(url, {
    method: "POST",
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body: new URLSearchParams(data)
  });
  return res.json();
}

async function getData(url) {
  return (await fetch(url)).json();
}

// Show a green (success) or red (error) message box
function showMessage(result) {
  const box = document.getElementById("message");
  box.textContent = result.message;
  box.className = "message " + (result.ok ? "ok" : "error");
}

// ---------- Load data from the backend ----------

// Draws one card per bus. Clicking a card selects that bus.
async function loadBuses() {
  const buses = await getData("/api/buses");
  const list = document.getElementById("busList");
  list.innerHTML = "";

  buses.forEach(bus => {
    const card = document.createElement("button");
    card.className = "bus-card" + (bus.id === currentBus ? " active" : "");
    card.innerHTML = `
      <span class="bus-route"></span>
      <span class="bus-meta"></span>
      <span class="bus-meta"></span>
      <span class="bus-seats"></span>`;
    const parts = card.querySelectorAll("span");
    parts[0].textContent = bus.route;
    parts[1].textContent = `${bus.busNumber}`;
    parts[2].textContent = `🕒 Departs ${bus.departure}  •  Journey: ${bus.journeyTime}`;
    parts[3].textContent = `Rs. ${bus.fare}  •  ${bus.available} seats left`;
    card.onclick = () => selectBus(bus.id);
    list.appendChild(card);

    if (bus.id === currentBus) {
      document.getElementById("busInfo").textContent =
        `${bus.route}  •  Bus ${bus.busNumber}  •  Departs ${bus.departure}  •  Fare Rs. ${bus.fare}`;
      document.getElementById("seatTitle").textContent = `Seat Map: ${bus.route}`;
    }
  });
}

async function loadSeats() {
  const seats = await getData(`/api/seats?bus=${currentBus}`);
  const map = document.getElementById("seatMap");
  map.innerHTML = "";
  let booked = 0;

  seats.forEach((s, index) => {
    // After every 2nd seat in a row, add an empty space for the aisle
    if (index % 4 === 2) map.appendChild(document.createElement("div"));

    const btn = document.createElement("button");
    btn.textContent = s.seat;
    btn.className = "seat " + (s.booked ? "booked" : "free");
    if (s.booked) {
      booked++;
      btn.disabled = true;
    } else {
      if (s.seat === selectedSeat) btn.className = "seat selected";
      btn.onclick = () => selectSeat(s.seat);
    }
    map.appendChild(btn);
  });

  document.getElementById("counts").textContent =
    `Booked: ${booked}   •   Available: ${seats.length - booked}`;
}

async function loadPassengers() {
  const data = await getData(`/api/passengers?bus=${currentBus}`);
  const table = document.getElementById("passengerTable");
  table.innerHTML = "";

  if (data.passengers.length === 0) {
    table.innerHTML = `<tr><td colspan="2" class="muted">No bookings on this bus yet.</td></tr>`;
  }
  data.passengers.forEach(p => {
    const row = document.createElement("tr");
    const seatCell = document.createElement("td");
    const nameCell = document.createElement("td");
    seatCell.textContent = p.seat;
    nameCell.textContent = p.name;        // textContent = safe, no HTML injection
    row.append(seatCell, nameCell);
    table.appendChild(row);
  });

  document.getElementById("total").textContent = `Total collection for this bus: Rs. ${data.total}`;
}

function refreshAll() {
  loadBuses();
  loadSeats();
  loadPassengers();
}

// ---------- User actions ----------

function selectBus(id) {
  currentBus = id;
  selectedSeat = null;
  document.getElementById("bookSeat").value = "";
  document.getElementById("message").className = "message hidden";
  refreshAll();
}

function selectSeat(seatNo) {
  selectedSeat = seatNo;
  document.getElementById("bookSeat").value = seatNo;
  loadSeats();
}

document.getElementById("bookForm").addEventListener("submit", async (e) => {
  e.preventDefault();                       // stop the page from reloading
  const result = await postData("/api/book", {
    bus: currentBus,
    seat: document.getElementById("bookSeat").value,
    name: document.getElementById("bookName").value
  });
  showMessage(result);
  if (result.ok) {
    selectedSeat = null;
    e.target.reset();
  }
  refreshAll();
});

document.getElementById("cancelForm").addEventListener("submit", async (e) => {
  e.preventDefault();
  const result = await postData("/api/cancel", {
    bus: currentBus,
    seat: document.getElementById("cancelSeat").value
  });
  showMessage(result);
  if (result.ok) e.target.reset();
  refreshAll();
});

document.getElementById("searchForm").addEventListener("submit", async (e) => {
  e.preventDefault();
  const name = document.getElementById("searchName").value;
  const result = await getData("/api/search?name=" + encodeURIComponent(name));
  showMessage(result);
});

// ---------- Chatbot ----------
// The chatbot "brain" is in C++ (server.cpp).
// This part only shows the chat window and sends messages to /api/chat.

const chatBox = document.getElementById("chatBox");
const chatMessages = document.getElementById("chatMessages");

function addBubble(text, who) {
  const bubble = document.createElement("div");
  bubble.className = "bubble " + who;        // "bot" or "user"
  bubble.textContent = text;
  chatMessages.appendChild(bubble);
  chatMessages.scrollTop = chatMessages.scrollHeight;   // scroll to newest message
}

async function sendChat(text) {
  addBubble(text, "user");
  const result = await postData("/api/chat", { bus: currentBus, message: text });
  addBubble(result.message, "bot");
  refreshAll();
}

document.getElementById("chatToggle").onclick = () => {
  chatBox.classList.toggle("hidden");
  if (chatMessages.children.length === 0)
    addBubble("Hi! I'm the Bus Assistant. Ask me about buses, fares, timing or seats.", "bot");
};
document.getElementById("chatClose").onclick = () => chatBox.classList.add("hidden");

document.getElementById("chatForm").addEventListener("submit", (e) => {
  e.preventDefault();
  const input = document.getElementById("chatInput");
  sendChat(input.value);
  input.value = "";
});

document.querySelectorAll(".chip").forEach(chip => {
  chip.onclick = () => sendChat(chip.textContent);
});

// Load everything when the page opens
refreshAll();
