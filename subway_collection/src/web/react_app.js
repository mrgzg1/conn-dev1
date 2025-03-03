const deviceIP = location.origin;

const App = () => {
  const [imuData, setImuData] = React.useState({
    timestamp: 0,
    accel: { x: 0, y: 0, z: 0 },
    gyro: { x: 0, y: 0, z: 0 },
    temperature: 0
  });
  
  const [imuHistory, setImuHistory] = React.useState([]);
  const [loading, setLoading] = React.useState(false);
  const [autoRefresh, setAutoRefresh] = React.useState(false);
  const refreshIntervalRef = React.useRef(null);

  // Fetch current IMU data
  const fetchIMUData = async () => {
    try {
      const response = await fetch(`${deviceIP}/imu_data`);
      const data = await response.json();
      setImuData(data);
    } catch (err) {
      console.error('Error fetching IMU data:', err);
    }
  };

  // Fetch IMU history data
  const fetchIMUHistory = async () => {
    setLoading(true);
    try {
      const response = await fetch(`${deviceIP}/imu_history`);
      const data = await response.json();
      setImuHistory(data);
    } catch (err) {
      console.error('Error fetching IMU history:', err);
    } finally {
      setLoading(false);
    }
  };

  // Toggle auto-refresh of data
  const toggleAutoRefresh = () => {
    const newState = !autoRefresh;
    setAutoRefresh(newState);
    
    if (newState) {
      // Start auto-refresh
      refreshIntervalRef.current = setInterval(() => {
        fetchIMUData();
        fetchIMUHistory();
      }, 1000); // Refresh every second
    } else {
      // Stop auto-refresh
      clearInterval(refreshIntervalRef.current);
    }
  };

  // Initial data fetch
  React.useEffect(() => {
    fetchIMUData();
    fetchIMUHistory();
    
    return () => {
      if (refreshIntervalRef.current) {
        clearInterval(refreshIntervalRef.current);
      }
    };
  }, []);

  return (
    <div style={{ maxWidth: '1000px', margin: '0 auto', padding: '20px' }}>
      <h1 style={{ textAlign: 'center', color: '#333' }}>Subway Analytics - IMU Data</h1>
      
      <div style={{ 
        display: 'grid',
        gridTemplateColumns: '1fr 1fr',
        gap: '20px',
        marginBottom: '20px'
      }}>
        <div style={{ 
          padding: '15px',
          backgroundColor: 'white',
          borderRadius: '8px',
          boxShadow: '0 2px 4px rgba(0,0,0,0.1)'
        }}>
          <h2 style={{ margin: '0 0 15px 0', color: '#444' }}>Current Readings</h2>
          <div style={{ marginBottom: '10px' }}>
            <strong>Temperature:</strong> {imuData.temperature}°C
          </div>
          <div style={{ marginBottom: '10px' }}>
            <strong>Accelerometer:</strong><br />
            X: {imuData.accel.x.toFixed(3)} g<br />
            Y: {imuData.accel.y.toFixed(3)} g<br />
            Z: {imuData.accel.z.toFixed(3)} g
          </div>
          <div>
            <strong>Gyroscope:</strong><br />
            X: {imuData.gyro.x.toFixed(3)} dps<br />
            Y: {imuData.gyro.y.toFixed(3)} dps<br />
            Z: {imuData.gyro.z.toFixed(3)} dps
          </div>
        </div>
        
        <div style={{ 
          padding: '15px',
          backgroundColor: 'white',
          borderRadius: '8px',
          boxShadow: '0 2px 4px rgba(0,0,0,0.1)'
        }}>
          <h2 style={{ margin: '0 0 15px 0', color: '#444' }}>Data Statistics</h2>
          {imuHistory.length > 0 ? (
            <>
              <div style={{ marginBottom: '10px' }}>
                <strong>Samples:</strong> {imuHistory.length}
              </div>
              <div style={{ marginBottom: '10px' }}>
                <strong>Avg. Temperature:</strong> {(imuHistory.reduce((sum, item) => sum + item.temperature, 0) / imuHistory.length).toFixed(1)}°C
              </div>
              <div>
                <strong>Max Acceleration:</strong> {Math.max(...imuHistory.map(item => 
                  Math.sqrt(item.accel.x**2 + item.accel.y**2 + item.accel.z**2)
                )).toFixed(3)} g
              </div>
            </>
          ) : (
            <div>No historical data available yet</div>
          )}
        </div>
      </div>
      
      <div style={{ 
        height: '400px', 
        marginBottom: '20px',
        backgroundColor: 'white',
        borderRadius: '8px',
        padding: '15px',
        boxShadow: '0 2px 4px rgba(0,0,0,0.1)'
      }}>
        <IMUHistoryChart history={imuHistory} />
      </div>
      
      <div style={{ 
        display: 'flex',
        justifyContent: 'center',
        gap: '15px',
        marginBottom: '20px'
      }}>
        <button 
          onClick={fetchIMUData}
          style={{
            padding: '10px 20px',
            backgroundColor: '#4CAF50',
            color: 'white',
            fontSize: '16px',
            border: 'none',
            borderRadius: '4px',
            cursor: 'pointer'
          }}
        >
          Refresh Current Data
        </button>
        
        <button 
          onClick={fetchIMUHistory}
          disabled={loading}
          style={{
            padding: '10px 20px',
            backgroundColor: loading ? '#cccccc' : '#2196F3',
            color: 'white',
            fontSize: '16px',
            border: 'none',
            borderRadius: '4px',
            cursor: loading ? 'not-allowed' : 'pointer'
          }}
        >
          {loading ? 'Loading...' : 'Refresh History'}
        </button>
        
        <button 
          onClick={toggleAutoRefresh}
          style={{
            padding: '10px 20px',
            backgroundColor: autoRefresh ? '#f44336' : '#4CAF50',
            color: 'white',
            fontSize: '16px',
            border: 'none',
            borderRadius: '4px',
            cursor: 'pointer'
          }}
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
