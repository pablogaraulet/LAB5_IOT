# Lab 5 – Activity Monitor

# File Structure and How to Run the Code

This lab is divided into three parts:

- **Part A** is implemented in the file `main.cpp` (currently outside the `src` folder).
- **Part B** is implemented in the file `src/part_B.cpp`.
- **server.py** contains the backend code used in Part B for processing activity classification in the cloud.
- **PART_C_LAB5.pdf** includes the analysis and performance comparison between Part A and Part B.

We structured the code this way because PlatformIO only compiles the file located inside the `src/` folder. Therefore:

- If you want to compile and upload **Part A**, you need to **move `main.cpp` into `src/`** and **move `part_B.cpp` out of it**.
- If you want to run **Part B**, make sure `part_B.cpp` is inside `src/` and `main.cpp` is outside.

#CONECTIONS FOR THIS LAB
![image](https://github.com/user-attachments/assets/90ca61f1-85b8-464c-8aca-165b12621d1f)
![image](https://github.com/user-attachments/assets/596ff287-cee1-4a37-aecf-1f36ebbbb152)
![image](https://github.com/user-attachments/assets/5bbddb93-5978-434f-9672-2d11a494e2a9)


# Part A – BLE Activity Monitor

In this part, we built a Bluetooth-based activity monitor using the ESP32 board and an LSM6DSO accelerometer. The board reads acceleration data, processes it locally to distinguish between *steps* and *jumps*, and transmits the results via BLE to a smartphone using an app like nRF Connect.

The activity classification is done entirely on-device, ensuring low latency and independence from network connectivity.

DEMO PART A:  https://youtube.com/shorts/qoPYlF6i3zU?feature=share


# Part B – Cloud Activity Monitor

In the cloud-based design, the ESP32 captures raw sensor readings and sends them to a cloud server via HTTP. The backend server (implemented in `server.py` using Flask) receives the data, performs activity classification, and responds with the result.

This setup allows centralized processing, scalability, and remote access, but introduces additional latency due to data transmission and remote computation.

The cloud server exposes the following endpoints:
- `/sensor` – accepts POST data from the ESP32
- `/reset` – resets the counters
- `/stats` and `/` – allow viewing current counts via browser

DEMO PART B: https://youtube.com/shorts/Ocs1S-hkaQU?feature=share

## Part C – Activity Monitor Performance Comparison

Part C is documented in the file `PART_C_LAB5.pdf`, included in the repository. It contains a performance comparison between the BLE (Part A) and Cloud (Part B) approaches, analyzing total detection latency from sensor read to classification.
