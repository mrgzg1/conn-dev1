// Make the component globally available
window.LEDSequenceChart = ({ sequences, selectedLED, onSequenceUpdate }) => {
  const chartRef = React.useRef(null);
  const chartInstance = React.useRef(null);

  const updateChart = () => {
    if (!chartRef.current) return;
    
    // Destroy existing chart
    if (chartInstance.current) {
      chartInstance.current.destroy();
    }

    const ctx = chartRef.current.getContext('2d');
    chartInstance.current = new Chart(ctx, {
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
        maintainAspectRatio: false,
        animation: {
          duration: 0 // Disable animations for smoother drawing
        },
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
            text: 'LED Sequence Editor - Click and Drag to Draw'
          }
        },
        events: ['mousedown', 'mousemove', 'mouseup', 'mouseleave']
      }
    });
  };

  const handleChartInteraction = (event) => {
    if (!chartInstance.current) return;
    
    const rect = chartRef.current.getBoundingClientRect();
    const x = event.clientX - rect.left;
    const y = event.clientY - rect.top;
    
    const xValue = chartInstance.current.scales.x.getValueForPixel(x);
    const yValue = chartInstance.current.scales.y.getValueForPixel(y);
    
    if (xValue >= 0 && xValue < 50) {
      const newValue = Math.min(Math.max(Math.round(yValue), 0), 100);
      onSequenceUpdate(selectedLED, Math.floor(xValue), newValue);
    }
  };

  React.useEffect(() => {
    updateChart();
    return () => {
      if (chartInstance.current) {
        chartInstance.current.destroy();
      }
    };
  }, [sequences, selectedLED]);

  return (
    <canvas 
      ref={chartRef}
      onMouseDown={handleChartInteraction}
      onMouseMove={(e) => {
        if (e.buttons === 1) { // Only update if primary mouse button is pressed
          handleChartInteraction(e);
        }
      }}
    />
  );
};
  