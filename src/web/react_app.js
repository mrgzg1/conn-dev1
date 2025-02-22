const deviceIP = location.origin;

const App = () => {
  const [sensorData, setSensorData] = React.useState({ reading: 0, leds: { red: 0, green: 0, blue: 0 } });
  const [readings, setReadings] = React.useState([]);
  const [selectedLED, setSelectedLED] = React.useState('red');
  const [isDrawing, setIsDrawing] = React.useState(false);
  const [sequences, setSequences] = React.useState({
    red: Array(50).fill(0),
    green: Array(50).fill(0),
    blue: Array(50).fill(0)
  });
  
  const sequenceChartRef = React.useRef(null);
  const chartRef = React.useRef(null);
  let sequenceChart = null;

  // Send PWM value to LED
  const setPWM = async (color, value) => {
    const colorCode = color[0].toUpperCase(); // Get first letter capitalized
    try {
      await fetch(`${deviceIP}/PWM${colorCode}${value}`);
      setSensorData(prev => ({
        ...prev,
        leds: {
          ...prev.leds,
          [color]: value
        }
      }));
    } catch (err) {
      console.error('Error setting PWM:', err);
    }
  };

  // Handle drawing on the sequence chart
  const handleChartClick = (event) => {
    if (!sequenceChart) return;
    
    const rect = sequenceChartRef.current.getBoundingClientRect();
    const x = event.clientX - rect.left;
    const y = event.clientY - rect.top;
    
    const xValue = sequenceChart.scales.x.getValueForPixel(x);
    const yValue = sequenceChart.scales.y.getValueForPixel(y);
    
    if (xValue >= 0 && xValue < 50) {
      const newSequences = { ...sequences };
      newSequences[selectedLED][Math.floor(xValue)] = Math.min(Math.max(Math.round(yValue), 0), 100);
      setSequences(newSequences);
      updateSequenceChart();
    }
  };

  // Play the sequence
  const playSequence = async () => {
    const duration = 5000; // 5 seconds
    const steps = 50;
    const stepDuration = duration / steps;
    
    for (let i = 0; i < steps; i++) {
      await setPWM('red', sequences.red[i]);
      await setPWM('green', sequences.green[i]);
      await setPWM('blue', sequences.blue[i]);
      await new Promise(resolve => setTimeout(resolve, stepDuration));
    }
    
    // Turn off all LEDs at the end
    await setPWM('red', 0);
    await setPWM('green', 0);
    await setPWM('blue', 0);
  };

  // Update sequence chart
  const updateSequenceChart = () => {
    if (!sequenceChartRef.current) return;
    
    if (sequenceChart) {
      sequenceChart.destroy();
    }

    const ctx = sequenceChartRef.current.getContext('2d');
    sequenceChart = new Chart(ctx, {
      type: 'line',
      data: {
        labels: Array.from({length: 50}, (_, i) => (i / 10).toFixed(1)),
        datasets: [
          {
            label: 'Red LED',
            data: sequences.red,
            borderColor: 'rgba(255, 0, 0, 0.8)',
            backgroundColor: 'rgba(255, 0, 0, 0.1)',
            borderWidth: selectedLED === 'red' ? 3 : 1,
            tension: 0.1,
            fill: true
          },
          {
            label: 'Green LED',
            data: sequences.green,
            borderColor: 'rgba(0, 255, 0, 0.8)',
            backgroundColor: 'rgba(0, 255, 0, 0.1)',
            borderWidth: selectedLED === 'green' ? 3 : 1,
            tension: 0.1,
            fill: true
          },
          {
            label: 'Blue LED',
            data: sequences.blue,
            borderColor: 'rgba(0, 0, 255, 0.8)',
            backgroundColor: 'rgba(0, 0, 255, 0.1)',
            borderWidth: selectedLED === 'blue' ? 3 : 1,
            tension: 0.1,
            fill: true
          }
        ]
      },
      options: {
        responsive: true,
        scales: {
          y: {
            beginAtZero: true,
            max: 100,
            title: {
              display: true,
              text: 'LED Power (%)'
            }
          },
          x: {
            title: {
              display: true,
              text: 'Time (seconds)'
            }
          }
        },
        plugins: {
          title: {
            display: true,
            text: 'LED Sequence Editor - Click to Draw'
          }
        }
      }
    });
  };

  // Initialize sequence chart
  React.useEffect(() => {
    updateSequenceChart();
  }, [selectedLED]);

  // Handle mouse events for drawing
  React.useEffect(() => {
    const canvas = sequenceChartRef.current;
    if (!canvas) return;

    const handleMouseDown = (e) => {
      setIsDrawing(true);
      handleChartClick(e);
    };

    const handleMouseMove = (e) => {
      if (isDrawing) {
        handleChartClick(e);
      }
    };

    const handleMouseUp = () => {
      setIsDrawing(false);
    };

    canvas.addEventListener('mousedown', handleMouseDown);
    canvas.addEventListener('mousemove', handleMouseMove);
    canvas.addEventListener('mouseup', handleMouseUp);
    canvas.addEventListener('mouseleave', handleMouseUp);

    return () => {
      canvas.removeEventListener('mousedown', handleMouseDown);
      canvas.removeEventListener('mousemove', handleMouseMove);
      canvas.removeEventListener('mouseup', handleMouseUp);
      canvas.removeEventListener('mouseleave', handleMouseUp);
    };
  }, [isDrawing, selectedLED]);

  return (
    <div>
      <h1>LED Sequence Editor</h1>
      <div style={{ width: '800px', height: '400px', marginBottom: '20px' }}>
        <canvas ref={sequenceChartRef}></canvas>
      </div>
      <div style={{ marginBottom: '20px' }}>
        <button 
          onClick={() => setSelectedLED('red')}
          style={{
            padding: '10px 20px',
            margin: '0 10px',
            backgroundColor: selectedLED === 'red' ? 'red' : 'white',
            color: selectedLED === 'red' ? 'white' : 'black'
          }}
        >
          Edit Red LED
        </button>
        <button 
          onClick={() => setSelectedLED('green')}
          style={{
            padding: '10px 20px',
            margin: '0 10px',
            backgroundColor: selectedLED === 'green' ? 'green' : 'white',
            color: selectedLED === 'green' ? 'white' : 'black'
          }}
        >
          Edit Green LED
        </button>
        <button 
          onClick={() => setSelectedLED('blue')}
          style={{
            padding: '10px 20px',
            margin: '0 10px',
            backgroundColor: selectedLED === 'blue' ? 'blue' : 'white',
            color: selectedLED === 'blue' ? 'white' : 'black'
          }}
        >
          Edit Blue LED
        </button>
      </div>
      <div style={{ marginBottom: '20px' }}>
        <button 
          onClick={playSequence}
          style={{
            padding: '10px 20px',
            margin: '0 10px',
            backgroundColor: '#4CAF50',
            color: 'white',
            fontSize: '16px'
          }}
        >
          Play Sequence
        </button>
        <button 
          onClick={() => {
            setSequences({
              red: Array(50).fill(0),
              green: Array(50).fill(0),
              blue: Array(50).fill(0)
            });
            updateSequenceChart();
          }}
          style={{
            padding: '10px 20px',
            margin: '0 10px',
            backgroundColor: '#f44336',
            color: 'white',
            fontSize: '16px'
          }}
        >
          Clear All
        </button>
      </div>
    </div>
  );
};

ReactDOM.render(<App />, document.getElementById("root"));
