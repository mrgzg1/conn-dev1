// Configuration for dashboard views
window.DASHBOARD_CONFIG = {
  views: [
    {
      id: "imu_view",
      title: "IMU Sensor Data",
      charts: [
        {
          id: "accel_chart",
          title: "Acceleration",
          type: "line",
          sensor: "imu",
          dataPath: "accel",
          components: ["x", "y", "z"],
          colors: ["#FF6384", "#36A2EB", "#FFCE56"],
          yAxisLabel: "g",
          timeRange: 30
        },
        {
          id: "gyro_chart",
          title: "Gyroscope",
          type: "line",
          sensor: "imu",
          dataPath: "gyro",
          components: ["x", "y", "z"],
          colors: ["#4BC0C0", "#9966FF", "#FF9F40"],
          yAxisLabel: "dps",
          timeRange: 30
        }
      ],
      currentMetrics: [
        {
          label: "Temperature",
          sensor: "imu",
          path: "temperature",
          unit: "°C",
          precision: 1
        },
        {
          label: "Accel X",
          sensor: "imu",
          path: "accel.x",
          unit: "g",
          precision: 3
        },
        {
          label: "Accel Y",
          sensor: "imu",
          path: "accel.y",
          unit: "g",
          precision: 3
        },
        {
          label: "Accel Z",
          sensor: "imu",
          path: "accel.z",
          unit: "g",
          precision: 3
        },
        {
          label: "Gyro X",
          sensor: "imu",
          path: "gyro.x",
          unit: "dps",
          precision: 2
        },
        {
          label: "Gyro Y",
          sensor: "imu",
          path: "gyro.y",
          unit: "dps",
          precision: 2
        },
        {
          label: "Gyro Z",
          sensor: "imu",
          path: "gyro.z",
          unit: "dps",
          precision: 2
        }
      ]
    },
    {
      id: "storage_view",
      title: "Storage Status",
      charts: [],
      custom: "storage_stats"
    }
  ]
};

// Helper function to get nested object values using dot notation
window.getNestedValue = (obj, path) => {
  if (!obj) return null;
  return path.split('.').reduce((prev, curr) => {
    return prev ? prev[curr] : null;
  }, obj);
};

// Custom Sensor Chart component
class CustomSensorChart extends React.Component {
  constructor(props) {
    super(props);
    this.chartRef = React.createRef();
    this.chart = null;
  }
  
  componentDidMount() {
    this.initChart();
  }
  
  componentDidUpdate(prevProps) {
    // Check if data has changed
    if (
      prevProps.sensorData !== this.props.sensorData ||
      prevProps.sensorHistory !== this.props.sensorHistory
    ) {
      this.updateChart();
    }
  }
  
  componentWillUnmount() {
    if (this.chart) {
      this.chart.destroy();
    }
  }
  
  // Initialize Chart.js chart
  initChart() {
    const ctx = this.chartRef.current.getContext('2d');
    const { chart, sensor, dataPath, components, colors, yAxisLabel } = this.props;
    
    // Prepare datasets (one for each component)
    const datasets = components.map((component, index) => ({
      label: component.toUpperCase(),
      backgroundColor: colors[index % colors.length],
      borderColor: colors[index % colors.length],
      data: [],
      fill: false
    }));
    
    // Create Chart.js instance
    this.chart = new Chart(ctx, {
      type: 'line',
      data: {
        labels: [],
        datasets: datasets
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        title: {
          display: true,
          text: chart.title
        },
        tooltips: {
          mode: 'index',
          intersect: false,
        },
        hover: {
          mode: 'nearest',
          intersect: true
        },
        scales: {
          x: {
            display: true,
            title: {
              display: true,
              text: 'Time'
            }
          },
          y: {
            display: true,
            title: {
              display: true,
              text: yAxisLabel || 'Value'
            }
          }
        },
        animation: {
          duration: 0 // Disable animations for better performance
        }
      }
    });
    
    // Initial update
    this.updateChart();
  }
  
  // Update chart with new data
  updateChart() {
    if (!this.chart) return;
    
    const { sensorHistory, sensor, dataPath, components, timeRange } = this.props;
    
    // Get history data for this sensor
    const history = sensorHistory[sensor] || [];
    
    // Limit data points to last 'timeRange' items
    const limitedHistory = timeRange ? history.slice(-timeRange) : history;
    
    // Extract timestamps for x-axis
    const labels = limitedHistory.map((entry, index) => {
      if (entry.timestamp) {
        // Format timestamp if available
        const date = new Date(entry.timestamp);
        return date.toLocaleTimeString();
      }
      return index.toString();
    });
    
    // Update chart data
    this.chart.data.labels = labels;
    
    // Update each dataset (component)
    components.forEach((component, i) => {
      // Extract data for this component
      const data = limitedHistory.map(entry => {
        // Get value at the specified path and component
        const pathValue = getNestedValue(entry, dataPath);
        if (pathValue && typeof pathValue === 'object') {
          return pathValue[component];
        } else if (component === 'value' && pathValue !== undefined) {
          // If the property is a direct value, not an object
          return pathValue;
        }
        return null;
      });
      
      // Update dataset
      this.chart.data.datasets[i].data = data;
    });
    
    // Refresh chart
    this.chart.update();
  }
  
