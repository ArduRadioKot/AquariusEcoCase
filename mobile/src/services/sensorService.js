import axios from 'axios';
import { API_ENDPOINTS } from '../config/api';

/**
 * Получение данных сенсора
 * @param {string} device - ID устройства (по умолчанию 'esp01')
 * @returns {Promise<Object>} Данные сенсора
 */
export const getSensorData = async (device = 'esp01') => {
  try {
    const url = `${API_ENDPOINTS.SENSOR_DATA}?device=${device}`;
    console.log('Fetching sensor data from:', url);
    
    const response = await axios.get(url, {
      timeout: 10000,
      headers: {
        'Accept': 'application/json',
      },
    });
    
    console.log('Sensor data response:', response.data);
    return response.data;
  } catch (error) {
    if (error.response) {
      // Сервер ответил с кодом ошибки
      console.error('Error response status:', error.response.status);
      console.error('Error response data:', error.response.data);
      console.error('Error response headers:', error.response.headers);
    } else if (error.request) {
      // Запрос был отправлен, но ответа не получено
      console.error('No response received:', error.request);
    } else {
      // Ошибка при настройке запроса
      console.error('Error setting up request:', error.message);
    }
    console.error('Full error:', error);
    throw error;
  }
};

/**
 * Преобразование данных сенсора в удобный формат
 * @param {Object} serverData - Данные с сервера
 * @returns {Array} Массив данных сенсоров
 */
export const formatSensorData = (serverData) => {
  console.log('Formatting sensor data:', serverData);
  
  if (!serverData) {
    console.warn('No server data provided');
    return [];
  }
  
  // Проверяем разные возможные форматы ответа
  let data = null;
  if (serverData.data) {
    // Формат: { timestamp: "...", data: { ... } }
    data = serverData.data;
  } else if (serverData.sensor_data) {
    // Альтернативный формат
    data = serverData.sensor_data;
  } else if (typeof serverData === 'object' && !serverData.timestamp) {
    // Прямой формат данных
    data = serverData;
  }
  
  if (!data) {
    console.warn('No sensor data found in response:', serverData);
    return [];
  }

  console.log('Processing sensor data:', data);
  const sensors = [];

  if (data.temperature) {
    sensors.push({
      id: 'temperature',
      name: 'Температура',
      value: parseFloat(data.temperature),
      unit: '°C',
      icon: 'thermometer',
      color: '#e74c3c',
    });
  }

  if (data.humidity) {
    sensors.push({
      id: 'humidity',
      name: 'Влажность',
      value: parseFloat(data.humidity),
      unit: '%',
      icon: 'water',
      color: '#3498db',
    });
  }

  if (data.co2) {
    sensors.push({
      id: 'co2',
      name: 'CO₂',
      value: parseInt(data.co2),
      unit: 'ppm',
      icon: 'cloud',
      color: '#9b59b6',
    });
  }

  if (data.pm25) {
    sensors.push({
      id: 'pm25',
      name: 'PM2.5',
      value: parseInt(data.pm25),
      unit: 'µg/m³',
      icon: 'wind',
      color: '#f39c12',
    });
  }

  if (data.pressure) {
    sensors.push({
      id: 'pressure',
      name: 'Давление',
      value: parseFloat(data.pressure),
      unit: 'hPa',
      icon: 'gauge',
      color: '#16a085',
    });
  }

  if (data.voc) {
    sensors.push({
      id: 'voc',
      name: 'ЛОС',
      value: parseInt(data.voc),
      unit: 'ppb',
      icon: 'flask',
      color: '#e67e22',
    });
  }

  if (data.ammonia) {
    sensors.push({
      id: 'ammonia',
      name: 'Амиак (NH₃)',
      value: parseFloat(data.ammonia),
      unit: 'ppm',
      icon: 'atom',
      color: '#27ae60',
    });
  }

  if (data.nox) {
    sensors.push({
      id: 'nox',
      name: 'Оксиды азота (NOₓ)',
      value: parseInt(data.nox),
      unit: 'ppb',
      icon: 'factory',
      color: '#c0392b',
    });
  }

  if (data.benzene) {
    sensors.push({
      id: 'benzene',
      name: 'Бензол (C₆H₆)',
      value: parseFloat(data.benzene),
      unit: 'ppb',
      icon: 'vial',
      color: '#8e44ad',
    });
  }

  if (data.uv_index) {
    sensors.push({
      id: 'uv_index',
      name: 'УФ-индекс',
      value: parseInt(data.uv_index),
      unit: '',
      icon: 'sun',
      color: '#f1c40f',
    });
  }

  return sensors;
};
