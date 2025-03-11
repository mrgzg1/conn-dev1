// Helper function to get nested object values using dot notation
const getNestedValue = (obj, path) => {
  if (!obj) return null;
  return path.split('.').reduce((prev, curr) => {
    return prev ? prev[curr] : null;
  }, obj);
};

// Custom Chart component for configured views
window.CustomSensorChart = ({ view, sensorData, sensorHistory, title }) => {
  const chartRef = React.useRef(null);
  const chartInstance = React.useRef(null);

  const updateChart = () => {
    if (!chartRef.current) return;
    
    // Destroy existing chart
    if (chartInstance.current) {
      chartInstance.current.destroy();
    }
    
    // Check if we have data for any of the metrics
    const hasData = view.metrics.some(metric => 
      sensorHistory[metric.sensorType] && sensorHistory[metric.sensorType].length > 0
    );
    
    if (!hasData) return;
    
    // Extract timestamps from the first available sensor data
    let timestamps = [];
    for (const metric of view.metrics) {
      if (sensorHistory[metric.sensorType] && sensorHistory[metric.sensorType].length > 0) {
        timestamps = sensorHistory[metric.sensorType].map(entry => {
          const date = new Date(entry.timestamp);
          return date.toLocaleTimeString();
        });
        break;
      }
    }
    
    // Create datasets for each metric
    const datasets = view.metrics.map(metric => {
      const history = sensorHistory[metric.sensorType] || [];
      const data = history.map(entry => {
        let value = getNestedValue(entry, metric.dataKey);
        return metric.transform ? metric.transform(value) : value;
      });
      
      return {
        label: metric.label,
        data: data,
        borderColor: metric.color,
        backgroundColor: metric.color.replace('1)', '0.1)'),
        borderWidth: 2,
        fill: false,
        tension: 0.2 // Adds smoother curves
      };
    });
    
    const ctx = chartRef.current.getContext('2d');
    
    chartInstance.current = new Chart(ctx, {
      type: 'line',
      data: {
        labels: timestamps,
        datasets: datasets
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        scales: {
          x: {
            title: {
              display: true,
              text: 'Time'
            }
          },
          y: {
            title: {
              display: true,
              text: 'Value'
            },
            beginAtZero: false
          }
        },
        plugins: {
          title: {
            display: true,
            text: title || view.title
          },
          legend: {
            position: 'top',
          }
        }
      }
    });
  };

  React.useEffect(() => {
    updateChart();
    return () => {
      if (chartInstance.current) {
        chartInstance.current.destroy();
      }
    };
  }, [view, sensorHistory, title]);

  return (
    <div style={{ width: '100%', height: '100%' }}>
      {view.metrics.some(metric => 
        sensorHistory[metric.sensorType] && sensorHistory[metric.sensorType].length > 0
      ) ? (
        <canvas ref={chartRef} />
      ) : (
        <div style={{ 
          height: '100%', 
          display: 'flex', 
          alignItems: 'center', 
          justifyContent: 'center',
          color: '#666'
        }}>
          No historical data available yet
        </div>
      )}
    </div>
  );
};

