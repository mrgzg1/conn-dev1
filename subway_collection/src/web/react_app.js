const deviceIP = location.origin;

// Get the dashboard configuration from chart.js
const DASHBOARD_CONFIG = window.DASHBOARD_CONFIG;

// Helper function to get nested object values using dot notation (defined in chart.js)
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

// CustomViewPanel is now defined in chart.js
const CustomViewPanel = window.CustomViewPanel;

// Sensor data store for persistent browser storage
class SensorDataStore {
  constructor() {
    this.store = this.loadFromLocalStorage() || this.createInitialStore();
  }
  
  loadFromLocalStorage() {
    const data = localStorage.getItem('subwaySensorData');
    return data ? JSON.parse(data) : null;
  }
  
  saveToLocalStorage() {
    localStorage.setItem('subwaySensorData', JSON.stringify(this.store));
  }
  
  createInitialStore() {
    return {
      sessions: [],
      currentSession: null
    };
  }
  
  createNewSession(firstDataPoint) {
    const sessionId = Date.now().toString();
    const realWorldTime = new Date().toISOString();
    const lowestTimestamp = this.findLowestTimestamp(firstDataPoint);
    
    const session = {
      id: sessionId,
      startRealTime: realWorldTime,
      deviceBootTime: lowestTimestamp,
      data: {}
    };
    
    // Initialize storage for each sensor type
    Object.keys(firstDataPoint.sensors).forEach(sensorKey => {
      session.data[sensorKey] = { history: [], latestTimestamp: 0 };
    });
    
    this.store.sessions.push(session);
    this.store.currentSession = sessionId;
    this.saveToLocalStorage();
    
    return session;
  }
  
  findLowestTimestamp(data) {
    // Find earliest timestamp in the dataset
    let lowestTime = Number.MAX_SAFE_INTEGER;
    
    Object.keys(data.sensors).forEach(sensorKey => {
      const sensor = data.sensors[sensorKey];
      if (sensor.historyData && sensor.historyData.length > 0) {
        // Find first entry with valid timestamp
        const firstValidEntry = sensor.historyData.find(entry => 
          entry.timestamp !== null && entry.timestamp > 0
        );
        if (firstValidEntry && firstValidEntry.timestamp < lowestTime) {
          lowestTime = firstValidEntry.timestamp;
        }
      }
    });
    
    return lowestTime === Number.MAX_SAFE_INTEGER ? 0 : lowestTime;
  }
  
  isNewSession(data) {
    if (!this.store.currentSession) return true;
    
    const currentSession = this.getSession(this.store.currentSession);
    if (!currentSession) return true;
    
    // Check for timestamp reset pattern
    // If new data has timestamps much lower than our stored latest timestamps
    const latestStoredTime = this.getLatestTimestamp(currentSession);
    const earliestNewTime = this.findLowestTimestamp(data);
    
    // If new data starts with timestamps at least 10s less than our latest data
    // it's probably a new session (device restarted)
    return earliestNewTime < latestStoredTime - 10000;
  }
  
  getLatestTimestamp(session) {
    let latest = 0;
    Object.keys(session.data).forEach(sensorKey => {
      if (session.data[sensorKey].latestTimestamp > latest) {
        latest = session.data[sensorKey].latestTimestamp;
      }
    });
    return latest;
  }
  
  getSession(sessionId) {
    return this.store.sessions.find(s => s.id === sessionId);
  }
  
  getCurrentSession() {
    if (!this.store.currentSession) return null;
    return this.getSession(this.store.currentSession);
  }
  
  addData(data) {
    if (this.isNewSession(data)) {
      return this.createNewSession(data);
    }
    
    const session = this.getSession(this.store.currentSession);
    
    // Add new data points to the session
    Object.keys(data.sensors).forEach(sensorKey => {
      const sensorData = data.sensors[sensorKey];
      
      // Initialize if needed
      if (!session.data[sensorKey]) {
        session.data[sensorKey] = { history: [], latestTimestamp: 0 };
      }
      
      // Add history data
      if (sensorData.historyData && sensorData.historyData.length > 0) {
        // Filter for data points newer than what we have
        const newPoints = sensorData.historyData.filter(point => 
          point.timestamp > session.data[sensorKey].latestTimestamp
        );
        
        if (newPoints.length > 0) {
          session.data[sensorKey].history.push(...newPoints);
          
          // Update latest timestamp
          const latestPoint = newPoints.reduce((latest, point) => 
            point.timestamp > latest ? point.timestamp : latest, 
            session.data[sensorKey].latestTimestamp
          );
          
          session.data[sensorKey].latestTimestamp = latestPoint;
        }
      }
    });
    
    this.saveToLocalStorage();
    return session;
  }
  
