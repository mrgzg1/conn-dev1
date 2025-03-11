// Dashboard configuration - defines custom views for data visualization
window.DASHBOARD_CONFIG = {
  // Sensor-level data validation - defines valid ranges and filtering rules
  validation: {
    sensors: {
      "imu": {
        fields: {
          "temperature": { min: 0, max: 100 }, // Normal temperature range in °C
          "accel.x": { min: -5, max: 5 },      // Normal acceleration range in g
          "accel.y": { min: -5, max: 5 },
          "accel.z": { min: -5, max: 5 },
          "gyro.x": { min: -200, max: 200 },   // Normal gyro range in dps
          "gyro.y": { min: -200, max: 200 },
          "gyro.z": { min: -200, max: 200 }
        }
      },
      "bme280": {
        fields: {
          "temperature": { min: -40, max: 85 },  // BME280 spec range
          "humidity": { min: 0, max: 100 },      // Humidity percentage
          "pressure": { min: 30000, max: 110000 } // Normal atmospheric pressure range in Pa
        }
      }
    },
    // Default filter function for outliers - can be overridden per metric
    isValidValue: function(value, validationRule) {
      // Check if value is null, undefined, NaN, or outside valid range
      if (value === null || value === undefined || isNaN(value)) {
        return false;
      }
      if (validationRule) {
        if (validationRule.min !== undefined && value < validationRule.min) {
          return false;
        }
        if (validationRule.max !== undefined && value > validationRule.max) {
          return false;
        }
      }
      return true;
    }
  },
  
  views: [
    {
      id: "temperature",
      title: "Temperature",
      description: "Temperature readings from all sensors",
      metrics: [
        { 
          sensorType: "imu", 
          dataKey: "temperature", 
          label: "IMU Temperature", 
          color: "rgba(255, 99, 132, 1)",
          // Custom validation can override the default sensor validation
          validation: { min: 0, max: 50 }
        },
        { 
          sensorType: "bme280", 
          dataKey: "temperature", 
          label: "BME280 Temperature", 
          color: "rgba(54, 162, 235, 1)"
        }
      ]
    },
    {
      id: "humidity",
      title: "Humidity",
      description: "Humidity levels from environmental sensor",
      metrics: [
        { 
          sensorType: "bme280", 
          dataKey: "humidity", 
          label: "Humidity", 
          color: "rgba(75, 192, 192, 1)"
        }
      ]
    },
    {
      id: "pressure",
      title: "Pressure",
      description: "Atmospheric pressure readings",
      metrics: [
        { 
          sensorType: "bme280", 
          dataKey: "pressure", 
          label: "Pressure (hPa)", 
          color: "rgba(153, 102, 255, 1)", 
          transform: (value) => value / 100, // Convert Pa to hPa
          validation: { min: 300, max: 1100 } // Valid range after transformation to hPa
        }
      ]
    },
    {
      id: "acceleration",
      title: "IMU Acceleration",
      description: "Accelerometer readings from IMU sensor",
      metrics: [
        { 
          sensorType: "imu", 
          dataKey: "accel.x", 
          label: "X-axis", 
          color: "rgba(255, 99, 132, 1)"
        },
        { 
          sensorType: "imu", 
          dataKey: "accel.y", 
          label: "Y-axis", 
          color: "rgba(54, 162, 235, 1)"
        },
        { 
          sensorType: "imu", 
          dataKey: "accel.z", 
          label: "Z-axis", 
          color: "rgba(75, 192, 192, 1)"
        }
      ]
    }
  ]
};

// Helper function to get nested object values using dot notation
const getNestedValue = (obj, path) => {
  if (!obj) return null;
  return path.split('.').reduce((prev, curr) => {
    return prev ? prev[curr] : null;
  }, obj);
};

// Validation helper function - also used by CustomSensorChart
window.validateDataPoint = (metric, value) => {
  // Get validation config - prefer metric-specific validation over sensor default
  const config = window.DASHBOARD_CONFIG.validation;
  const sensorValidation = config.sensors[metric.sensorType]?.fields || {};
  
  // Get validation rule - prefer metric-specific over sensor default
  let dataKey = metric.dataKey;
  const validationRule = metric.validation || 
                        sensorValidation[dataKey] || 
                        {};
  
  // Transform value if needed before validation
  const transformedValue = metric.transform ? metric.transform(value) : value;
  
  // Validate using the isValidValue function
  return config.isValidValue(transformedValue, validationRule) ? transformedValue : null;
};

