# Recipe that tells Render how to build and run the C++ server

# Start from an image that already has the g++ compiler
FROM gcc:13

WORKDIR /app

# Copy the project files into the image
COPY . .

# Compile the server (-pthread is needed because the server uses threads)
RUN g++ -std=c++17 -O2 -o server server.cpp -pthread

# Render tells us the port through PORT; 8080 is used locally
EXPOSE 8080

# Start the server
CMD ["./server"]
