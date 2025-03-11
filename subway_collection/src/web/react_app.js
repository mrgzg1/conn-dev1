const deviceIP = location.origin;

// Dashboard configuration - defines custom views for data visualization
const DASHBOARD_CONFIG = {
  views: [
    {
      id: "temperature",
      title: "Temperature",
      description: "Temperature readings from all sensors",
      metrics: [
        { sensorType: "imu", dataKey: "temperature", label: "IMU Temperature", color: "rgba(255, 99, 132, 1)" },
        { sensorType: "bme280", dataKey: "temperature", label: "BME280 Temperature", color: "rgba(54, 162, 235, 1)" }
      ]
    },
    {
      id: "humidity",
      title: "Humidity",
      description: "Humidity levels from environmental sensor",
      metrics: [
        { sensorType: "bme280", dataKey: "humidity", label: "Humidity", color: "rgba(75, 192, 192, 1)" }
      ]
    },
    {
      id: "pressure",
      title: "Pressure",
      description: "Atmospheric pressure readings",
      metrics: [
        { sensorType: "bme280", dataKey: "pressure", label: "Pressure (hPa)", 
          color: "rgba(153, 102, 255, 1)", 
          transform: (value) => value / 100 // Convert Pa to hPa
        }
      ]
    },
    {
      id: "acceleration",
      title: "IMU Acceleration",
      description: "Accelerometer readings from IMU sensor",
      metrics: [
        { sensorType: "imu", dataKey: "accel.x", label: "X-axis", color: "rgba(255, 99, 132, 1)" },
        { sensorType: "imu", dataKey: "accel.y", label: "Y-axis", color: "rgba(54, 162, 235, 1)" },
        { sensorType: "imu", dataKey: "accel.z", label: "Z-axis", color: "rgba(75, 192, 192, 1)" }
      ]
    }
  ]
};

// Helper function to get nested object values using dot notation (also defined in chart.js)
const getNestedValue = (obj, path) => {
  if (!obj) return null;
  return path.split('.').reduce((prev, curr) => {
    return prev ? prev[curr] : null;
  }, obj);
};

const SensorData = ({ sensorType, data }) => {
  // Helper function to render data based on sensor type
  const renderSensorData = () => {
    if (!data || Object.keys(data).length === 0) {
      return <div>No data available</div>;
    }

    if (sensorType === "imu") {
      return (
        <>
          <div style={{ marginBottom: '10px' }}>
            <strong>Temperature:</strong> {data.temperature}°C
          </div>
          <div style={{ marginBottom: '10px' }}>
            <strong>Accelerometer:</strong><br />
            X: {data.accel?.x?.toFixed(3) || 0} g<br />
            Y: {data.accel?.y?.toFixed(3) || 0} g<br />
            Z: {data.accel?.z?.toFixed(3) || 0} g
          </div>
          <div>
            <strong>Gyroscope:</strong><br />
            X: {data.gyro?.x?.toFixed(3) || 0} dps<br />
            Y: {data.gyro?.y?.toFixed(3) || 0} dps<br />
            Z: {data.gyro?.z?.toFixed(3) || 0} dps
          </div>
        </>
      );
    }
    else if (sensorType === "bme280") {
      return (
        <>
          <div style={{ marginBottom: '10px' }}>
            <strong>Temperature:</strong> {data.temperature?.toFixed(2) || 0}°C
          </div>
          <div style={{ marginBottom: '10px' }}>
            <strong>Humidity:</strong> {data.humidity?.toFixed(2) || 0}%
          </div>
          <div>
            <strong>Pressure:</strong> {(data.pressure / 100)?.toFixed(2) || 0} hPa
          </div>
        </>
      );
    }
    else {
      // Generic display for any sensor
      return (
        <>
          {Object.entries(data).map(([key, value]) => {
            // Skip timestamp and complex objects
            if (key === 'timestamp' || typeof value === 'object') return null;
            
            // Format the value if it's a number
            const formattedValue = typeof value === 'number' ? value.toFixed(2) : value;
            
            return (
              <div key={key} style={{ marginBottom: '10px' }}>
                <strong>{key.charAt(0).toUpperCase() + key.slice(1)}:</strong> {formattedValue}
              </div>
            );
          })}
        </>
      );
    }
  };

  return renderSensorData();
};