  getTimeAlignedData(sessionId = null) {
    const session = sessionId ? 
      this.getSession(sessionId) : 
      this.getSession(this.store.currentSession);
    
    if (!session) return null;
    
    // Deep clone to avoid modifying the original data
    const result = JSON.parse(JSON.stringify(session));
    
    // Calculate real-world timestamps for each data point
    const sessionStartTime = new Date(session.startRealTime).getTime();
    const deviceBootTime = session.deviceBootTime;
    
    // Transform timestamps
    Object.keys(result.data).forEach(sensorKey => {
      if (!result.data[sensorKey] || !result.data[sensorKey].history) return;
      
      result.data[sensorKey].history = result.data[sensorKey].history.map(point => {
        // Only transform if point has a valid timestamp
        if (point.timestamp !== null) {
          const offsetFromBoot = point.timestamp - deviceBootTime;
          const realTime = sessionStartTime + offsetFromBoot;
          
          // Create a new point with real timestamp
          return {
            ...point,
            realTimestamp: new Date(realTime).toISOString(),
            originalTimestamp: point.timestamp
          };
        }
        return point;
      });
    });
    
    return result;
  }
  
  // For debugging
  clearAllData() {
    localStorage.removeItem('subwaySensorData');
    this.store = this.createInitialStore();
    return this.store;
  }
  
  // Get session list
  getSessions() {
    return this.store.sessions.map(session => ({
      id: session.id,
      startTime: session.startRealTime,
      isCurrent: session.id === this.store.currentSession
    }));
  }
  
  // Get data stats for all sessions
  getDataStats() {
    return {
      sessionCount: this.store.sessions.length,
      currentSession: this.store.currentSession,
      totalDataPoints: this.store.sessions.reduce((total, session) => {
        let sessionTotal = 0;
        Object.keys(session.data).forEach(sensorKey => {
          sessionTotal += session.data[sensorKey].history?.length || 0;
        });
        return total + sessionTotal;
      }, 0)
    };
  }
}

