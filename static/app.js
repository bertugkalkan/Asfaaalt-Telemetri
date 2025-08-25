const socket = io();

const statusEl = document.getElementById('status-indicator');
const kpiSpeed = document.getElementById('kpi-speed');
const kpiTBat = document.getElementById('kpi-tbat');
const kpiVBat = document.getElementById('kpi-vbat');
const kpiEnergy = document.getElementById('kpi-energy');

let lastDataTs = 0;

const chartOptions = {
	responsive: true,
	maintainAspectRatio: false,
	scales: {
		x: { grid: { color: 'rgba(148,163,184,0.1)' }, ticks: { color: '#cbd5e1' } },
		y: { grid: { color: 'rgba(148,163,184,0.1)' }, ticks: { color: '#cbd5e1' } },
	},
	plugins: { legend: { labels: { color: '#e5e7eb' } } }
};

function createLineChart(canvasId, label, color){
	const ctx = document.getElementById(canvasId).getContext('2d');
	return new Chart(ctx, {
		type: 'line',
		data: { labels: [], datasets: [{ label, data: [], borderColor: color, tension: 0.2, pointRadius: 0 }] },
		options: chartOptions
	});
}

const chartSpeed = createLineChart('chart-speed', 'Hız (km/h)', '#60a5fa');
const chartTemp = createLineChart('chart-temp', 'T Batarya (°C)', '#f87171');
const chartVolt = createLineChart('chart-voltage', 'V Batarya (V)', '#34d399');
const chartEnergy = createLineChart('chart-energy', 'Kalan Enerji (Wh)', '#fbbf24');

function fmt(n, digits=1){
	if(n === undefined || n === null || Number.isNaN(n)) return '-';
	return Number(n).toFixed(digits);
}

function addPoint(chart, label, value){
	const maxPoints = 300; // keep recent window
	chart.data.labels.push(label);
	chart.data.datasets[0].data.push(value);
	if(chart.data.labels.length > maxPoints){
		chart.data.labels.shift();
		chart.data.datasets[0].data.shift();
	}
	chart.update('none');
}

function setStatus(connected){
	if(connected){
		statusEl.textContent = 'Bağlı';
		statusEl.style.background = '#064e3b';
	}else{
		statusEl.textContent = 'Bağlantı yok';
		statusEl.style.background = '#7f1d1d';
	}
}

socket.on('connect', () => {
	setStatus(false);
});

socket.on('history', (items) => {
	items.forEach(t => {
		const label = (t.milliseconds/1000).toFixed(0)+'s';
		addPoint(chartSpeed, label, t.speed_kmh);
		addPoint(chartTemp, label, t.temp_batt_c);
		addPoint(chartVolt, label, t.voltage_batt_v);
		addPoint(chartEnergy, label, t.energy_remaining_wh);
		kpiSpeed.textContent = fmt(t.speed_kmh, 1);
		kpiTBat.textContent = fmt(t.temp_batt_c, 1);
		kpiVBat.textContent = fmt(t.voltage_batt_v, 2);
		kpiEnergy.textContent = fmt(t.energy_remaining_wh, 0);
		lastDataTs = Date.now();
	});
});

socket.on('telemetry', (t) => {
	const label = (t.milliseconds/1000).toFixed(0)+'s';
	addPoint(chartSpeed, label, t.speed_kmh);
	addPoint(chartTemp, label, t.temp_batt_c);
	addPoint(chartVolt, label, t.voltage_batt_v);
	addPoint(chartEnergy, label, t.energy_remaining_wh);
	kpiSpeed.textContent = fmt(t.speed_kmh, 1);
	kpiTBat.textContent = fmt(t.temp_batt_c, 1);
	kpiVBat.textContent = fmt(t.voltage_batt_v, 2);
	kpiEnergy.textContent = fmt(t.energy_remaining_wh, 0);
	lastDataTs = Date.now();
});

socket.on('status', (s) => {
	setStatus(!!s.connected);
});

setInterval(() => {
	const connected = (Date.now() - lastDataTs) < 5000;
	setStatus(connected);
}, 1000);