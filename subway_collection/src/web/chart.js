// Make the component globally available
window.IMUHistoryChart = ({ history }) => {
  const chartRef = React.useRef(null);
  const chartInstance = React.useRef(null);

  const updateChart = () => {
    if (!chartRef.current) return;
    
    // Destroy existing chart
    if (chartInstance.current) {
      chartInstance.current.destroy();
    }

    // Extract timestamps for x-axis labels
    const labels = history.map(entry => {
      const date = new Date(entry.timestamp);
      return date.toLocaleTimeString();
    });

    // Extract data for each sensor
    const accelXData = history.map(entry => entry.accel.x);
    const accelYData = history.map(entry => entry.accel.y);
    const accelZData = history.map(entry => entry.accel.z);
    const temperatureData = history.map(entry => entry.temperature);

    const ctx = chartRef.current.getContext('2d');
    chartInstance.current = new Chart(ctx, {
      type: 'line',
      data: {
        labels: labels,
        datasets: [
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
        ]
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
        },
        plugins: {
          title: {
            display: true,
            text: 'IMU Sensor Readings'
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
  }, [history]);

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
  