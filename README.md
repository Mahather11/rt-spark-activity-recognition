# Full-Stack IoT Activity Recognition System

**Course:** BCA182 Embedded Systems Programming  
**Laboratory Activity No. 3**  
**Institution:** Mindanao State University - Iligan Institute of Technology

##  Group Members
*   **Mahatir D. Manding**
*   **Ali Tabrani Ismael**
*   **Aman S. Dipatuan**

---

##  Project Overview
This repository contains a complete Full-Stack Internet of Things (IoT) system designed for real-time human activity recognition (Walking vs. Running). The system captures motion data, transmits it to a cloud backend, classifies it using a Machine Learning model, and displays the results on an Android mobile application.

The project is divided into three core components:
1.  **Device Firmware (`/Firmware`):** Embedded C code running on the STM32F407 (RT-Spark) board.
2.  **Cloud Backend (`/Backend`):** Python-based server using Flask, Mosquitto MQTT, SQLite, and Scikit-Learn.
3.  **Mobile App (`/AndroidApp`):** Android application built with Kotlin and Jetpack Compose.

---

## ⚠️ Hardware Integration Challenges & Solutions
During the development of this laboratory, we encountered hardware limitations on the provided RT-Spark board. To ensure the successful demonstration of the full-stack architecture, we implemented the following engineering workarounds:

1.  **Sensor Integration Challenge:** During testing, the onboard LSM6DSL accelerometer/gyroscope returned persistent `HAL_ERROR` (NACK) responses across all I2C buses (I2C1 and I2C2) and both possible addresses (0x6A and 0x6B). Despite extensive troubleshooting, the physical sensor remained unresponsive. 
    *   **Our Solution:** To ensure the rest of the cloud and app pipeline could be rigorously tested, we implemented a synthetic data generator in the firmware. It generates realistic waveform data mimicking walking/running, proving that the system works flawlessly once sensor data is introduced.
2.  **Wi-Fi Driver Incompatibility:** The development board is equipped with an RW007 Wi-Fi module, which lacks native support for the ISM43362 driver specified in the lab manual.
    *   **Our Solution:** We developed a custom UART-to-MQTT bridge (`serial_to_mqtt.py`). The STM32 sends JSON data via USB, and the Python script acts as a gateway, forwarding the data to the local Mosquitto MQTT broker.

---

## 📸 System Demonstration

### 1. Mobile App: Real-Time Prediction
The Android Emulator successfully fetches the latest prediction from the Flask API via Volley and displays it on the UI.
<img width="1907" height="992" alt="image" src="https://github.com/user-attachments/assets/07f61ced-6b70-4a61-8bd4-f863a3acff32" />


### 2. Mobile App: Activity History
The app fulfills the requirement to display the history of records stored in the cloud database.
<img width="568" height="843" alt="image" src="https://github.com/user-attachments/assets/bc0f074b-7ca5-4263-94e9-287ef48e4551" />


### 3. Device Firmware & UART Bridge
The STM32 board generates synthetic sensor data, formats it as JSON, and transmits it over UART. The Python bridge script reads this data and publishes it to Mosquitto.
<img width="1547" height="911" alt="image" src="https://github.com/user-attachments/assets/6ce80cc3-0ccf-40c5-b039-e75cd296a945" />



### 4. Cloud Backend & Machine Learning
The Flask Web API serves the data, while the MQTT Listener receives the JSON payload and classifies it using our trained Random Forest model.
<img width="1554" height="1010" alt="image" src="https://github.com/user-attachments/assets/8738087a-afc0-401b-aacd-e4b99b1277ab" />


---

## 🛠️ Technologies Used
*   **Firmware:** C, STM32 HAL, FreeRTOS, CMSIS-Driver, coreMQTT.
*   **Backend:** Python 3, Flask, Paho-MQTT, SQLite, Scikit-Learn, Pandas.
*   **Mobile:** Kotlin, Jetpack Compose, Android Volley (HTTP library).

---

## 🚀 How to Run the Project

### 1. Backend
1. Navigate to the `/Backend` folder.
2. Install dependencies: `pip install -r requirements.txt`
3. Start the Flask API: `python app.py`
4. Start the MQTT Listener: `python mqtt_listener.py` (in a new terminal).

### 2. Firmware Bridge
1. Ensure the STM32 board is flashed and connected via USB.
2. Navigate to the `/Firmware` folder.
3. Update the `SERIAL_PORT` in `serial_to_mqtt.py` to match your COM port (e.g., `COM5`).
4. Run the bridge: `python serial_to_mqtt.py`

### 3. Mobile App
1. Open the `/AndroidApp` folder in Android Studio.
2. Ensure your `BASE_URL` in `MainActivity.kt` points to your laptop's IP (e.g., `http://192.168.1.101:5000/api/activities`).
3. Build and run the app on an Android Emulator or physical device.
