AOS.init({
    duration: 1000,
    once: true,
    offset: 100
  });

  let currentCharts = [];


  // Функция для получения текста статуса
  function getStatusText(status) {
    const statusTexts = {
      normal: "Норма",
      warning: "Выше нормы",
      danger: "Критично"
    };
    return statusTexts[status] || "Неизвестно";
  }

  // Функция для получения пути графика истории
  function getHistoryPath(trend) {
    const paths = {
      up: "M0,40 L20,35 L40,30 L60,25 L80,20 L100,15",
      down: "M0,15 L20,20 L40,25 L60,30 L80,35 L100,40",
      stable: "M0,30 L20,25 L40,30 L60,25 L80,30 L100,25"
    };
    return paths[trend] || paths.stable;
  }

  // Функция для получения иконки уведомления
  function getAlertIcon(type) {
    const icons = {
      danger: "exclamation-triangle",
      warning: "exclamation-circle",
      info: "info-circle"
    };
    return icons[type] || "info-circle";
  }

  // Функция для генерации данных графика на основе текущего значения
  function generateChartData(baseValue, variation) {
    return Array.from({length: 8}, (_, i) => {
      const randomVariation = (Math.random() - 0.5) * 2 * variation;
      return Math.max(0, baseValue + randomVariation);
    });
  }

  // Обработчик выбора местоположения
  document.querySelectorAll('.location-option').forEach(option => {
    option.addEventListener('click', function() {
      // Убираем активный класс у всех опций
      document.querySelectorAll('.location-option').forEach(opt => {
        opt.classList.remove('active');
      });
      
      // Добавляем активный класс к выбранной опции
      this.classList.add('active');
      
      // Показываем данные для выбранного местоположения
      const locationId = this.getAttribute('data-location');
      showLocationData(locationId);
    });
  });

  // Инициализация с данными ESP8266 по умолчанию
  document.addEventListener('DOMContentLoaded', () => {
    showLocationData('esp01');
  });

  // Функция для получения данных с сервера
  async function fetchSensorData(device = 'esp01') {
    try {
      const response = await fetch(`/api/sensor-data?device=${device}`);
      if (!response.ok) {
        throw new Error('Failed to fetch sensor data');
      }
      const data = await response.json();
      return data;
    } catch (error) {
      console.error('Error fetching sensor data:', error);
      return null;
    }
  }

  // Функция для преобразования данных с сервера в формат для отображения
  function convertServerDataToDisplay(serverData) {
    if (!serverData || !serverData.data) {
      return null;
    }

    const data = serverData.data;
    const sensorData = [];

    // Маппинг данных с сервера на формат для отображения
    if (data.temperature) {
      sensorData.push({
        type: 'temperature',
        name: 'Температура',
        value: parseFloat(data.temperature),
        unit: '°C',
        average: parseFloat(data.temperature) - 0.5 + Math.random(),
        trend: Math.random() > 0.5 ? 'up' : 'down',
        change: (Math.random() - 0.5) * 0.5,
        status: parseFloat(data.temperature) > 28 ? 'warning' : 'normal',
        icon: 'thermometer-half'
      });
    }

    if (data.humidity) {
      sensorData.push({
        type: 'humidity',
        name: 'Влажность',
        value: parseFloat(data.humidity),
        unit: '%',
        average: parseFloat(data.humidity) - 2 + Math.random() * 4,
        trend: Math.random() > 0.5 ? 'up' : 'down',
        change: (Math.random() - 0.5) * 2,
        status: parseFloat(data.humidity) < 30 || parseFloat(data.humidity) > 70 ? 'warning' : 'normal',
        icon: 'tint'
      });
    }

    if (data.co2) {
      sensorData.push({
        type: 'co2',
        name: 'CO₂',
        value: parseInt(data.co2),
        unit: 'ppm',
        average: parseInt(data.co2) - 50 + Math.random() * 100,
        trend: Math.random() > 0.5 ? 'up' : 'down',
        change: (Math.random() - 0.5) * 30,
        status: parseInt(data.co2) > 1000 ? 'danger' : parseInt(data.co2) > 800 ? 'warning' : 'normal',
        icon: 'smog'
      });
    }

    if (data.pm25) {
      sensorData.push({
        type: 'pm',
        name: 'PM2.5',
        value: parseInt(data.pm25),
        unit: 'µg/m³',
        average: parseInt(data.pm25) - 2 + Math.random() * 4,
        trend: Math.random() > 0.5 ? 'up' : 'down',
        change: (Math.random() - 0.5) * 2,
        status: parseInt(data.pm25) > 35 ? 'danger' : parseInt(data.pm25) > 25 ? 'warning' : 'normal',
        icon: 'wind'
      });
    }

    if (data.pressure) {
      sensorData.push({
        type: 'pressure',
        name: 'Давление',
        value: parseFloat(data.pressure),
        unit: 'hPa',
        average: parseFloat(data.pressure) - 2 + Math.random() * 4,
        trend: 'stable',
        change: 0,
        status: 'normal',
        icon: 'weight-hanging'
      });
    }

    if (data.voc) {
      sensorData.push({
        type: 'voc',
        name: 'ЛОС',
        value: parseInt(data.voc),
        unit: 'ppb',
        average: parseInt(data.voc) - 20 + Math.random() * 40,
        trend: Math.random() > 0.5 ? 'up' : 'down',
        change: (Math.random() - 0.5) * 15,
        status: parseInt(data.voc) > 400 ? 'danger' : parseInt(data.voc) > 300 ? 'warning' : 'normal',
        icon: 'flask'
      });
    }

    if (data.ammonia) {
      sensorData.push({
        type: 'ammonia',
        name: 'Амиак (NH₃)',
        value: parseFloat(data.ammonia),
        unit: 'ppm',
        average: parseFloat(data.ammonia) - 0.2 + Math.random() * 0.4,
        trend: Math.random() > 0.5 ? 'up' : 'down',
        change: (Math.random() - 0.5) * 0.1,
        status: parseFloat(data.ammonia) > 2 ? 'danger' : parseFloat(data.ammonia) > 1.5 ? 'warning' : 'normal',
        icon: 'atom'
      });
    }

    if (data.nox) {
      sensorData.push({
        type: 'nox',
        name: 'Оксиды азота (NOₓ)',
        value: parseInt(data.nox),
        unit: 'ppb',
        average: parseInt(data.nox) - 5 + Math.random() * 10,
        trend: Math.random() > 0.5 ? 'up' : 'down',
        change: (Math.random() - 0.5) * 5,
        status: parseInt(data.nox) > 80 ? 'danger' : parseInt(data.nox) > 60 ? 'warning' : 'normal',
        icon: 'industry'
      });
    }

    if (data.benzene) {
      sensorData.push({
        type: 'benzene',
        name: 'Бензол (C₆H₆)',
        value: parseFloat(data.benzene),
        unit: 'ppb',
        average: parseFloat(data.benzene) - 0.2 + Math.random() * 0.4,
        trend: Math.random() > 0.5 ? 'up' : 'down',
        change: (Math.random() - 0.5) * 0.1,
        status: parseFloat(data.benzene) > 3 ? 'danger' : parseFloat(data.benzene) > 2 ? 'warning' : 'normal',
        icon: 'vial'
      });
    }

    return {
      name: 'ESP8266 Sensor',
      address: 'Подключенный датчик',
      status: data.status || 'Online',
      sensors: sensorData.length,
      icon: 'wifi',
      sensorData: sensorData,
      alerts: generateAlerts(sensorData)
    };
  }

  // Генерация уведомлений на основе данных
  function generateAlerts(sensorData) {
    const alerts = [];
    sensorData.forEach(sensor => {
      if (sensor.status === 'danger') {
        alerts.push({
          type: 'danger',
          title: `Критичный уровень ${sensor.name}`,
          message: `Значение ${sensor.value}${sensor.unit} превышает допустимый предел`,
          time: new Date().toLocaleTimeString([], {hour: '2-digit', minute:'2-digit'})
        });
      } else if (sensor.status === 'warning') {
        alerts.push({
          type: 'warning',
          title: `Повышенный уровень ${sensor.name}`,
          message: `Значение ${sensor.value}${sensor.unit} приближается к верхней границе нормы`,
          time: new Date().toLocaleTimeString([], {hour: '2-digit', minute:'2-digit'})
        });
      }
    });
    if (alerts.length === 0) {
      alerts.push({
        type: 'info',
        title: 'Все показатели в норме',
        message: 'Все датчики показывают значения в пределах нормы',
        time: new Date().toLocaleTimeString([], {hour: '2-digit', minute:'2-digit'})
      });
    }
    return alerts;
  }

  // Функция для отображения данных
  async function showLocationData(locationId) {
    // Получаем данные с сервера
    const serverData = await fetchSensorData(locationId || 'esp01');
    if (serverData && serverData.data) {
      const displayData = convertServerDataToDisplay(serverData);
      if (displayData) {
        displaySensorData(displayData);
        return;
      }
    }
    // Если данных нет, показываем сообщение
    const sensorDataContainer = document.getElementById('sensorData');
    sensorDataContainer.innerHTML = `
      <div class="no-data">
        <i class="fas fa-exclamation-triangle"></i>
        <h3>Данные недоступны</h3>
        <p>Датчик не подключен или не передает данные</p>
      </div>
    `;
    
    // Обновляем заголовок
    const sensorHeader = document.querySelector('.sensor-header');
    if (sensorHeader) {
      sensorHeader.querySelector('.sensor-image i').className = 'fas fa-wifi';
      sensorHeader.querySelector('.sensor-title').textContent = 'ESP8266 Датчик';
      sensorHeader.querySelector('.sensor-location').textContent = 'Подключенный сенсор';
      const metaValues = sensorHeader.querySelectorAll('.meta-value');
      if (metaValues.length > 0) metaValues[0].textContent = 'Offline';
      if (metaValues.length > 1) metaValues[1].textContent = '--:--';
      if (metaValues.length > 2) metaValues[2].textContent = '0';
    }
    
    // Очищаем уведомления
    const alertsContainer = document.getElementById('alertsContainer');
    if (alertsContainer) {
      alertsContainer.innerHTML = `
        <div class="alert-item">
          <div class="alert-icon alert-warning">
            <i class="fas fa-exclamation-circle"></i>
          </div>
          <div class="alert-content">
            <h4>Датчик не подключен</h4>
            <p>Датчик не передает данные. Проверьте подключение.</p>
          </div>
          <div class="alert-time">--:--</div>
        </div>
      `;
    }
  }
    
  // Функция для отображения данных сенсоров
  function displaySensorData(location) {
    // Обновляем статус в выборе местоположения
    const statusElement = document.getElementById('sensorStatus');
    if (statusElement) {
      statusElement.textContent = location.status;
      statusElement.className = location.status === 'Online' ? 'location-status' : 'location-status status-offline';
    }
    
    // Обновляем заголовок с информацией о датчике
    const sensorHeader = document.querySelector('.sensor-header');
    if (sensorHeader) {
      sensorHeader.querySelector('.sensor-image i').className = `fas fa-${location.icon}`;
      sensorHeader.querySelector('.sensor-title').textContent = location.name;
      sensorHeader.querySelector('.sensor-location').textContent = location.address;
      const metaValues = sensorHeader.querySelectorAll('.meta-value');
      if (metaValues.length > 0) metaValues[0].textContent = location.status;
      if (metaValues.length > 1) metaValues[1].textContent = new Date().toLocaleTimeString([], {hour: '2-digit', minute:'2-digit'});
      if (metaValues.length > 2) metaValues[2].textContent = location.sensors;
    }
    
    // Обновляем данные датчиков
    const sensorDataContainer = document.getElementById('sensorData');
    
    if (!location.sensorData || location.sensorData.length === 0) {
      sensorDataContainer.innerHTML = `
        <div class="no-data">
          <i class="fas fa-exclamation-triangle"></i>
          <h3>Данные недоступны</h3>
          <p>Датчик в настоящее время offline и не передает данные</p>
        </div>
      `;
    } else {
      sensorDataContainer.innerHTML = location.sensorData.map(sensor => `
        <div class="sensor-card">
          <div class="sensor-card-header">
            <div class="sensor-card-icon">
              <i class="fas fa-${sensor.icon}"></i>
            </div>
            <div class="sensor-card-status status-${sensor.status}">${getStatusText(sensor.status)}</div>
          </div>
          <div class="sensor-card-title">${sensor.name}</div>
          <div class="sensor-card-value">${sensor.value}<span class="sensor-card-unit">${sensor.unit}</span></div>
          <div class="sensor-card-average">Среднее за день: ${sensor.average.toFixed(1)}${sensor.unit}</div>
          <div class="sensor-card-details">
            <div class="sensor-card-trend trend-${sensor.trend}">
              <i class="fas fa-arrow-${sensor.trend === 'stable' ? 'minus' : sensor.trend}"></i>
              <span>${sensor.trend === 'up' ? '+' : ''}${sensor.change.toFixed(1)}${sensor.unit}</span>
            </div>
            <div>Обновлено: ${new Date().toLocaleTimeString([], {hour: '2-digit', minute:'2-digit'})}</div>
          </div>
          <div class="sensor-card-history">
            <svg class="history-line" viewBox="0 0 100 40" preserveAspectRatio="none">
              <path d="${getHistoryPath(sensor.trend)}"></path>
            </svg>
          </div>
        </div>
      `).join('');
    }
    
    // Обновляем уведомления
    const alertsContainer = document.getElementById('alertsContainer');
    if (alertsContainer && location.alerts) {
    alertsContainer.innerHTML = location.alerts.map(alert => `
      <div class="alert-item">
        <div class="alert-icon alert-${alert.type}">
          <i class="fas fa-${getAlertIcon(alert.type)}"></i>
        </div>
        <div class="alert-content">
          <h4>${alert.title}</h4>
          <p>${alert.message}</p>
        </div>
        <div class="alert-time">${alert.time}</div>
      </div>
    `).join('');
    }
    
    // Обновляем графики
    updateCharts(location);
  }

  // Обновленная функция для обновления графиков
  function updateCharts(location) {
    // Уничтожаем предыдущие графики
    currentCharts.forEach(chart => chart.destroy());
    currentCharts = [];
    
    if (!location.sensorData || location.sensorData.length === 0) return;
    
    // График температуры и влажности
    const tempSensor = location.sensorData.find(s => s.type === 'temperature');
    const humiditySensor = location.sensorData.find(s => s.type === 'humidity');
    
    if (tempSensor || humiditySensor) {
    const tempHumidityCtx = document.getElementById('tempHumidityChart').getContext('2d');
    const tempHumidityChart = new Chart(tempHumidityCtx, {
      type: 'line',
      data: {
        labels: ['00:00', '03:00', '06:00', '09:00', '12:00', '15:00', '18:00', '21:00'],
        datasets: [
          {
            label: 'Температура (°C)',
              data: tempSensor ? generateChartData(tempSensor.value, 2) : [],
            borderColor: '#e74c3c',
            backgroundColor: 'rgba(231, 76, 60, 0.1)',
            tension: 0.4,
            yAxisID: 'y'
          },
          {
            label: 'Влажность (%)',
              data: humiditySensor ? generateChartData(humiditySensor.value, 5) : [],
            borderColor: '#3498db',
            backgroundColor: 'rgba(52, 152, 219, 0.1)',
            tension: 0.4,
            yAxisID: 'y1'
          }
        ]
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        scales: {
          y: {
            type: 'linear',
            display: true,
            position: 'left',
            title: {
              display: true,
              text: 'Температура (°C)'
            }
          },
          y1: {
            type: 'linear',
            display: true,
            position: 'right',
            title: {
              display: true,
              text: 'Влажность (%)'
            },
            grid: {
              drawOnChartArea: false
            }
          }
        }
      }
    });
    currentCharts.push(tempHumidityChart);
    }
    
    // График качества воздуха
    const co2Sensor = location.sensorData.find(s => s.type === 'co2');
    const pmSensor = location.sensorData.find(s => s.type === 'pm');
    
    if (co2Sensor || pmSensor) {
    const airQualityCtx = document.getElementById('airQualityChart').getContext('2d');
    const airQualityChart = new Chart(airQualityCtx, {
      type: 'line',
      data: {
        labels: ['00:00', '03:00', '06:00', '09:00', '12:00', '15:00', '18:00', '21:00'],
        datasets: [
          {
            label: 'CO₂ (ppm)',
              data: co2Sensor ? generateChartData(co2Sensor.value, 50) : [],
            borderColor: '#9b59b6',
            backgroundColor: 'rgba(155, 89, 182, 0.1)',
            tension: 0.4,
            yAxisID: 'y'
          },
          {
            label: 'PM2.5 (µg/m³)',
              data: pmSensor ? generateChartData(pmSensor.value, 3) : [],
            borderColor: '#f39c12',
            backgroundColor: 'rgba(243, 156, 18, 0.1)',
            tension: 0.4,
            yAxisID: 'y1'
          }
        ]
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        scales: {
          y: {
            type: 'linear',
            display: true,
            position: 'left',
            title: {
              display: true,
              text: 'CO₂ (ppm)'
            }
          },
          y1: {
            type: 'linear',
            display: true,
            position: 'right',
            title: {
              display: true,
              text: 'PM2.5 (µg/m³)'
            },
            grid: {
              drawOnChartArea: false
            }
          }
        }
      }
    });
    currentCharts.push(airQualityChart);
  }
  }

  // Имитация обновления данных
  async function updateSensorData() {
    const activeLocation = document.querySelector('.location-option.active');
    if (activeLocation) {
      const locationId = activeLocation.getAttribute('data-location');
      await showLocationData(locationId);
    }
  }

  // Обновление данных каждые 5 секунд
  setInterval(updateSensorData, 5000);