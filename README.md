# Autonomous Solar Panel Cleaning Robot

An autonomous cleaning robot that inspects solar panels, classifies their surface condition with computer vision, and triggers cleaning only when needed — built across an ESP8266 microcontroller, a Raspberry Pi vision pipeline, and a Flask-served CNN classifier.

Highest-graded project in our cohort.

## Overview

Solar panels lose efficiency when dirty, damaged, or covered by debris — but cleaning every panel on a fixed schedule wastes water and labor. This project automates the *decision*: a Raspberry Pi captures images of the panel surface, sends them to a CNN classifier running on a PC, and the result decides whether the robot cleans, skips, or flags the panel for manual inspection.

## Architecture

```
Raspberry Pi 3B+  ──(image)──>  PC Flask Server  ──(classification)──>  Raspberry Pi
   (camera)                     (InceptionV3, 4-class)                        │
                                                                                ▼
                                                                    HTTP command
                                                                                │
                                                                                ▼
                                                                  ESP8266 NodeMCU
                                                                  (TB6612FNG driver)
                                                                                │
                                                                                ▼
                                                                    N20 motors — sliding
                                                                    cleaning mechanism
```

- **Raspberry Pi 3B+** — captures panel images and coordinates the pipeline
- **PC Flask server** — runs an **InceptionV3** model fine-tuned for 4-class classification (`Bird-drop`, `Clean`, `Dusty`, `Physical-Damage`), achieving **91.4% accuracy**
- **ESP8266 NodeMCU** — receives cleaning commands over HTTP and drives the cleaning mechanism via a **TB6612FNG** motor driver controlling **N20 motors** on a sliding stick
- **Dashboard** — dark-themed French-language web UI with a live scanner view, classification history, and manual/auto mode toggle

## Repository structure

```
solar-panel-cleaning-robot/
├── esp8266-firmware/       # NodeMCU firmware — motor control, HTTP command listener
├── vision-server/          # Flask server, InceptionV3 inference, training script
├── dashboard/              # Web UI (scanner, history, mode toggle)
└── README.md
```
*(adjust this tree to match your actual folder layout before pushing)*

## How it works

1. The Raspberry Pi camera captures an image of the panel.
2. The image is sent over HTTP to the Flask server running InceptionV3.
3. The model classifies the surface into one of four categories.
4. Based on the result, a command (clean / skip / alert) is relayed back and forwarded over HTTP to the ESP8266.
5. The ESP8266 drives the N20 motors via the TB6612FNG to run the cleaning mechanism.
6. Every scan and decision is logged and viewable on the dashboard, alongside a manual override toggle.

## Model

- **Architecture**: InceptionV3 (transfer learning / fine-tuned)
- **Classes**: Bird-drop, Clean, Dusty, Physical-Damage
- **Accuracy**: 91.4%

*(Model weights are not included in this repo — see [Notes](#notes) below.)*

## Setup

### ESP8266 firmware
Flash `esp8266-firmware/` via the Arduino IDE or PlatformIO. Update WiFi credentials and the Flask server's IP/port in the config before flashing.

### Vision server
```bash
cd vision-server
pip install -r requirements.txt
python app.py
```

### Dashboard
Open `dashboard/index.html` in a browser, or serve it via the Flask app if it's integrated there — adjust this section once the folder structure is finalized.

## Notes

- Trained model weights (`.h5`/`.pt`) are excluded from version control — either due to size or to keep the repo lightweight. Re-train using the script in `vision-server/`, or reach out for the weights directly.
- This was a course project; hardware wiring diagrams and the full lab report are available on request.

## License

MIT