const SensorStatistics = ({ sensorType, history }) => {
  if (!history || history.length === 0) {
    return <div>No historical data available yet</div>;
  }

  // Helper function to render statistics based on sensor type
  const renderSensorStatistics = () => {
    if (sensorType === "imu") {
      return (
        <>
          <div style={{ marginBottom: '10px' }}>
            <strong>Samples:</strong> {history.length}
          </div>
          <div style={{ marginBottom: '10px' }}>
            <strong>Avg. Temperature:</strong> {(history.reduce((sum, item) => sum + item.temperature, 0) / history.length).toFixed(1)}°C
          </div>
          <div>
            <strong>Max Acceleration:</strong> {Math.max(...history.map(item => 
              Math.sqrt(item.accel?.x**2 + item.accel?.y**2 + item.accel?.z**2)
            )).toFixed(3)} g
          </div>
        </>
      );
    }
    else if (sensorType === "bme280") {
      return (
        <>
          <div style={{ marginBottom: '10px' }}>
            <strong>Samples:</strong> {history.length}
          </div>
          <div style={{ marginBottom: '10px' }}>
            <strong>Avg. Temperature:</strong> {(history.reduce((sum, item) => sum + item.temperature, 0) / history.length).toFixed(1)}°C
          </div>
          <div style={{ marginBottom: '10px' }}>
            <strong>Avg. Humidity:</strong> {(history.reduce((sum, item) => sum + item.humidity, 0) / history.length).toFixed(1)}%
          </div>
          <div>
            <strong>Avg. Pressure:</strong> {(history.reduce((sum, item) => sum + item.pressure, 0) / history.length / 100).toFixed(1)} hPa
          </div>
        </>
      );
    }
    else {
      // Find numeric values to calculate statistics
      const numericKeys = Object.keys(history[0]).filter(key => 
        key !== 'timestamp' && typeof history[0][key] === 'number'
      );
      
      return (
        <>
          <div style={{ marginBottom: '10px' }}>
            <strong>Samples:</strong> {history.length}
          </div>
          {numericKeys.map(key => {
            const avg = (history.reduce((sum, item) => sum + item[key], 0) / history.length).toFixed(2);
            return (
              <div key={key} style={{ marginBottom: '10px' }}>
                <strong>Avg. {key.charAt(0).toUpperCase() + key.slice(1)}:</strong> {avg}
              </div>
            );
          })}
        </>
      );
    }
  };

  return renderSensorStatistics();
};

