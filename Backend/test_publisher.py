import paho.mqtt.client as mqtt
import json
import time

client = mqtt.Client()
client.connect("127.0.0.1", 1883, 60)

# Simulate a walking activity
payload = {
    "accel_x": 0.05, 
    "accel_y": -0.02, 
    "accel_z": 9.81, 
    "gyro_x": 0.01, 
    "gyro_y": 0.0, 
    "gyro_z": 0.05
}

json_data = json.dumps(payload)
print(f"Sending data: {json_data}")
client.publish("activity/sensor_data", json_data)

time.sleep(1)
client.disconnect()
print("Data sent successfully!")
