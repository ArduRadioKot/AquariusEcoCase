from flask import Flask, request, render_template, jsonify
import datetime
import threading
import json
from collections import deque

app = Flask(__name__)

# Хранилище данных
data_history = deque(maxlen=100)  # последние 100 записей
serial_data_history = deque(maxlen=50)  # последние 50 serial данных

class DataStore:
    def __init__(self):
        self.data = {}
        self.serial_data = []
        self.lock = threading.Lock()
    
    def add_data(self, device, data):
        with self.lock:
            timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            record = {
                'device': device,
                'timestamp': timestamp,
                'data': data
            }
            data_history.append(record)
    
    def add_serial_data(self, device, serial_data):
        with self.lock:
            timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            record = {
                'device': device,
                'timestamp': timestamp,
                'serial_data': serial_data
            }
            serial_data_history.append(record)
    
    def get_all_data(self):
        with self.lock:
            return list(data_history)
    
    def get_all_serial_data(self):
        with self.lock:
            return list(serial_data_history)

data_store = DataStore()

@app.route('/')
def index():
    return render_template('index2.html')

@app.route('/data', methods=['POST'])
def receive_data():
    try:
        device = request.form.get('device', 'unknown')
        status = request.form.get('status', '')
        free_memory = request.form.get('free_memory', '')
        
        data = {
            'status': status,
            'free_memory': free_memory
        }
        
        data_store.add_data(device, data)
        
        print(f"Received data from {device}: {data}")
        return jsonify({'status': 'success'}), 200
    
    except Exception as e:
        print(f"Error processing data: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 400

@app.route('/serial', methods=['POST'])
def receive_serial():
    try:
        device = request.form.get('device', 'unknown')
        serial_data = request.form.get('serial_data', '')
        
        data_store.add_serial_data(device, serial_data)
        
        print(f"Received serial data from {device}: {serial_data}")
        return jsonify({'status': 'success'}), 200
    
    except Exception as e:
        print(f"Error processing serial data: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 400

@app.route('/api/data')
def get_data():
    return jsonify({
        'device_data': data_store.get_all_data(),
        'serial_data': data_store.get_all_serial_data()
    })

@app.route('/api/clear')
def clear_data():
    global data_history, serial_data_history
    data_history.clear()
    serial_data_history.clear()
    return jsonify({'status': 'cleared'})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5002, debug=True)