// Custom View Panel Component
const CustomViewPanel = ({ view, sensorData, sensorHistory, refreshAllData, loading }) => {
  return (
    <div className="sensor-panel">
      <h2 style={{ margin: '0 0 15px 0', color: '#444', textAlign: 'center' }}>{view.title}</h2>
      <p style={{ margin: '0 0 20px 0', color: '#666', textAlign: 'center' }}>{view.description}</p>
      
      {/* Custom Chart */}
      <div className="chart-container">
        <CustomSensorChart 
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
            let value = currentData ? getNestedValue(currentData, metric.dataKey) : null;
            
            if (value !== null && metric.transform) {
              value = metric.transform(value);
            }
            
            return (
              <div key={index} className="current-value-item">
                <div className="metric-label" style={{ color: metric.color }}>
                  {metric.label}:
                </div>
                <div className="metric-value">
                  {value !== null ? value.toFixed(2) : "N/A"}
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

// DataSync component to handle data fetching separate from rendering
const DataSync = ({ children }) => {
  const [sensors, setSensors] = React.useState([]);
  const [sensorData, setSensorData] = React.useState({});
  const [sensorHistory, setSensorHistory] = React.useState({});
  const [loading, setLoading] = React.useState({});
  const [autoRefresh, setAutoRefresh] = React.useState(false);
  const refreshIntervalRef = React.useRef(null);
  const [lastUpdated, setLastUpdated] = React.useState(new Date());

  // Fetch available sensors
  const fetchSensors = async () => {
    try {
      const response = await fetch(`${deviceIP}/api/sensors`);
      const data = await response.json();
      setSensors(data);
      
      // Initialize loading state for each sensor
      const initialLoadingState = {};
      data.forEach(sensor => {
        initialLoadingState[sensor.endpoint] = false;
      });
      setLoading(initialLoadingState);
      
      // Fetch data for all sensors
      data.forEach(sensor => {
        fetchSensorData(sensor.endpoint);
        fetchSensorHistory(sensor.endpoint);
      });
    } catch (err) {
      console.error('Error fetching sensors:', err);
    }
  };

  // Fetch current sensor data
  const fetchSensorData = async (sensorEndpoint) => {
    if (!sensorEndpoint) return;
    
    try {
      const response = await fetch(`${deviceIP}/api/${sensorEndpoint}/data`);
      const data = await response.json();
      setSensorData(prevData => ({
        ...prevData,
        [sensorEndpoint]: data
      }));
    } catch (err) {
      console.error(`Error fetching ${sensorEndpoint} data:`, err);
    }
  };

  // Fetch sensor history data
  const fetchSensorHistory = async (sensorEndpoint) => {
    if (!sensorEndpoint) return;
    
    setLoading(prev => ({ ...prev, [sensorEndpoint]: true }));
    try {
      const response = await fetch(`${deviceIP}/api/${sensorEndpoint}/history`);
      const data = await response.json();
      setSensorHistory(prevHistory => ({
        ...prevHistory,
        [sensorEndpoint]: data
      }));
      setLastUpdated(new Date());
    } catch (err) {
      console.error(`Error fetching ${sensorEndpoint} history:`, err);
    } finally {
      setLoading(prev => ({ ...prev, [sensorEndpoint]: false }));
    }
  };

  // Refresh all sensor data
  const refreshAllData = () => {
    sensors.forEach(sensor => {
      fetchSensorData(sensor.endpoint);
      fetchSensorHistory(sensor.endpoint);
    });
  };

  // Toggle auto-refresh of data
  const toggleAutoRefresh = () => {
    const newState = !autoRefresh;
    setAutoRefresh(newState);
    
    if (newState) {
      // Start auto-refresh for all sensors
      refreshIntervalRef.current = setInterval(() => {
        refreshAllData();
      }, 2000); // Refresh every 2 seconds
    } else {
      // Stop auto-refresh
      clearInterval(refreshIntervalRef.current);
    }
  };

  // Initial data fetch
  React.useEffect(() => {
    fetchSensors();
    
    // Start initial auto-refresh
    refreshIntervalRef.current = setInterval(() => {
      refreshAllData();
    }, 2000);
    setAutoRefresh(true);
    
    return () => {
      if (refreshIntervalRef.current) {
        clearInterval(refreshIntervalRef.current);
      }
    };
  }, []);

  // Create a download link for the sensor data
  const downloadSensorData = () => {
    const dataToExport = {
      timestamp: new Date().toISOString(),
      sensors: {}
    };
    
    // Add current readings and history for each sensor
    sensors.forEach(sensor => {
      const endpoint = sensor.endpoint;
      dataToExport.sensors[endpoint] = {
        name: sensor.name,
        currentData: sensorData[endpoint] || null,
        historyData: sensorHistory[endpoint] || []
      };
    });
    
    // Create a file for download
    const dataStr = JSON.stringify(dataToExport, null, 2);
    const blob = new Blob([dataStr], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    
    // Create and trigger download link
    const link = document.createElement('a');
    link.href = url;
    link.download = `subway_sensor_data_${new Date().toISOString().replace(/[:.]/g, '-')}.json`;
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
  };

  // Pass all the data down to children components
  return children({
    sensors,
    sensorData,
    sensorHistory,
    loading,
    autoRefresh,
    refreshAllData,
    toggleAutoRefresh,
    downloadSensorData,
    lastUpdated
  });
};

const Dashboard = ({ sensors, sensorData, sensorHistory, loading, autoRefresh, refreshAllData, toggleAutoRefresh, downloadSensorData, lastUpdated }) => {
  return (
    <div style={{ maxWidth: '1200px', margin: '0 auto', padding: '20px' }}>
      <h1 style={{ textAlign: 'center', color: '#333' }}>Subway Analytics - Sensor Dashboard</h1>
      <div style={{ textAlign: 'center', color: '#666', marginBottom: '20px' }}>
        Last Updated: {lastUpdated.toLocaleTimeString()}
      </div>
      
      {/* Global Action Buttons */}
      <div className="button-container" style={{ marginBottom: '30px' }}>
        <button 
          onClick={refreshAllData}
          className="action-button primary"
        >
          Refresh All Data
        </button>
        
        <button 
          onClick={toggleAutoRefresh}
          className={`action-button ${autoRefresh ? 'warning' : 'secondary'}`}
        >
          {autoRefresh ? 'Stop Auto-Refresh' : 'Start Auto-Refresh'}
        </button>
        
        <button 
          onClick={downloadSensorData}
          className="action-button primary"
        >
          Download All Data
        </button>
      </div>
      
      {/* Custom Views Panels */}
      {sensors.length > 0 ? (
        <div className="all-sensors-container">
          {DASHBOARD_CONFIG.views.map(view => (
            <CustomViewPanel
              key={view.id}
              view={view}
              sensorData={sensorData}
              sensorHistory={sensorHistory}
              refreshAllData={refreshAllData}
              loading={loading}
            />
          ))}
        </div>
      ) : (
        <div style={{ textAlign: 'center', padding: '40px 0', color: '#666' }}>
          Loading sensors...
        </div>
      )}
      
      {/* Raw Sensor Data */}
      <div className="sensor-details" style={{ marginTop: '40px' }}>
        <h2 style={{ textAlign: 'center', color: '#333', marginBottom: '20px' }}>Available Sensors</h2>
        <div className="sensors-list">
          {sensors.map(sensor => (
            <div key={sensor.endpoint} className="sensor-info-card">
              <h3>{sensor.name}</h3>
              <p>API Endpoint: {sensor.endpoint}</p>
              <p>Data Points: {sensorHistory[sensor.endpoint]?.length || 0}</p>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
};

const App = () => {
  return <DataSync>{props => <Dashboard {...props} />}</DataSync>;
};

// Wait for the chart component to be available
setTimeout(() => {
  ReactDOM.render(<App />, document.getElementById("root"));
}, 100);
