# Roll-to-Roll-system
IIOT Course Project -  Sag based speed/ Tension control in R2R system

A closed-loop Roll-to-Roll system integrating embedded control, real-time sensing, and IoT-based monitoring. The system uses an ESP32 for motor control and data acquisition, and a Raspberry Pi running a Dockerized IoT stack for communication, visualization, and storage.

---

## 🚀 Overview

This project implements a **sag-based control system** for a Roll-to-Roll mechanism. The controller adjusts motor speed dynamically based on sag input, while system states (RPM, current, control signal) are streamed over MQTT and visualized in real time.

---

## ✨ Key Features

- Closed-loop control using **Proportional (P) control**
- **Sag input via Serial / IoT interface**
- **Bidirectional motor control** (MX1508 driver)
- Encoder-based **RPM estimation**
- Current sensing using ACS712
- Real-time data streaming over MQTT
- Visualization using Node-RED dashboards
- Time-series storage using InfluxDB
- Fully containerized IoT stack using Docker (IoTStack)
- Edge deployment on Raspberry Pi (low latency, offline capable)

---

## 🏗️ System Architecture
      +-------------------+
      |      nodes        |
      |-------------------|
      |  Control (P)      |
      |  Encoder (RPM)    |
      |  Current Sensor   |
      |           |
      +--------+----------+
               |
               | MQTT
               v
    +------------------------+
    | Raspberry Pi (Docker) |
    |------------------------|
    | Mosquitto (MQTT)      |
    | Node-RED              |
    | InfluxDB              |
    +------------------------+
               |
               v
         Dashboard UI


         ## 🔧 Hardware Components

- ESP32 Dev Module  
- DC Motor  
- MX1508 Dual H-Bridge Motor Driver  
- Incremental Encoder  
- ACS712 Current Sensor  
- Power Supply  
- 3D Printed Mechanical Assembly  

---

## 🧠 Control Strategy

The system uses a **Proportional Controller**:


## 🌐 IoT Stack (Raspberry Pi)

The IoT backend is hosted locally using **Docker via IoTStack**, enabling modular and reproducible deployment.

### Services Used

- **Mosquitto (MQTT Broker)**  
  Lightweight publish–subscribe communication

- **Node-RED**  
  Flow-based programming for:
  - Data processing  
  - Dashboard visualization  

- **InfluxDB**  
  Time-series database for logging system data  

### Advantages

- Low latency (edge processing)  
- Offline capability  
- Easy scalability  
- Container isolation  

---

## 🐳 IoTStack Setup (Raspberry Pi)

```bash
git clone https://github.com/SensorsIot/IOTstack.git
cd IOTstack
./menu.sh

docker-compose up -d

```