// Original SensorHistoryChart for backward compatibility
window.SensorHistoryChart = ({ history, sensorType, title }) => {
  const chartRef = React.useRef(null);
  const chartInstance = React.useRef(null);

  const updateChart = () => {
    if (!chartRef.current || !history || history.length === 0) return;
    
    // Destroy existing chart
    if (chartInstance.current) {
      chartInstance.current.destroy();
    }

    // Extract timestamps for x-axis labels
    const labels = history.map(entry => {
      const date = new Date(entry.timestamp);
      return date.toLocaleTimeString();
    });

    const ctx = chartRef.current.getContext('2d');
    let datasets = [];
    let scales = {};

    // Configure chart based on sensor type
    if (sensorType === "imu") {
      // Extract data for IMU sensor
      const accelXData = history.map(entry => entry.accel?.x);
      const accelYData = history.map(entry => entry.accel?.y);
      const accelZData = history.map(entry => entry.accel?.z);
      const temperatureData = history.map(entry => entry.temperature);

      datasets = [
        {
          label: 'Temperature (°C)',
          data: temperatureData,
          borderColor: 'rgba(255, 99, 132, 1)',
          backgroundColor: 'rgba(255, 99, 132, 0.1)',
          borderWidth: 2,
          yAxisID: 'y2',
          fill: true
        },
        {
          label: 'Accel X (g)',
          data: accelXData,
          borderColor: 'rgba(54, 162, 235, 1)',
          backgroundColor: 'rgba(54, 162, 235, 0.1)',
          borderWidth: 1,
          fill: false
        },
        {
          label: 'Accel Y (g)',
          data: accelYData,
          borderColor: 'rgba(75, 192, 192, 1)',
          backgroundColor: 'rgba(75, 192, 192, 0.1)',
          borderWidth: 1,
          fill: false
        },
        {
          label: 'Accel Z (g)',
          data: accelZData,
          borderColor: 'rgba(153, 102, 255, 1)',
          backgroundColor: 'rgba(153, 102, 255, 0.1)',
          borderWidth: 1,
          fill: false
        }
      ];

      scales = {
        x: {
          title: {
            display: true,
            text: 'Time'
          }
        },
        y: {
          title: {
            display: true,
            text: 'Acceleration (g)'
          },
          beginAtZero: false
        },
        y2: {
          position: 'right',
          title: {
            display: true,
            text: 'Temperature (°C)'
          },
          beginAtZero: false,
          grid: {
            drawOnChartArea: false
          }
        }
      };
    } 
    else if (sensorType === "bme280") {
      // Extract data for BME280 sensor
      const temperatureData = history.map(entry => entry.temperature);
      const humidityData = history.map(entry => entry.humidity);
      const pressureData = history.map(entry => entry.pressure / 100); // Convert Pa to hPa

      datasets = [
        {
          label: 'Temperature (°C)',
          data: temperatureData,
          borderColor: 'rgba(255, 99, 132, 1)',
          backgroundColor: 'rgba(255, 99, 132, 0.1)',
          borderWidth: 2,
          fill: true,
          yAxisID: 'y-temp'
        },
        {
          label: 'Humidity (%)',
          data: humidityData,
          borderColor: 'rgba(54, 162, 235, 1)',
          backgroundColor: 'rgba(54, 162, 235, 0.1)',
          borderWidth: 2,
          fill: true,
          yAxisID: 'y-humidity'
        },
        {
          label: 'Pressure (hPa)',
          data: pressureData,
          borderColor: 'rgba(75, 192, 192, 1)',
          backgroundColor: 'rgba(75, 192, 192, 0.1)',
          borderWidth: 2,
          fill: true,
          yAxisID: 'y-pressure'
        }
      ];

      scales = {
        x: {
          title: {
            display: true,
            text: 'Time'
          }
        },
        'y-temp': {
          type: 'linear',
          position: 'left',
          title: {
            display: true,
            text: 'Temperature (°C)'
          },
          grid: {
            drawOnChartArea: true
          }
        },
        'y-humidity': {
          type: 'linear',
          position: 'right',
          title: {
            display: true,
            text: 'Humidity (%)'
          },
          min: 0,
          max: 100,
          grid: {
            drawOnChartArea: false
          }
        },
        'y-pressure': {
          type: 'linear',
          position: 'right',
          title: {
            display: true,
            text: 'Pressure (hPa)'
          },
          grid: {
            drawOnChartArea: false
          }
        }
      };
    }
    else {
      // Generic chart for unknown sensor type
      const keys = Object.keys(history[0]).filter(key => 
        key !== 'timestamp' && typeof history[0][key] !== 'object'
      );
      
      const colors = [
        { border: 'rgba(255, 99, 132, 1)', background: 'rgba(255, 99, 132, 0.1)' },
        { border: 'rgba(54, 162, 235, 1)', background: 'rgba(54, 162, 235, 0.1)' },
        { border: 'rgba(75, 192, 192, 1)', background: 'rgba(75, 192, 192, 0.1)' },
        { border: 'rgba(153, 102, 255, 1)', background: 'rgba(153, 102, 255, 0.1)' },
        { border: 'rgba(255, 159, 64, 1)', background: 'rgba(255, 159, 64, 0.1)' }
      ];
      
      datasets = keys.map((key, index) => {
        const colorIndex = index % colors.length;
        return {
          label: key,
          data: history.map(entry => entry[key]),
          borderColor: colors[colorIndex].border,
          backgroundColor: colors[colorIndex].background,
          borderWidth: 2,
          fill: false
        };
      });

      scales = {
        x: {
          title: {
            display: true,
            text: 'Time'
          }
        },
        y: {
          title: {
            display: true,
            text: 'Value'
          },
          beginAtZero: false
        }
      };
    }

    chartInstance.current = new Chart(ctx, {
      type: 'line',
      data: {
        labels: labels,
        datasets: datasets
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        scales: scales,
        plugins: {
          title: {
            display: true,
            text: title || `${sensorType.toUpperCase()} Sensor Readings`
          },
          legend: {
            position: 'top',
          }
        }
      }
    });
  };

  React.useEffect(() => {
    if (history && history.length > 0) {
      updateChart();
    }
    return () => {
      if (chartInstance.current) {
        chartInstance.current.destroy();
      }
    };
  }, [history, sensorType, title]);

  return (
    <div style={{ width: '100%', height: '100%' }}>
      {history && history.length > 0 ? (
        <canvas ref={chartRef} />
      ) : (
        <div style={{ 
          height: '100%', 
          display: 'flex', 
          alignItems: 'center', 
          justifyContent: 'center',
          color: '#666'
        }}>
          No historical data available yet
        </div>
      )}
    </div>
  );
};

// Legacy component for backward compatibility
window.IMUHistoryChart = ({ history }) => {
  return <SensorHistoryChart history={history} sensorType="imu" title="IMU Sensor Readings" />;
};