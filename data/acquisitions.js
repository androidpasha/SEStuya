(async function () {
  const data = await loadAndParseData('log.txt');
  //Chart.register(ChartZoom);
  const decimation = {
    enabled: false,
    algorithm: 'min-max',
  };
  new Chart(
    document.getElementById('acquisitions'),
    {
      options: {
        // maintainAspectRatio: false,
        maintainAspectRatio: false,
        responsiveAnimationDuration: 0,
        animation: false,
        responsive: true,
        // parsing: false,
        interaction: {
          mode: 'nearest',
          axis: 'x',
          intersect: false
        },
        scales: {
          x: {
            type: 'timestack',
          },
        },
        interaction: {
          mode: 'nearest',
          axis: 'x',
          intersect: false
        },
        plugins: {
          decimation: decimation,
          scales: {
            x: {
              type: 'time',
              ticks: {
                source: 'auto',
                // Disabled rotation for performance
                maxRotation: 0,
                autoSkip: true,
              }
            }
          },
          zoom: {
            pan: { // Панорамирование (свайп влево/вправо)
              enabled: true,
              mode: 'x', // можно 'xy' для 2D панорамирования
            },
            zoom: {
              wheel: {
                enabled: true,
              },
              pinch: { //Щепок
                enabled: true
              },
              mode: 'x',
              scaleMode: 'x',
            }
          }
        }
      },
      type: 'line',
      data:
      {
        labels: data.map(row => row.x),
        datasets: [
          {
            label: 'Напруга батареї',
            data: data.map(row => row.y),
            borderColor: "black",
            borderWidth: 1,
            tension: 0.2,
            pointRadius: 0
          }
        ]
      }
    }
  );
})();

async function loadAndParseData(url) {
  const response = await fetch(url);
  if (!response.ok) throw new Error("Ошибка загрузки log.txt");

  const text = await response.text();
  const lines = text.trim().split("\n");

  const data = [];

  for (const line of lines) {
    const match = line.match(/^(\d{2})\.(\d{2})\.(\d{2}) (\d{2}):(\d{2}) Ubat\s*=\s*([\d.]+)/);
    if (!match) continue;

    const [, dd, mm, yy, hh, min, valueStr] = match;

    // Дата в формате "YY-MM-DDTHH:MM"
    const date = new Date(`20${yy}-${mm}-${dd}T${hh}:${min}:00`);
    const timestamp = date.getTime();
    const value = parseFloat(valueStr);

    data.push({ x: timestamp, y: value });
  }

  return data;
}