// DataSync component to handle data fetching separate from rendering
const DataSync = ({ children }) => {
  const [sensors, setSensors] = React.useState([]);
  const [sensorData, setSensorData] = React.useState({});
  const [sensorHistory, setSensorHistory] = React.useState({});
  const [loading, setLoading] = React.useState({});
  const [autoRefresh, setAutoRefresh] = React.useState(false);
  const refreshIntervalRef = React.useRef(null);
  const [lastUpdated, setLastUpdated] = React.useState(new Date());
  
  // Initialize data store
  const dataStore = React.useRef(new SensorDataStore());

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

  // Store data in localStorage
  const storeCurrentData = React.useCallback(() => {
    // Only store if we have data
    if (Object.keys(sensorData).length === 0 || Object.keys(sensorHistory).length === 0) {
      return;
    }

    // Format data for storage
    const dataToStore = {
      timestamp: new Date().toISOString(),
      sensors: {}
    };
    
    sensors.forEach(sensor => {
      const endpoint = sensor.endpoint;
      dataToStore.sensors[endpoint] = {
        name: sensor.name,
        currentData: sensorData[endpoint] || null,
        historyData: sensorHistory[endpoint] || []
      };
    });
    
    // Add to data store
    dataStore.current.addData(dataToStore);
    
  }, [sensors, sensorData, sensorHistory]);
  
  // Export stored data as CSV with time alignment
  const downloadStoredData = () => {
    // Get time-aligned data from the current session
    const timeAlignedData = dataStore.current.getTimeAlignedData();
    
    if (!timeAlignedData) {
      console.error("No stored data available to download");
      return;
    }
    
    // Use the chart.js helper function to generate CSV
    const csv = window.generateTimeAlignedCSV(timeAlignedData);
    
    if (!csv) {
      console.error("Failed to generate CSV");
      return;
    }
    
    // Create and download the CSV file
    const blob = new Blob([csv], { type: 'text/csv' });
    const url = URL.createObjectURL(blob);
    
    const link = document.createElement('a');
    link.href = url;
    link.download = `subway_aligned_data_${new Date().toISOString().replace(/[:.]/g, '-')}.csv`;
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
  };
  
  // Initial data fetch
  React.useEffect(() => {
    fetchSensors();
    
    // Start initial auto-refresh
    refreshIntervalRef.current = setInterval(() => {
      refreshAllData();
    }, 2000);
    setAutoRefresh(true);
    
    // Cleanup function to clear interval when component unmounts
    return () => {
      if (refreshIntervalRef.current) {
        clearInterval(refreshIntervalRef.current);
      }
    };
  }, []); // Empty dependency array
  
  // Store data whenever it changes
  React.useEffect(() => {
    storeCurrentData();
  }, [sensorHistory, storeCurrentData]);

  // Create a download link for the sensor data
  const downloadSensorData = () => {
    // Generate CSV from sensor data using the helper function from chart.js
    const csv = window.convertToCSV(sensors, sensorData, sensorHistory);
    
    // Create a file for download
    const blob = new Blob([csv], { type: 'text/csv' });
    const url = URL.createObjectURL(blob);
    
    // Create and trigger download link
    const link = document.createElement('a');
    link.href = url;
    link.download = `subway_sensor_data_${new Date().toISOString().replace(/[:.]/g, '-')}.csv`;
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
    downloadStoredData,
    lastUpdated,
    storageStats: dataStore.current.getDataStats(),
    clearStorage: () => dataStore.current.clearAllData()
  });
};

const Dashboard = ({ 
  sensors, 
  sensorData, 
  sensorHistory, 
  loading, 
  autoRefresh, 
  refreshAllData, 
  toggleAutoRefresh, 
  downloadSensorData, 
  downloadStoredData,
  lastUpdated,
  storageStats,
  clearStorage
}) => {
  return (
    <div style={{ maxWidth: '1200px', margin: '0 auto', padding: '20px' }}>
      <h1 style={{ textAlign: 'center', color: '#333' }}>Subway Analytics - Sensor Dashboard</h1>
      
      <div style={{ textAlign: 'center', color: '#666', marginBottom: '20px' }}>
        Last Updated: {lastUpdated.toLocaleTimeString()}
      </div>
      
      {/* Storage Stats */}
      <div className="storage-stats" style={{ 
        textAlign: 'center', 
        marginBottom: '20px',
        backgroundColor: '#f0f7ff',
        padding: '10px',
        borderRadius: '8px',
        boxShadow: '0 1px 3px rgba(0,0,0,0.1)'
      }}>
        <div style={{ fontWeight: 'bold', marginBottom: '5px' }}>Browser Storage</div>
        <div style={{ fontSize: '14px' }}>
          Sessions: {storageStats.sessionCount} | 
          Current: {storageStats.currentSession?.substring(0, 8)}... | 
          Total Points: {storageStats.totalDataPoints}
        </div>
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
      </div>
      
      {/* Data Export Buttons */}
      <div className="button-container" style={{ marginBottom: '30px' }}>
        <button 
          onClick={downloadSensorData}
          className="action-button secondary"
        >
          Download Current Data (CSV)
        </button>
        
        <button 
          onClick={downloadStoredData}
          className="action-button primary"
        >
          Download Time-Aligned Data (CSV)
        </button>
        
        <button 
          onClick={() => {
            if (window.confirm('Clear all stored data? This cannot be undone.')) {
              clearStorage();
            }
          }}
          className="action-button warning"
        >
          Clear Stored Data
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

// Wait for the chart component to be available before rendering
// The CustomSensorChart component is defined on the window object by chart.js
setTimeout(() => {
  ReactDOM.render(<App />, document.getElementById("root"));
}, 100);