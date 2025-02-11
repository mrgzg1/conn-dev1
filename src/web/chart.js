let chartInstance = null;

const renderChart = (chartRef, data) => {
  const ctx = chartRef.current.getContext("2d");
  
  // Destroy existing chart if it exists
  if (chartInstance) {
    chartInstance.destroy();
  }

  chartInstance = new Chart(ctx, {
    type: "line",
    data: {
      labels: Array.from({ length: data.length }, (_, i) => i),
      datasets: [
        { 
          label: "Sensor Readings",
          data: data,
          borderColor: "blue",
          borderWidth: 2,
          fill: false
        }
      ],
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      scales: {
        x: { 
          title: { display: true, text: "Time" }
        },
        y: { 
          title: { display: true, text: "Value" },
          beginAtZero: true
        }
      },
    },
  });
};
  