  render() {
    return (
      <div className="chart-container">
        <canvas ref={this.chartRef}></canvas>
      </div>
    );
  }
}

// Custom view panel for each chart group
window.CustomViewPanel = ({ view, sensorData, sensorHistory, refreshAllData, loading }) => {
  // Render storage stats panel
  if (view.custom === "storage_stats") {
    return (
      <div className="sensor-panel">
        <h2 style={{ marginTop: 0 }}>{view.title}</h2>
        
        <div className="storage-stats-container">
          <h3>Flash Storage Status</h3>
          <StorageStats refreshAllData={refreshAllData} />
        </div>
        
        <div className="storage-stats-container">
          <h3>Stored Data</h3>
          <p>Access and download data stored on the device flash memory:</p>
          <div className="button-container">
            <button 
              className="action-button secondary"
              onClick={() => {
                alert('This would download data from device flash storage');
                // In a real implementation, this would fetch from /api/storage/data
              }}
            >
              Download Storage Data (CSV)
            </button>
            <button 
              className="action-button warning"
              onClick={() => {
                if (window.confirm('This will erase all stored data. Are you sure?')) {
                  alert('Storage format would be triggered (not implemented in demo)');
                  // In a real implementation, this would call an API endpoint
                }
              }}
            >
              Format Storage
            </button>
          </div>
        </div>
      </div>
    );
  }
  
  // Standard chart panel
  return (
    <div className="sensor-panel">
      <h2 style={{ marginTop: 0 }}>{view.title}</h2>
      
      {/* Current values */}
      {view.currentMetrics && (
        <div className="current-values-container">
          <h3>Current Values</h3>
          <div className="current-values-grid">
            {view.currentMetrics.map(metric => (
              <div key={metric.label} className="current-value-item">
                <div className="metric-label">{metric.label}</div>
                <div className="metric-value">
                  {(() => {
                    const value = getNestedValue(sensorData[metric.sensor], metric.path);
                    if (value === null || value === undefined) return "N/A";
                    return typeof value === 'number' 
                      ? value.toFixed(metric.precision || 2) + (metric.unit || '') 
                      : value + (metric.unit || '');
                  })()}
                </div>
              </div>
            ))}
          </div>
        </div>
      )}
      
      {/* Charts */}
      {view.charts.map(chart => (
        <CustomSensorChart
          key={chart.id}
          chart={chart}
          sensor={chart.sensor}
          dataPath={chart.dataPath}
          components={chart.components}
          colors={chart.colors}
          yAxisLabel={chart.yAxisLabel}
          timeRange={chart.timeRange}
          sensorData={sensorData}
          sensorHistory={sensorHistory}
        />
      ))}
      
      {/* Loading indicator */}
      {loading[view.charts[0]?.sensor] && (
        <div style={{ textAlign: 'center', padding: '20px', color: '#666' }}>
          Loading data...
        </div>
      )}
    </div>
  );
};

// Storage Stats Component
const StorageStats = ({ refreshAllData }) => {
  const [stats, setStats] = React.useState(null);
  const [loading, setLoading] = React.useState(true);
  
  // Fetch storage stats
  const fetchStats = async () => {
    setLoading(true);
    try {
      const response = await fetch(`${location.origin}/api/storage/stats`);
      if (response.ok) {
        const data = await response.json();
        setStats(data);
      } else {
        console.error("Failed to fetch storage stats");
      }
    } catch (err) {
      console.error("Error fetching storage stats:", err);
    } finally {
      setLoading(false);
    }
  };
  
  // Initial fetch
  React.useEffect(() => {
    fetchStats();
    // Set up periodic refresh
    const interval = setInterval(fetchStats, 10000);
    return () => clearInterval(interval);
  }, []);
  
  if (loading && !stats) {
    return <div>Loading storage information...</div>;
  }
  
  if (!stats) {
    return (
      <div>
        <p>Failed to load storage information</p>
        <button 
          className="action-button secondary"
          onClick={fetchStats}
        >
          Retry
        </button>
      </div>
    );
  }
  
  return (
    <div>
      <div className="storage-stats-grid">
        <div className="storage-stat-item">
          <div className="storage-stat-label">Total Readings</div>
          <div className="storage-stat-value">{stats.readings}</div>
        </div>
        <div className="storage-stat-item">
          <div className="storage-stat-label">Sectors Used</div>
          <div className="storage-stat-value">{stats.sectors.used} / {stats.sectors.total}</div>
        </div>
        <div className="storage-stat-item">
          <div className="storage-stat-label">Storage Used</div>
          <div className="storage-stat-value">
            {Math.round((stats.sectors.used / stats.sectors.total) * 100)}%
          </div>
        </div>
        <div className="storage-stat-item">
          <div className="storage-stat-label">Buffered Items</div>
          <div className="storage-stat-value">
            {stats.buffer?.items || 0} / {stats.buffer?.capacity || 0}
          </div>
        </div>
        <div className="storage-stat-item">
          <div className="storage-stat-label">Boot Count</div>
          <div className="storage-stat-value">{stats.bootCount}</div>
        </div>
      </div>
      
      <div style={{ textAlign: 'center', marginTop: '20px' }}>
        <button 
          className="action-button secondary"
          onClick={fetchStats}
        >
          Refresh Storage Stats
        </button>
      </div>
    </div>
  );
};

