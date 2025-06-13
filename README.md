# 🚀 **ESP32-CAM Cloud-Based Number Plate Recognition System**

A **next-gen IoT + Cloud solution** that leverages `ESP32-CAM`, `Google Vision API`, and `Google Cloud Run` to:

* Automatically capture vehicle images
* Extract number plates via OCR
* Upload to Google Drive **organized by date**
* Log data to Google Sheets in real time

---

## 🌐 **High-Level Architecture**

```
ESP32-CAM ─▶ Cloud Run API (Flask Server) ─▶ Vision API OCR ─▶ Google Drive (Date-wise Folders) + Sheets Log
```

➡️ *Hardware meets cloud computing for a fully automated number plate logging system.*

---

## 🛠 **Technologies Used**

| Component         | Stack / Tools                                  |
| ----------------- | ---------------------------------------------- |
| **Hardware**      | ESP32-CAM (AI Thinker module)                  |
| **Firmware**      | Arduino C++ (ESP-IDF libraries)                |
| **Cloud Backend** | Google Cloud Run + Flask API (Python 3.x)      |
| **OCR Service**   | Google Cloud Vision API                        |
| **Storage**       | Google Drive (Auto-date-wise folder structure) |
| **Data Log**      | Google Sheets API (Cloud-synced logging)       |
| **Security**      | HTTPS, OAuth 2.0 for API auth                  |

---

## 📂 **Date-wise Uploads to Google Drive**

Each captured image is:

* Uploaded to **Google Drive**
* Automatically stored in a **folder named by current date (YYYY-MM-DD)**
* Enables easy retrieval, auditing, and long-term archival
* Example structure:

```
Google Drive
└── ESP32-CAM
    ├── 2025-06-13
    │    ├── esp32cam_1623578491234.jpg
    │    └── esp32cam_1623578505678.jpg
    └── 2025-06-14
         └── esp32cam_1623664896789.jpg
```

📌 **Timestamped filenames + organized folders → seamless integration with Google Sheets logs.**

---

## ⚙ **Firmware Process Flow**

```cpp
// ESP32-CAM
1️⃣ Capture JPEG frame
2️⃣ Base64 encode image
3️⃣ HTTPS POST to Flask Cloud API (Cloud Run)
```

---

## 🧠 **Server-Side Processing**

```python
# Flask server
1️⃣ Receive JSON payload (image + metadata)
2️⃣ Call Vision API for text detection
3️⃣ Save image to Drive (auto-create date folder)
4️⃣ Append log to Sheets (plate, Drive URL, timestamp)
5️⃣ Return OCR result as JSON response
```

---

## 🚀 **Deployment Steps**

### ESP32-CAM

✅ Configure your WiFi + server endpoint
✅ Upload via Arduino IDE or PlatformIO

### Flask Cloud API

```bash
# Build Docker image
gcloud builds submit --tag gcr.io/YOUR_PROJECT/vision-server

# Deploy to Cloud Run
gcloud run deploy vision-server \
  --image gcr.io/YOUR_PROJECT/vision-server \
  --platform managed \
  --allow-unauthenticated
```

---

## 📊 **Example Google Sheets Log**


## 📈 **Google Sheets Example**

|    Date   |     Time     | Plate Number | Image URL                            |
| --------- |------------- | ------------ | ------------------------------------ |
| 2025-06-13|    11:49:00  |  KA01AB1234  | https://drive.google.com/...         |


---

## 💻 **Folder Structure**

```
├── esp32_cam_firmware/
│   └── esp32_cam_code.ino
├── server/
│   ├── app.py
│   ├── requirements.txt
│   └── Dockerfile
└── README.md
```

---

## 🌟 **Highlights**

✅ Advanced cloud-native design
✅ Fully automated end-to-end pipeline
✅ Scales with serverless architecture
✅ Organized date-wise Drive structure
✅ Real-time data logging + storage redundancy

---

## 💡 **Potential Extensions**

* Real-time dashboard (Firebase / React front-end)
* Offline fallback with SD card storage
* AI-based plate validation (TensorFlow Lite)

---

## 🔐 **Security**

* HTTPS communication (ESP32 client with `WiFiClientSecure`)
* OAuth-secured access to Google APIs
* Cloud Run container isolation

---

## 📌 **License**

MIT — Open to adapt, extend, and improve.
