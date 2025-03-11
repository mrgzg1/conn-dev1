const deviceIP = location.origin;

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

const App = () => {
  const [sensors, setSensors] = React.useState([]);
  const [currentSensor, setCurrentSensor] = React.useState(null);
  const [sensorData, setSensorData] = React.useState({});
  const [sensorHistory, setSensorHistory] = React.useState([]);
  const [loading, setLoading] = React.useState(false);
  const [autoRefresh, setAutoRefresh] = React.useState(false);
  const refreshIntervalRef = React.useRef(null);

  // Fetch available sensors
  const fetchSensors = async () => {
    try {
      const response = await fetch(`${deviceIP}/api/sensors`);
      const data = await response.json();
      setSensors(data);
      
      // Select the first sensor by default
      if (data.length > 0 && !currentSensor) {
        setCurrentSensor(data[0].endpoint);
      }
    } catch (err) {
      console.error('Error fetching sensors:', err);
    }
  };

  // Fetch current sensor data
  const fetchSensorData = async (sensorEndpoint = currentSensor) => {
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
  const fetchSensorHistory = async (sensorEndpoint = currentSensor) => {
    if (!sensorEndpoint) return;
    
    setLoading(true);
    try {
      const response = await fetch(`${deviceIP}/api/${sensorEndpoint}/history`);
      const data = await response.json();
      setSensorHistory(prevHistory => ({
        ...prevHistory,
        [sensorEndpoint]: data
      }));
    } catch (err) {
      console.error(`Error fetching ${sensorEndpoint} history:`, err);
    } finally {
      setLoading(false);
    }
  };

  // Toggle auto-refresh of data
  const toggleAutoRefresh = () => {
    const newState = !autoRefresh;
    setAutoRefresh(newState);
    
    if (newState && currentSensor) {
      // Start auto-refresh
      refreshIntervalRef.current = setInterval(() => {
        fetchSensorData(currentSensor);
        fetchSensorHistory(currentSensor);
      }, 1000); // Refresh every second
    } else {
      // Stop auto-refresh
      clearInterval(refreshIntervalRef.current);
    }
  };

  // Change current sensor
  const changeSensor = (sensorEndpoint) => {
    setCurrentSensor(sensorEndpoint);
    
    // Fetch data for the new sensor
    fetchSensorData(sensorEndpoint);
    fetchSensorHistory(sensorEndpoint);
    
    // Update auto-refresh if active
    if (autoRefresh) {
      clearInterval(refreshIntervalRef.current);
      refreshIntervalRef.current = setInterval(() => {
        fetchSensorData(sensorEndpoint);
        fetchSensorHistory(sensorEndpoint);
      }, 1000);
    }
  };

  // Initial data fetch
  React.useEffect(() => {
    fetchSensors();
    
    return () => {
      if (refreshIntervalRef.current) {
        clearInterval(refreshIntervalRef.current);
      }
    };
  }, []);

  // When current sensor changes, fetch data
  React.useEffect(() => {
    if (currentSensor) {
      fetchSensorData();
      fetchSensorHistory();
    }
  }, [currentSensor]);

  // Find the current sensor name
  const currentSensorName = React.useMemo(() => {
    const sensor = sensors.find(s => s.endpoint === currentSensor);
    return sensor ? sensor.name : "Sensor";
  }, [sensors, currentSensor]);

  return (
    <div style={{ maxWidth: '1000px', margin: '0 auto', padding: '20px' }}>
      <h1 style={{ textAlign: 'center', color: '#333' }}>Subway Analytics - Sensor Dashboard</h1>
      
      {/* Sensor Selector */}
      <div className="sensor-selector">
        {sensors.map(sensor => (
          <button 
            key={sensor.endpoint}
            onClick={() => changeSensor(sensor.endpoint)}
            className={`sensor-button ${currentSensor === sensor.endpoint ? 'active' : ''}`}
          >
            {sensor.name}
          </button>
        ))}
      </div>
      
      {/* Sensor Data and Statistics */}
      <div className="sensor-grid">
        <div className="sensor-card">
          <h2 style={{ margin: '0 0 15px 0', color: '#444' }}>Current Readings</h2>
          {currentSensor && sensorData[currentSensor] ? (
            <SensorData 
              sensorType={currentSensor} 
              data={sensorData[currentSensor]} 
            />
          ) : (
            <div>Loading sensor data...</div>
          )}
        </div>
        
        <div className="sensor-card">
          <h2 style={{ margin: '0 0 15px 0', color: '#444' }}>Data Statistics</h2>
          {currentSensor && sensorHistory[currentSensor] ? (
            <SensorStatistics 
              sensorType={currentSensor}
              history={sensorHistory[currentSensor]}
            />
          ) : (
            <div>Loading statistics...</div>
          )}
        </div>
      </div>
      
      {/* Sensor Chart */}
      <div className="chart-container">
        {currentSensor && sensorHistory[currentSensor] ? (
          <SensorHistoryChart 
            history={sensorHistory[currentSensor]}
            sensorType={currentSensor}
            title={`${currentSensorName} Readings`}
          />
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
      
      {/* Action Buttons */}
      <div className="button-container">
        <button 
          onClick={() => fetchSensorData()}
          className="action-button primary"
        >
          Refresh Current Data
        </button>
        
        <button 
          onClick={() => fetchSensorHistory()}
          disabled={loading}
          className={`action-button secondary ${loading ? 'disabled' : ''}`}
        >
          {loading ? 'Loading...' : 'Refresh History'}
        </button>
        
        <button 
          onClick={toggleAutoRefresh}
          className={`action-button ${autoRefresh ? 'warning' : 'primary'}`}
        >
          {autoRefresh ? 'Stop Auto-Refresh' : 'Start Auto-Refresh'}
        </button>
      </div>
    </div>
  );
};

// Wait for the chart component to be available
setTimeout(() => {
  ReactDOM.render(<App />, document.getElementById("root"));
}, 100);