// Helper function to convert data to CSV
window.convertToCSV = (sensors, sensorData, sensorHistory) => {
  // Start with headers
  let csv = "timestamp,sensor,";
  
  // First determine all possible data fields from history
  const allFields = new Set();
  
  // Collect all fields from all sensors
  sensors.forEach(sensor => {
    const history = sensorHistory[sensor.endpoint] || [];
    if (history.length > 0) {
      collectFields(history[0], "", allFields);
    }
  });
  
  // Add all fields as headers
  csv += Array.from(allFields).join(",") + "\n";
  
  // Add data rows from history
  sensors.forEach(sensor => {
    const history = sensorHistory[sensor.endpoint] || [];
    history.forEach(entry => {
      // Start with timestamp and sensor
      const timestamp = entry.timestamp || new Date().getTime();
      let row = `${timestamp},${sensor.name},`;
      
      // Add values for each field
      Array.from(allFields).forEach(field => {
        const value = getNestedValue(entry, field);
        if (value !== null && value !== undefined) {
          if (typeof value === 'object') {
            row += JSON.stringify(value).replace(/,/g, ';') + ',';
          } else {
            row += value + ',';
          }
        } else {
          row += ',';
        }
      });
      
      // Remove trailing comma and add newline
      csv += row.slice(0, -1) + "\n";
    });
  });
  
  return csv;
};

// Generate time-aligned CSV from stored data
window.generateTimeAlignedCSV = (timeAlignedData) => {
  if (!timeAlignedData) return null;
  
  // Extract sensor data
  const sensorData = timeAlignedData.data;
  
  // Get all sensor names
  const sensorNames = Object.keys(sensorData);
  if (sensorNames.length === 0) return null;
  
  // Create headers first
  let headers = ["timestamp", "realTimestamp"];
  
  // Add specific headers for each sensor's values
  sensorNames.forEach(sensorName => {
    // Add the sensor prefix to each value
    headers.push(`${sensorName}_accel_x`);
    headers.push(`${sensorName}_accel_y`);
    headers.push(`${sensorName}_accel_z`);
    headers.push(`${sensorName}_gyro_x`);
    headers.push(`${sensorName}_gyro_y`);
    headers.push(`${sensorName}_gyro_z`);
    headers.push(`${sensorName}_temperature`);
  });
  
  // Start CSV with headers
  let csv = headers.join(",") + "\n";
  
  // Collect all timestamps to build aligned rows
  const timestamps = new Set();
  sensorNames.forEach(sensorName => {
    const history = sensorData[sensorName]?.history || [];
    history.forEach(entry => {
      if (entry.timestamp) {
        timestamps.add(entry.timestamp);
      }
    });
  });
  
  // Convert to sorted array
  const sortedTimestamps = Array.from(timestamps).sort((a, b) => a - b);
  
  // Build rows - one for each timestamp
  sortedTimestamps.forEach(timestamp => {
    let row = [timestamp];
    
    // Get real timestamp (use the first sensor that has this timestamp)
    let realTimestamp = "";
    for (const sensorName of sensorNames) {
      const entry = (sensorData[sensorName]?.history || []).find(e => e.timestamp === timestamp);
      if (entry && entry.realTimestamp) {
        realTimestamp = entry.realTimestamp;
        break;
      }
    }
    row.push(realTimestamp);
    
    // Add data for each sensor
    sensorNames.forEach(sensorName => {
      const entry = (sensorData[sensorName]?.history || []).find(e => e.timestamp === timestamp);
      
      // Add sensor values
      if (entry) {
        row.push(entry.accel?.x ?? "");
        row.push(entry.accel?.y ?? "");
        row.push(entry.accel?.z ?? "");
        row.push(entry.gyro?.x ?? "");
        row.push(entry.gyro?.y ?? "");
        row.push(entry.gyro?.z ?? "");
        row.push(entry.temperature ?? "");
      } else {
        // Add empty values if sensor didn't have data at this timestamp
        row.push("", "", "", "", "", "", "");
      }
    });
    
    csv += row.join(",") + "\n";
  });
  
  return csv;
};

// Helper function to collect all fields from an object
function collectFields(obj, prefix, fieldsSet) {
  if (!obj || typeof obj !== 'object') return;
  
  Object.keys(obj).forEach(key => {
    // Skip timestamp field
    if (key === "timestamp") return;
    
    const value = obj[key];
    const newPrefix = prefix ? `${prefix}.${key}` : key;
    
    if (value && typeof value === 'object' && !Array.isArray(value)) {
      // Recurse into nested objects
      collectFields(value, newPrefix, fieldsSet);
    } else {
      // Add leaf field
      fieldsSet.add(newPrefix);
    }
  });
}

// Make the components available on the window for use by other scripts
window.CustomSensorChart = CustomSensorChart;