// Custom View Panel Component
window.CustomViewPanel = ({ view, sensorData, sensorHistory, refreshAllData, loading }) => {
  return (
    <div className="sensor-panel">
      <h2 style={{ margin: '0 0 15px 0', color: '#444', textAlign: 'center' }}>{view.title}</h2>
      <p style={{ margin: '0 0 20px 0', color: '#666', textAlign: 'center' }}>{view.description}</p>
      
      {/* Custom Chart */}
      <div className="chart-container">
        <window.CustomSensorChart 
          view={view}
          sensorData={sensorData}
          sensorHistory={sensorHistory}
        />
      </div>

      {/* Current Values Display */}
      <div className="current-values-container">
        <h3 style={{ margin: '10px 0', color: '#555' }}>Current Values</h3>
        <div className="current-values-grid">
          {view.metrics.map((metric, index) => {
            const currentData = sensorData[metric.sensorType];
            let rawValue = currentData ? getNestedValue(currentData, metric.dataKey) : null;
            
            // Validate value using our validation rules
            const validValue = window.validateDataPoint(metric, rawValue);
            
            return (
              <div key={index} className="current-value-item">
                <div className="metric-label" style={{ color: metric.color }}>
                  {metric.label}:
                </div>
                <div className="metric-value">
                  {validValue !== null ? validValue.toFixed(2) : "N/A"}
                </div>
              </div>
            );
          })}
        </div>
      </div>
      
      {/* Refresh Button */}
      <div className="button-container">
        <button 
          onClick={refreshAllData}
          disabled={Object.values(loading).some(val => val)}
          className="action-button secondary"
        >
          {Object.values(loading).some(val => val) ? 'Loading...' : 'Refresh Data'}
        </button>
      </div>
    </div>
  );
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
      
      // Filter and validate data points
      const validData = history.map((entry, index) => {
        const rawValue = getNestedValue(entry, metric.dataKey);
        const validValue = window.validateDataPoint(metric, rawValue);
        
        // Return object with timestamp and value for filtering
        return {
          timestamp: entry.timestamp,
          value: validValue,
          index: index
        };
      }).filter(item => item.value !== null);
      
      // Extract just the validated values in the original order
      const data = new Array(history.length).fill(null);
      validData.forEach(item => {
        data[item.index] = item.value;
      });
      
      return {
        label: metric.label,
        data: data,
        borderColor: metric.color,
        backgroundColor: metric.color.replace('1)', '0.1)'),
        borderWidth: 2,
        fill: false,
        tension: 0.2, // Adds smoother curves
        spanGaps: true // Connect lines across null/invalid values
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

// Helper functions for data export and processing

// Generate time-aligned CSV data with validation
window.generateTimeAlignedCSV = (timeAlignedData) => {
  if (!timeAlignedData) {
    console.error("No time-aligned data available to export");
    return null;
  }
  
  // Build CSV headers
  const headers = ['realTimestamp', 'deviceTimestamp'];
  const sensorTypes = Object.keys(timeAlignedData.data);
  
  // Collect all field names from all sensors and create metric configs for validation
  const fieldMap = {};
  const metricConfigs = {}; // Map of headerName -> metric config for validation
  
  sensorTypes.forEach(sensorType => {
    if (!timeAlignedData.data[sensorType] || !timeAlignedData.data[sensorType].history || 
        timeAlignedData.data[sensorType].history.length === 0) {
      return;
    }
    
    // Get field names from the first data point
    const sample = timeAlignedData.data[sensorType].history[0];
    
    Object.keys(sample).forEach(key => {
      // Skip timestamp fields
      if (key === 'timestamp' || key === 'realTimestamp' || key === 'originalTimestamp') return;
      
      if (typeof sample[key] === 'object' && sample[key] !== null) {
        // Handle nested objects like accel and gyro
        Object.keys(sample[key]).forEach(subKey => {
          const headerName = `${sensorType}:${key}.${subKey}`;
          headers.push(headerName);
          fieldMap[`${sensorType}.${key}.${subKey}`] = { sensorType, field: key, subField: subKey };
          
          // Create metric config for validation - similar to dashboard metrics
          metricConfigs[headerName] = {
            sensorType: sensorType,
            dataKey: `${key}.${subKey}` // Use dot notation for nested fields
          };
          
          // Check if this field has a validation in the dashboard config
          const dashboardViews = window.DASHBOARD_CONFIG.views;
          dashboardViews.forEach(view => {
            view.metrics.forEach(metric => {
              if (metric.sensorType === sensorType && metric.dataKey === `${key}.${subKey}`) {
                // Copy transform and validation from dashboard config
                if (metric.transform) metricConfigs[headerName].transform = metric.transform;
                if (metric.validation) metricConfigs[headerName].validation = metric.validation;
              }
            });
          });
        });
      } else {
        // Handle flat fields
        const headerName = `${sensorType}:${key}`;
        headers.push(headerName);
        fieldMap[`${sensorType}.${key}`] = { sensorType, field: key };
        
        // Create metric config for validation
        metricConfigs[headerName] = {
          sensorType: sensorType,
          dataKey: key
        };
        
        // Check if this field has a validation in the dashboard config
        const dashboardViews = window.DASHBOARD_CONFIG.views;
        dashboardViews.forEach(view => {
          view.metrics.forEach(metric => {
            if (metric.sensorType === sensorType && metric.dataKey === key) {
              // Copy transform and validation from dashboard config
              if (metric.transform) metricConfigs[headerName].transform = metric.transform;
              if (metric.validation) metricConfigs[headerName].validation = metric.validation;
            }
          });
        });
      }
    });
  });
  
  // Start CSV string with headers
  let csv = headers.join(',') + '\n';
  
  // Combine data from all sensors by timestamp
  const allDataPoints = [];
  
  sensorTypes.forEach(sensorType => {
    if (!timeAlignedData.data[sensorType] || !timeAlignedData.data[sensorType].history) return;
    
    timeAlignedData.data[sensorType].history.forEach(point => {
      if (!point.realTimestamp) return;
      
      allDataPoints.push({
        sensorType,
        realTimestamp: point.realTimestamp,
        deviceTimestamp: point.originalTimestamp || point.timestamp,
        data: point
      });
    });
  });
  
  // Sort all data points by realTimestamp
  allDataPoints.sort((a, b) => {
    return new Date(a.realTimestamp) - new Date(b.realTimestamp);
  });
  
  // Group data points by realTimestamp
  const rowsByTimestamp = {};
  
  allDataPoints.forEach(point => {
    const { realTimestamp, deviceTimestamp, sensorType, data } = point;
    
    if (!rowsByTimestamp[realTimestamp]) {
      // Initialize a new row with empty values
      const row = new Array(headers.length).fill('');
      row[0] = realTimestamp;
      row[1] = deviceTimestamp;
      rowsByTimestamp[realTimestamp] = row;
    }
    
    // Add data values to the row
    Object.keys(data).forEach(key => {
      // Skip timestamp fields
      if (key === 'timestamp' || key === 'realTimestamp' || key === 'originalTimestamp') return;
      
      if (typeof data[key] === 'object' && data[key] !== null) {
        // Handle nested objects
        Object.keys(data[key]).forEach(subKey => {
          const headerName = `${sensorType}:${key}.${subKey}`;
          const headerIndex = headers.indexOf(headerName);
          
          if (headerIndex > 0) {
            const rawValue = data[key][subKey];
            
            // Validate the value if we have a metric config for it
            if (metricConfigs[headerName]) {
              const metric = metricConfigs[headerName];
              const validValue = window.validateDataPoint(metric, rawValue);
              rowsByTimestamp[realTimestamp][headerIndex] = validValue !== null ? validValue : '';
            } else {
              rowsByTimestamp[realTimestamp][headerIndex] = rawValue;
            }
          }
        });
      } else {
        // Handle flat fields
        const headerName = `${sensorType}:${key}`;
        const headerIndex = headers.indexOf(headerName);
        
        if (headerIndex > 0) {
          const rawValue = data[key];
          
          // Validate the value if we have a metric config for it
          if (metricConfigs[headerName]) {
            const metric = metricConfigs[headerName];
            const validValue = window.validateDataPoint(metric, rawValue);
            rowsByTimestamp[realTimestamp][headerIndex] = validValue !== null ? validValue : '';
          } else {
            rowsByTimestamp[realTimestamp][headerIndex] = rawValue;
          }
        }
      }
    });
  });
  
  // Convert rows object to array and sort by timestamp
  const sortedRows = Object.values(rowsByTimestamp)
    .sort((a, b) => new Date(a[0]) - new Date(b[0]));
  
  // Add rows to CSV
  sortedRows.forEach(row => {
    csv += row.join(',') + '\n';
  });
  
  return csv;
};

// Generate regular CSV data with validation
window.convertToCSV = (sensors, sensorData, sensorHistory) => {
  // Define CSV headers based on available sensors
  const headers = ['timestamp'];
  const metricConfigs = {}; // For data validation
  
  // Build header columns based on available sensors and their data structure
  sensors.forEach(sensor => {
    const endpoint = sensor.endpoint;
    const history = sensorHistory[endpoint] || [];
    
    // Skip empty sensors
    if (!history.length) return;
    
    // Use first data point to determine available fields
    const sample = history[0] || {};
    
    // Add each field to headers with sensor prefix
    Object.keys(sample).forEach(key => {
      // Skip timestamp as we already have it
      if (key === 'timestamp') return;
      
      // Handle nested objects like accel and gyro
      if (typeof sample[key] === 'object' && sample[key] !== null) {
        Object.keys(sample[key]).forEach(subKey => {
          const headerName = `${endpoint}:${key}.${subKey}`;
          headers.push(headerName);
          
          // Create metric config for validation
          metricConfigs[headerName] = {
            sensorType: endpoint,
            dataKey: `${key}.${subKey}`
          };
          
          // Check for validation config in dashboard
          const dashboardViews = window.DASHBOARD_CONFIG.views;
          dashboardViews.forEach(view => {
            view.metrics.forEach(metric => {
              if (metric.sensorType === endpoint && metric.dataKey === `${key}.${subKey}`) {
                if (metric.transform) metricConfigs[headerName].transform = metric.transform;
                if (metric.validation) metricConfigs[headerName].validation = metric.validation;
              }
            });
          });
        });
      } else {
        const headerName = `${endpoint}:${key}`;
        headers.push(headerName);
        
        // Create metric config for validation
        metricConfigs[headerName] = {
          sensorType: endpoint,
          dataKey: key
        };
        
        // Check for validation config in dashboard
        const dashboardViews = window.DASHBOARD_CONFIG.views;
        dashboardViews.forEach(view => {
          view.metrics.forEach(metric => {
            if (metric.sensorType === endpoint && metric.dataKey === key) {
              if (metric.transform) metricConfigs[headerName].transform = metric.transform;
              if (metric.validation) metricConfigs[headerName].validation = metric.validation;
            }
          });
        });
      }
    });
  });
  
  // Start CSV with headers
  let csv = headers.join(',') + '\n';
  
  // Create a map of timestamps to data rows
  const timeMap = new Map();
  
  // Process each sensor's history data
  sensors.forEach(sensor => {
    const endpoint = sensor.endpoint;
    const history = sensorHistory[endpoint] || [];
    
    history.forEach(point => {
      const timestamp = point.timestamp;
      
      // Create row for this timestamp if it doesn't exist
      if (!timeMap.has(timestamp)) {
        // Initialize with empty values for all columns
        const row = new Array(headers.length).fill('');
        row[0] = timestamp; // Set timestamp
        timeMap.set(timestamp, row);
      }
      
      // Get the row for this timestamp
      const row = timeMap.get(timestamp);
      
      // Populate the row with values from this data point
      Object.keys(point).forEach(key => {
        // Skip timestamp as it's already set
        if (key === 'timestamp') return;
        
        if (typeof point[key] === 'object' && point[key] !== null) {
          // Handle nested objects like accel and gyro
          Object.keys(point[key]).forEach(subKey => {
            const headerName = `${endpoint}:${key}.${subKey}`;
            const headerIndex = headers.indexOf(headerName);
            
            if (headerIndex > 0) { // Skip timestamp column (index 0)
              const rawValue = point[key][subKey];
              
              // Apply validation if we have a metric config
              if (metricConfigs[headerName]) {
                const validValue = window.validateDataPoint(metricConfigs[headerName], rawValue);
                row[headerIndex] = validValue !== null ? validValue : '';
              } else {
                row[headerIndex] = rawValue;
              }
            }
          });
        } else {
          // Handle simple values
          const headerName = `${endpoint}:${key}`;
          const headerIndex = headers.indexOf(headerName);
          
          if (headerIndex > 0) { // Skip timestamp column (index 0)
            const rawValue = point[key];
            
            // Apply validation if we have a metric config
            if (metricConfigs[headerName]) {
              const validValue = window.validateDataPoint(metricConfigs[headerName], rawValue);
              row[headerIndex] = validValue !== null ? validValue : '';
            } else {
              row[headerIndex] = rawValue;
            }
          }
        }
      });
    });
  });
  
  // Sort by timestamp and add rows to CSV
  Array.from(timeMap.entries())
    .sort((a, b) => a[0] - b[0]) // Sort by timestamp
    .forEach(([_, row]) => {
      csv += row.join(',') + '\n';
    });
  
  return csv;
};