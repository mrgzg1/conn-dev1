const deviceIP = location.origin;

const App = () => {
  const [sequences, setSequences] = React.useState({
    red: Array(50).fill(0),
    green: Array(50).fill(0),
    blue: Array(50).fill(0)
  });
  const [selectedLED, setSelectedLED] = React.useState('red');
  const [isPlaying, setIsPlaying] = React.useState(false);

  // Send PWM value to LED
  const setPWM = async (color, value) => {
    const colorCode = color[0].toUpperCase();
    try {
      await fetch(`${deviceIP}/PWM${colorCode}${value}`);
    } catch (err) {
      console.error('Error setting PWM:', err);
    }
  };

  // Handle sequence updates from the chart
  const handleSequenceUpdate = (color, index, value) => {
    setSequences(prev => ({
      ...prev,
      [color]: prev[color].map((v, i) => i === index ? value : v)
    }));
  };

  // Play the sequence
  const playSequence = async () => {
    if (isPlaying) return;
    
    setIsPlaying(true);
    const duration = 5000; // 5 seconds
    const steps = 50;
    const stepDuration = duration / steps;
    
    try {
      for (let i = 0; i < steps; i++) {
        await setPWM('red', sequences.red[i]);
        await setPWM('green', sequences.green[i]);
        await setPWM('blue', sequences.blue[i]);
        await new Promise(resolve => setTimeout(resolve, stepDuration));
      }
    } catch (err) {
      console.error('Error playing sequence:', err);
    } finally {
      // Turn off all LEDs at the end
      await setPWM('red', 0);
      await setPWM('green', 0);
      await setPWM('blue', 0);
      setIsPlaying(false);
    }
  };

  // Clear all sequences
  const clearSequences = () => {
    setSequences({
      red: Array(50).fill(0),
      green: Array(50).fill(0),
      blue: Array(50).fill(0)
    });
  };

  const LEDButton = ({ color }) => (
    <button 
      onClick={() => setSelectedLED(color)}
      style={{
        padding: '10px 20px',
        margin: '0 10px',
        backgroundColor: selectedLED === color ? color : 'white',
        color: selectedLED === color ? 'white' : 'black',
        border: `2px solid ${color}`,
        borderRadius: '4px',
        cursor: 'pointer',
        transition: 'all 0.3s ease'
      }}
    >
      Edit {color.charAt(0).toUpperCase() + color.slice(1)} LED
    </button>
  );

  return (
    <div style={{ maxWidth: '1000px', margin: '0 auto', padding: '20px' }}>
      <h1 style={{ textAlign: 'center', color: '#333' }}>LED Sequence Editor</h1>
      
      <div style={{ 
        width: '100%', 
        height: '400px', 
        marginBottom: '20px',
        border: '1px solid #ddd',
        borderRadius: '8px',
        padding: '10px'
      }}>
        <LEDSequenceChart 
          sequences={sequences}
          selectedLED={selectedLED}
          onSequenceUpdate={handleSequenceUpdate}
        />
      </div>

      <div style={{ 
        marginBottom: '20px',
        display: 'flex',
        justifyContent: 'center',
        gap: '10px'
      }}>
        <LEDButton color="red" />
        <LEDButton color="green" />
        <LEDButton color="blue" />
      </div>

      <div style={{ 
        display: 'flex',
        justifyContent: 'center',
        gap: '20px'
      }}>
        <button 
          onClick={playSequence}
          disabled={isPlaying}
          style={{
            padding: '12px 24px',
            backgroundColor: isPlaying ? '#999' : '#4CAF50',
            color: 'white',
            fontSize: '16px',
            border: 'none',
            borderRadius: '4px',
            cursor: isPlaying ? 'not-allowed' : 'pointer',
            transition: 'all 0.3s ease'
          }}
        >
          {isPlaying ? 'Playing...' : 'Play Sequence'}
        </button>
        
        <button 
          onClick={clearSequences}
          style={{
            padding: '12px 24px',
            backgroundColor: '#f44336',
            color: 'white',
            fontSize: '16px',
            border: 'none',
            borderRadius: '4px',
            cursor: 'pointer',
            transition: 'all 0.3s ease'
          }}
        >
          Clear All
        </button>
      </div>
    </div>
  );
};

// Wait for the chart component to be available
setTimeout(() => {
  ReactDOM.render(<App />, document.getElementById("root"));
}, 100);
