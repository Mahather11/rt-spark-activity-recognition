from flask import Flask, jsonify
import sqlite3
from datetime import datetime

app = Flask(__name__)
DATABASE = 'activity.db'

def enable_wal():
    conn = sqlite3.connect(DATABASE, timeout=10)
    conn.execute('PRAGMA journal_mode=WAL;')
    conn.close()

# 1. Initialize the database
def init_db():
    conn = sqlite3.connect(DATABASE, timeout=10)
    cursor = conn.cursor()
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS activities (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            activity TEXT NOT NULL,
            confidence REAL NOT NULL,
            timestamp TEXT NOT NULL
        )
    ''')
    conn.commit()
    conn.close()

# 2. API Endpoint for health checks
@app.route('/api/ping', methods=['GET'])
def ping():
    return jsonify({"status": "ok", "message": "Flask is alive!"})

# 3. API Endpoint for health checks under the activities base URL
@app.route('/api/activities/ping', methods=['GET'])
def activities_ping():
    return jsonify({"status": "ok", "message": "Flask is alive!"})

# 4. API Endpoint for the LATEST prediction
@app.route('/api/activities/latest', methods=['GET'])
def get_latest_activity():
    print("Request received for latest activity!")
    conn = sqlite3.connect(DATABASE, timeout=10)
    cursor = conn.cursor()
    cursor.execute("SELECT activity, confidence, timestamp FROM activities ORDER BY id DESC LIMIT 1")
    row = cursor.fetchone()
    conn.close()

    if row:
        response = {"activity": row[0], "confidence": row[1], "timestamp": row[2]}
    else:
        response = {"activity": "Waiting...", "confidence": 0.0, "timestamp": ""}

    print("Returning data to app!")
    return jsonify(response)

# 5. API Endpoint for HISTORY
@app.route('/api/activities/history', methods=['GET'])
def get_history():
    conn = sqlite3.connect(DATABASE, timeout=10) # <-- Added timeout
    cursor = conn.cursor()
    cursor.execute("SELECT activity, confidence, timestamp FROM activities ORDER BY id DESC LIMIT 20")
    rows = cursor.fetchall()
    conn.close()

    history = []
    for row in rows:
        history.append({"activity": row[0], "confidence": row[1], "timestamp": row[2]})
    return jsonify(history)

# 6. A temporary route to simulate the device sending data
@app.route('/api/test_insert', methods=['GET'])
def test_insert():
    conn = sqlite3.connect(DATABASE, timeout=10) # <-- Added timeout
    cursor = conn.cursor()
    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    cursor.execute("INSERT INTO activities (activity, confidence, timestamp) VALUES (?, ?, ?)", ("Walking", 0.95, now))
    conn.commit()
    conn.close()
    return "Inserted a test record!"

if __name__ == '__main__':
    init_db()
    enable_wal()
    print("Database ready. Starting server...")
    app.run(host='0.0.0.0', port=5000, debug=True, threaded=True)