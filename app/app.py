from flask import Flask, render_template, request, redirect, url_for, flash, session, jsonify
from werkzeug.security import generate_password_hash, check_password_hash
import sqlite3
import os
import datetime
import threading
import json
from collections import deque

# Путь к базе данных
DB_PATH = 'users.db'

def init_db():
    """Создаёт таблицу users, если её нет"""
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL UNIQUE,
            email TEXT NOT NULL UNIQUE,
            password_hash TEXT NOT NULL
        )
    ''')
    conn.commit()
    conn.close()

app = Flask(__name__)
app.secret_key = 'dev-secret-key-change-in-production'

# Инициализируем базу данных при старте
init_db()

# Хранилище данных сенсоров
sensor_data_history = deque(maxlen=1000)  # последние 1000 записей

class SensorDataStore:
    def __init__(self):
        self.sensor_data = {}
        self.lock = threading.Lock()
    
    def add_sensor_data(self, device, sensor_data):
        with self.lock:
            timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            record = {
                'device': device,
                'timestamp': timestamp,
                'sensor_data': sensor_data
            }
            sensor_data_history.append(record)
            # Сохраняем последние данные для каждого устройства
            self.sensor_data[device] = {
                'timestamp': timestamp,
                'data': sensor_data
            }
    
    def get_latest_data(self, device=None):
        with self.lock:
            if device:
                return self.sensor_data.get(device, None)
            return self.sensor_data
    
    def get_all_data(self):
        with self.lock:
            return list(sensor_data_history)

sensor_store = SensorDataStore()

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/login', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        # Если пользователь уже авторизован, перенаправляем на dashboard
        if 'user_id' in session:
            return redirect(url_for('dashboard'))
        email = request.form.get('email', '').strip()
        password = request.form.get('password', '')

        if not email or not password:
            flash('Заполните все поля', 'error')
            return render_template('login.html')

        try:
            conn = sqlite3.connect(DB_PATH)
            conn.row_factory = sqlite3.Row
            cursor = conn.cursor()
            cursor.execute("SELECT * FROM users WHERE email = ?", (email,))
            user = cursor.fetchone()
            conn.close()

            if user and check_password_hash(user['password_hash'], password):
                session['user_id'] = user['id']
                session['username'] = user['username']
                flash('Вы успешно вошли!', 'success')
                return redirect(url_for('dashboard'))
            else:
                flash('Неверный email или пароль', 'error')
        except Exception as e:
            flash('Произошла ошибка при входе. Попробуйте позже.', 'error')
    
    return render_template('login.html')

@app.route('/register', methods=['GET', 'POST'])
def register():
    if request.method == 'POST':
        username = request.form.get('username', '').strip()
        email = request.form.get('email', '').strip()
        password = request.form.get('password', '')
        confirm = request.form.get('confirm_password', '')

        # Валидация
        if not username or len(username) < 3:
            flash('Имя пользователя должно содержать минимум 3 символа', 'error')
            return render_template('register.html')
        
        if not email or '@' not in email:
            flash('Введите корректный email', 'error')
            return render_template('register.html')
        
        if not password or len(password) < 6:
            flash('Пароль должен содержать минимум 6 символов', 'error')
            return render_template('register.html')

        if password != confirm:
            flash('Пароли не совпадают', 'error')
            return render_template('register.html')

        try:
            conn = sqlite3.connect(DB_PATH)
            cursor = conn.cursor()
            password_hash = generate_password_hash(password)
            cursor.execute(
                "INSERT INTO users (username, email, password_hash) VALUES (?, ?, ?)",
                (username, email, password_hash)
            )
            user_id = cursor.lastrowid
            conn.commit()
            conn.close()

            # Автоматически логиним пользователя после регистрации
            session['user_id'] = user_id
            session['username'] = username
            flash('Регистрация успешна! Добро пожаловать!', 'success')
            return redirect(url_for('dashboard'))

        except sqlite3.IntegrityError:
            conn.close()
            flash('Пользователь с таким email или именем уже существует', 'error')
            return render_template('register.html')
        except Exception as e:
            conn.close()
            flash('Произошла ошибка при регистрации. Попробуйте позже.', 'error')
            return render_template('register.html')

    return render_template('register.html')

@app.route('/dashboard')
def dashboard():
    # Проверка авторизации
    if 'user_id' not in session:
        flash('Пожалуйста, войдите в систему', 'error')
        return redirect(url_for('login'))
    return render_template('dashboard.html')

@app.route('/logout')
def logout():
    session.clear()
    flash('Вы вышли из аккаунта', 'info')
    return redirect(url_for('index'))

# API для приема данных от ESP8266
@app.route('/data', methods=['POST'])
def receive_data():
    try:
        device = request.form.get('device', 'unknown')
        # Получаем все данные сенсоров из формы
        sensor_data = {
            'temperature': request.form.get('temperature'),
            'humidity': request.form.get('humidity'),
            'co2': request.form.get('co2'),
            'pm25': request.form.get('pm25'),
            'pm10': request.form.get('pm10'),
            'pressure': request.form.get('pressure'),
            'voc': request.form.get('voc'),
            'ammonia': request.form.get('ammonia'),
            'nox': request.form.get('nox'),
            'benzene': request.form.get('benzene'),
            'uv_index': request.form.get('uv_index'),
            'status': request.form.get('status', 'online'),
            'free_memory': request.form.get('free_memory', '')
        }
        
        # Удаляем None значения
        sensor_data = {k: v for k, v in sensor_data.items() if v is not None}
        
        sensor_store.add_sensor_data(device, sensor_data)
        
        print(f"Received sensor data from {device}: {sensor_data}")
        return jsonify({'status': 'success'}), 200
    
    except Exception as e:
        print(f"Error processing sensor data: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 400

# API для получения данных сенсоров
@app.route('/api/sensor-data')
def get_sensor_data():
    # Проверка авторизации
    if 'user_id' not in session:
        return jsonify({'error': 'Unauthorized'}), 401
    
    device = request.args.get('device', None)
    latest_data = sensor_store.get_latest_data(device)
    
    if device and latest_data:
        return jsonify(latest_data)
    elif device:
        return jsonify({'error': 'Device not found'}), 404
    else:
        # Возвращаем все данные
        return jsonify({
            'devices': sensor_store.get_latest_data(),
            'history': sensor_store.get_all_data()[-100:]  # последние 100 записей
        })

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5002, debug=True)