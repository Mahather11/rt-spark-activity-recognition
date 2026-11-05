"""Forward JSON lines from the STM32 UART to a local MQTT broker.

Install dependencies with:
    py -m pip install pyserial paho-mqtt
"""

import json
import sys

import paho.mqtt.client as mqtt
import serial


SERIAL_PORT = "COM5"
BAUD_RATE = 115200
BROKER_HOST = "127.0.0.1"
BROKER_PORT = 1883
MQTT_TOPIC = "activity/sensor_data"


def main() -> None:
    client = mqtt.Client()
    client.connect(BROKER_HOST, BROKER_PORT, keepalive=60)
    client.loop_start()

    try:
        with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as port:
            print(f"Reading {SERIAL_PORT} at {BAUD_RATE} baud; publishing to {MQTT_TOPIC}")
            while True:
                raw_line = port.readline()
                if not raw_line:
                    continue

                line = raw_line.decode("utf-8", errors="replace").rstrip("\r\n")
                print(f"[UART RX] {line}", flush=True)
                if not line.startswith("JSON:"):
                    continue

                payload = line[len("JSON:"):].strip()
                try:
                    json.loads(payload)
                except json.JSONDecodeError as exc:
                    print(f"Skipping invalid JSON line: {exc}", file=sys.stderr)
                    continue

                result = client.publish(MQTT_TOPIC, payload, qos=0)
                if result.rc != mqtt.MQTT_ERR_SUCCESS:
                    print(f"MQTT publish failed with code {result.rc}", file=sys.stderr)
                else:
                    print(f"Published: {payload}")
    except KeyboardInterrupt:
        print("\nStopping serial-to-MQTT bridge.")
    finally:
        client.loop_stop()
        client.disconnect()


if __name__ == "__main__":
    main()
