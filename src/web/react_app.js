const deviceIP = location.origin;

const App = () => {
  const [sensorData, setSensorData] = React.useState({ reading: 0, leds: { red: false, green: false, blue: false } });
  const [readings, setReadings] = React.useState([]);
  const chartRef = React.useRef(null);

  const toggleLED = async (color, state) => {
    const command = `${color}${state ? 'H' : 'L'}`; // Fixed to send H to turn on, L to turn off
    console.log(`Sending LED command: ${command}`);
    try {
      await fetch(`${deviceIP}/${command}`);
      // Update LED state immediately for better UX
      setSensorData(prev => ({
        ...prev,
        leds: {
          ...prev.leds,
          [color === 'R' ? 'red' : color === 'G' ? 'green' : 'blue']: state
        }
      }));
    } catch (err) {
      console.error('Error toggling LED:', err);
    }
  };

  React.useEffect(() => {
    const fetchData = async () => {
      try {
        const res = await fetch(`${deviceIP}/sensor`);
        const data = await res.json();
        setSensorData(prev => ({
          ...data,
          leds: {
            red: data.leds.red,
            green: data.leds.green, 
            blue: data.leds.blue
          }
        }));
        
        // Update readings array with new sensor value
        setReadings(prev => {
          const newReadings = [...prev, data.reading].slice(-50); // Keep last 50 readings
          return newReadings;
        });
      } catch (err) {
        console.error('Error fetching sensor data:', err);
      }
    };

    fetchData();
    const interval = setInterval(fetchData, 1000); // Poll every second to keep LED states in sync
    return () => clearInterval(interval);
  }, []);

  // Effect for updating chart
  React.useEffect(() => {
    if (chartRef.current && readings.length > 0) {
      const ctx = chartRef.current.getContext('2d');
      if (window.chartInstance) {
        window.chartInstance.destroy();
      }
      window.chartInstance = new Chart(ctx, {
        type: 'line',
        data: {
          labels: readings.map((_, i) => i),
          datasets: [{
            label: 'Sensor Reading',
            data: readings,
            borderColor: 'rgb(75, 192, 192)',
            tension: 0.1
          }]
        },
        options: {
          responsive: true,
          scales: {
            y: {
              beginAtZero: true,
              max: 1024 // Max value for analog read
            }
          }
        }
      });
    }
  }, [readings]);

  return (
    <div>
      <h1>Sensor Dashboard</h1>
      <div style={{ width: '800px', height: '400px' }}>
        <canvas ref={chartRef}></canvas>
      </div>
      <div style={{ marginTop: '20px' }}>
        <button 
          onClick={() => toggleLED('R', !sensorData.leds.red)}
          style={{
            padding: '10px 20px',
            margin: '0 10px',
            backgroundColor: sensorData.leds.red ? 'red' : 'white',
            color: sensorData.leds.red ? 'white' : 'black'
          }}
        >
          Red LED {sensorData.leds.red ? 'ON' : 'OFF'}
        </button>
        <button 
          onClick={() => toggleLED('G', !sensorData.leds.green)}
          style={{
            padding: '10px 20px',
            margin: '0 10px',
            backgroundColor: sensorData.leds.green ? 'green' : 'white',
            color: sensorData.leds.green ? 'white' : 'black'
          }}
        >
          Green LED {sensorData.leds.green ? 'ON' : 'OFF'}
        </button>
        <button 
          onClick={() => toggleLED('B', !sensorData.leds.blue)}
          style={{
            padding: '10px 20px',
            margin: '0 10px',
            backgroundColor: sensorData.leds.blue ? 'blue' : 'white',
            color: sensorData.leds.blue ? 'white' : 'black'
          }}
        >
          Blue LED {sensorData.leds.blue ? 'ON' : 'OFF'}
        </button>
      </div>
      <div style={{ marginTop: '20px' }}>
        Current sensor value: {sensorData.reading}
      </div>
    </div>
  );
};

ReactDOM.render(<App />, document.getElementById("root"));
