import paho.mqtt.client as mqtt
import json
import sqlite3
import joblib
import pandas as pd # Added for feature names
from datetime import datetime

print("Loading machine learning model...")
model = joblib.load('activity_model.pkl')

DATABASE = 'activity.db'

def on_connect(client, userdata, flags, rc):
    print(f"Connected to MQTT Broker with result code {rc}")
    client.subscribe("activity/sensor_data")

def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode('utf-8')
        data = json.loads(payload)
        
        # Define features to match training exactly
        feature_names = ['acceleration_x', 'acceleration_y', 'acceleration_z', 'gyro_x', 'gyro_y', 'gyro_z']
        
        # Prepare data as a Pandas DataFrame to fix the warning
        features_df = pd.DataFrame([[
            data['accel_x'], data['accel_y'], data['accel_z'],
            data['gyro_x'], data['gyro_y'], data['gyro_z']
        ]], columns=feature_names)
        
        # Make prediction
        prediction_num = model.predict(features_df)[0]
        
        # Map the number to a string (Assuming 0=Walking, 1=Running based on Kaggle docs)
        if prediction_num == 0:
            activity_str = "Walking"
        elif prediction_num == 1:
            activity_str = "Running"
        else:
            activity_str = str(prediction_num) # Fallback

        probabilities = model.predict_proba(features_df)[0]
        confidence = max(probabilities)
        
        print(f"Prediction: {activity_str} (Confidence: {confidence:.2f})")
        
        # Save to Database
        now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        conn = sqlite3.connect(DATABASE, timeout=10)
        cursor = conn.cursor()
        cursor.execute(
            "INSERT INTO activities (activity, confidence, timestamp) VALUES (?, ?, ?)", 
            (activity_str, float(confidence), now) # Save the string now!
        )
        conn.commit()
        conn.close()
        
    except Exception as e:
        print(f"Error processing message: {e}")

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

print("Connecting to MQTT Broker...")
client.connect("127.0.0.1", 1883, 60)
client.loop_forever()