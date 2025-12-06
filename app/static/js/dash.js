AOS.init({
    duration: 1000,
    once: true,
    offset: 100
  });

  // Регистрация плагинов Chart.js
  Chart.register(ChartZoom);
  
  let currentCharts = [];
  let currentSensorData = null;
  let chart1Selection = null;
  let chart2Selection = null;
  
  // Маппинг типов сенсоров на их свойства
  const sensorConfig = {
    temperature: { label: 'Температура', unit: '°C', color: '#e74c3c', yAxis: 'y' },
    humidity: { label: 'Влажность', unit: '%', color: '#3498db', yAxis: 'y1' },
    co2: { label: 'CO₂', unit: 'ppm', color: '#9b59b6', yAxis: 'y' },
    pm: { label: 'PM2.5', unit: 'µg/m³', color: '#f39c12', yAxis: 'y1' },
    pressure: { label: 'Давление', unit: 'hPa', color: '#16a085', yAxis: 'y' },
    voc: { label: 'ЛОС', unit: 'ppb', color: '#e67e22', yAxis: 'y1' }
  };


  // Функция для получения пути графика истории
  function getHistoryPath(trend) {
    const paths = {
      up: "M0,40 L20,35 L40,30 L60,25 L80,20 L100,15",
      down: "M0,15 L20,20 L40,25 L60,30 L80,35 L100,40",
      stable: "M0,30 L20,25 L40,30 L60,25 L80,30 L100,25"
    };
    return paths[trend] || paths.stable;
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
      sensorData: sensorData
    };
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
    
    // Обновляем графики
    updateCharts(location);
  }

  // Функция для создания графика с интерактивностью
  function createInteractiveChart(canvasId, sensor1Type, sensor2Type, location, chartIndex) {
    const ctx = document.getElementById(canvasId).getContext('2d');
    const sensor1 = location.sensorData.find(s => s.type === sensor1Type);
    const sensor2 = location.sensorData.find(s => s.type === sensor2Type);
    
    if (!sensor1 && !sensor2) return null;
    
    const config1 = sensorConfig[sensor1Type];
    const config2 = sensorConfig[sensor2Type];
    
    const labels = ['00:00', '03:00', '06:00', '09:00', '12:00', '15:00', '18:00', '21:00'];
    const data1 = sensor1 ? generateChartData(sensor1.value, getVariation(sensor1Type)) : [];
    const data2 = sensor2 ? generateChartData(sensor2.value, getVariation(sensor2Type)) : [];
    
    const datasets = [];
    if (sensor1 && config1) {
      const bgColor = hexToRgba(config1.color, 0.1);
      datasets.push({
        label: `${config1.label} (${config1.unit})`,
        data: data1,
        borderColor: config1.color,
        backgroundColor: bgColor,
        tension: 0.4,
        yAxisID: config1.yAxis,
        pointRadius: 3,
        pointHoverRadius: 5
      });
    }
    if (sensor2 && config2) {
      const bgColor = hexToRgba(config2.color, 0.1);
      datasets.push({
        label: `${config2.label} (${config2.unit})`,
        data: data2,
        borderColor: config2.color,
        backgroundColor: bgColor,
        tension: 0.4,
        yAxisID: config2.yAxis,
        pointRadius: 3,
        pointHoverRadius: 5
      });
    }
    
    const chart = new Chart(ctx, {
      type: 'line',
      data: {
        labels: labels,
        datasets: datasets
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        interaction: {
          mode: 'index',
          intersect: false,
        },
        plugins: {
          zoom: {
            pan: {
              enabled: true,
              mode: 'x',
            },
            zoom: {
              wheel: {
                enabled: true,
              },
              pinch: {
                enabled: true
              },
              mode: 'x',
              onZoomComplete: (ctx) => {
                updateChartStats(canvasId, chartIndex);
              }
            }
          },
          tooltip: {
            enabled: true
            }
          },
        scales: {
          x: {
            type: 'category',
            display: true,
            title: {
              display: true,
              text: 'Время'
            }
          },
          y: {
            type: 'linear',
            display: true,
            position: 'left',
            title: {
              display: true,
              text: sensor1 && config1 ? `${config1.label} (${config1.unit})` : ''
            }
          },
          y1: {
            type: 'linear',
            display: true,
            position: 'right',
            title: {
              display: true,
              text: sensor2 && config2 ? `${config2.label} (${config2.unit})` : ''
            },
            grid: {
              drawOnChartArea: false
            }
          }
        },
        onHover: (event, activeElements) => {
          event.native.target.style.cursor = activeElements.length > 0 ? 'pointer' : 'default';
        }
      }
    });
    
    // Добавляем обработчик для выделения области
    let isSelecting = false;
    let startX = null;
    let selectionBox = null;
    
    const canvas = ctx.canvas;
    const container = canvas.parentElement;
    
    function handleMouseDown(e) {
      const rect = canvas.getBoundingClientRect();
      const x = e.clientX - rect.left;
      const y = e.clientY - rect.top;
      
      const chartArea = chart.chartArea;
      if (x >= chartArea.left && x <= chartArea.right && y >= chartArea.top && y <= chartArea.bottom) {
        isSelecting = true;
        startX = x;
        
        // Создаем элемент для выделения
        selectionBox = document.createElement('div');
        selectionBox.style.position = 'absolute';
        selectionBox.style.border = '2px dashed ' + (chartIndex === 0 ? '#e74c3c' : '#9b59b6');
        selectionBox.style.backgroundColor = (chartIndex === 0 ? 'rgba(231, 76, 60, 0.1)' : 'rgba(155, 89, 182, 0.1)');
        selectionBox.style.pointerEvents = 'none';
        selectionBox.style.zIndex = '1000';
        const containerRect = container.getBoundingClientRect();
        selectionBox.style.left = (chartArea.left) + 'px';
        selectionBox.style.top = (chartArea.top) + 'px';
        selectionBox.style.width = '0px';
        selectionBox.style.height = (chartArea.bottom - chartArea.top) + 'px';
        container.style.position = 'relative';
        container.appendChild(selectionBox);
      }
    }
    
    function handleMouseMove(e) {
      if (isSelecting && startX !== null && selectionBox) {
        const rect = canvas.getBoundingClientRect();
        const x = e.clientX - rect.left;
        const chartArea = chart.chartArea;
        
        const left = Math.min(startX, x);
        const width = Math.abs(x - startX);
        
        selectionBox.style.left = (left) + 'px';
        selectionBox.style.width = width + 'px';
      }
    }
    
    function handleMouseUp(e) {
      if (isSelecting && startX !== null) {
        const rect = canvas.getBoundingClientRect();
        const x = e.clientX - rect.left;
        const chartArea = chart.chartArea;
        
        const startIndex = Math.round(((startX - chartArea.left) / (chartArea.right - chartArea.left)) * labels.length);
        const endIndex = Math.round(((x - chartArea.left) / (chartArea.right - chartArea.left)) * labels.length);
        
        const minIndex = Math.max(0, Math.min(startIndex, endIndex));
        const maxIndex = Math.min(labels.length - 1, Math.max(startIndex, endIndex));
        
        if (minIndex !== maxIndex && minIndex >= 0 && maxIndex < labels.length) {
          calculateAndDisplayStats(chart, minIndex, maxIndex, chartIndex);
        }
        
        if (selectionBox) {
          selectionBox.remove();
          selectionBox = null;
        }
        
        isSelecting = false;
        startX = null;
      }
    }
    
    canvas.addEventListener('mousedown', handleMouseDown);
    document.addEventListener('mousemove', handleMouseMove);
    document.addEventListener('mouseup', handleMouseUp);
    
    return chart;
  }
  
  // Функция для получения вариации данных
  function getVariation(sensorType) {
    const variations = {
      temperature: 2,
      humidity: 5,
      co2: 50,
      pm: 3,
      pressure: 2,
      voc: 20
    };
    return variations[sensorType] || 5;
  }
  
  // Функция для конвертации hex в rgba
  function hexToRgba(hex, alpha) {
    const r = parseInt(hex.slice(1, 3), 16);
    const g = parseInt(hex.slice(3, 5), 16);
    const b = parseInt(hex.slice(5, 7), 16);
    return `rgba(${r}, ${g}, ${b}, ${alpha})`;
  }
  
  // Функция для расчета и отображения статистики
  function calculateAndDisplayStats(chart, startIndex, endIndex, chartIndex) {
    const datasets = chart.data.datasets;
    const statsContainer = document.getElementById(`chart${chartIndex + 1}Stats`);
    
    if (!statsContainer || datasets.length === 0) return;
    
    statsContainer.style.display = 'flex';
    
    // Вычисляем статистику для первого датасета
    const dataset = datasets[0];
    const selectedData = dataset.data.slice(startIndex, endIndex + 1);
    if (selectedData.length === 0) return;
    
    const values = selectedData.map(v => typeof v === 'object' ? v.y : v).filter(v => !isNaN(v));
    if (values.length === 0) return;
    
    const max = Math.max(...values);
    const min = Math.min(...values);
    const avg = values.reduce((a, b) => a + b, 0) / values.length;
    
    const unit = dataset.label.match(/\(([^)]+)\)/)?.[1] || '';
    const decimals = unit === '°C' || unit === 'hPa' ? 1 : unit === '%' ? 1 : 0;
    
    document.getElementById(`chart${chartIndex + 1}Max`).textContent = `${max.toFixed(decimals)} ${unit}`;
    document.getElementById(`chart${chartIndex + 1}Min`).textContent = `${min.toFixed(decimals)} ${unit}`;
    document.getElementById(`chart${chartIndex + 1}Avg`).textContent = `${avg.toFixed(decimals)} ${unit}`;
  }
  
  // Функция для обновления статистики при зуме
  function updateChartStats(canvasId, chartIndex) {
    // Можно добавить логику обновления статистики при зуме
  }
  
  // Функция для сброса масштаба
  function resetZoom(canvasId) {
    const chart = currentCharts.find(c => c.canvas.id === canvasId);
    if (chart) {
      chart.resetZoom();
      const chartIndex = canvasId === 'tempHumidityChart' ? 0 : 1;
      const statsContainer = document.getElementById(`chart${chartIndex + 1}Stats`);
      if (statsContainer) {
        statsContainer.style.display = 'none';
      }
    }
  }
  
  // Функция для скачивания графика
  function downloadChart(canvasId) {
    const chart = currentCharts.find(c => c.canvas.id === canvasId);
    if (chart) {
      const url = chart.toBase64Image();
      const link = document.createElement('a');
      link.download = `chart-${canvasId}-${Date.now()}.png`;
      link.href = url;
      link.click();
    }
  }
  
  // Обновленная функция для обновления графиков
  function updateCharts(location) {
    // Сохраняем данные для использования в селекторах
    currentSensorData = location;
    
    // Уничтожаем предыдущие графики
    currentCharts.forEach(chart => chart.destroy());
    currentCharts = [];
    
    if (!location.sensorData || location.sensorData.length === 0) return;
    
    // Получаем выбранные сенсоры из селекторов
    const chart1Sensor1 = document.getElementById('chart1Sensor1').value;
    const chart1Sensor2 = document.getElementById('chart1Sensor2').value;
    const chart2Sensor1 = document.getElementById('chart2Sensor1').value;
    const chart2Sensor2 = document.getElementById('chart2Sensor2').value;
    
    // Создаем графики
    const chart1 = createInteractiveChart('tempHumidityChart', chart1Sensor1, chart1Sensor2, location, 0);
    if (chart1) currentCharts.push(chart1);
    
    const chart2 = createInteractiveChart('airQualityChart', chart2Sensor1, chart2Sensor2, location, 1);
    if (chart2) currentCharts.push(chart2);
  }
  
  // Обработчики для селекторов графиков
  function setupChartSelectors() {
    const chart1Sensor1 = document.getElementById('chart1Sensor1');
    const chart1Sensor2 = document.getElementById('chart1Sensor2');
    const chart2Sensor1 = document.getElementById('chart2Sensor1');
    const chart2Sensor2 = document.getElementById('chart2Sensor2');
    
    function updateChart1() {
      // Проверяем, что не выбраны одинаковые сенсоры
      if (chart1Sensor1.value === chart1Sensor2.value) {
        // Если совпадают, меняем второй на другой
        const options = Array.from(chart1Sensor2.options);
        const otherOption = options.find(opt => opt.value !== chart1Sensor1.value);
        if (otherOption) chart1Sensor2.value = otherOption.value;
      }
      if (currentSensorData) {
        updateCharts(currentSensorData);
      }
    }
    
    function updateChart2() {
      // Проверяем, что не выбраны одинаковые сенсоры
      if (chart2Sensor1.value === chart2Sensor2.value) {
        // Если совпадают, меняем второй на другой
        const options = Array.from(chart2Sensor2.options);
        const otherOption = options.find(opt => opt.value !== chart2Sensor1.value);
        if (otherOption) chart2Sensor2.value = otherOption.value;
      }
      if (currentSensorData) {
        updateCharts(currentSensorData);
      }
    }
    
    if (chart1Sensor1) chart1Sensor1.addEventListener('change', updateChart1);
    if (chart1Sensor2) chart1Sensor2.addEventListener('change', updateChart1);
    if (chart2Sensor1) chart2Sensor1.addEventListener('change', updateChart2);
    if (chart2Sensor2) chart2Sensor2.addEventListener('change', updateChart2);
  }
  
  // Инициализация при загрузке
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', setupChartSelectors);
  } else {
    setupChartSelectors();
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