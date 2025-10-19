const SENSOR_API_URL = '/test/status';
const PUMP_API_URL_BASE = '/test/pump/';
const REFRESH_INTERVAL_MS = 10000; // 10 seconds

// Helper function to update the DOM
function updateDisplay(data) {
    document.getElementById('temp').textContent = (
        data.temperature ? `${data.temperature.toFixed(1)} C` : '--');
    document.getElementById('humidity').textContent = (
        data.humidity ? `${data.humidity.toFixed(0)} %` : '--');
    document.getElementById('reservoir').textContent = (data.reservoir == 'ON') ? "full" : "empty";

    const getval = function(label) {
	const val = data[label];
	return val ? val.toFixed(1) : '--';
    };

    for (let i = 1; i <= 4; i++) {
        document.getElementById(`mstr${i}`).textContent = (
            data[`plant${i}_soil_moisture_raw`] || '--');
        document.getElementById(`mstr_pct${i}`).textContent = getval(`plant${i}_soil_moisture`);
        document.getElementById(`mstr_flt_pct${i}`).textContent =
	    getval(`plant${i}_soil_moisture_filtered`);
    }
}

// Simulates fetching sensor data from an API
async function fetchSensorData() {
    console.log('Fetching sensor data...');
    const statusMessage = document.getElementById('status-message');
    statusMessage.textContent = 'Updating data...';

    // --- API Simulation ---
    // In a real application, you would replace this try/catch block
    // with a real fetch request to SENSOR_API_URL.
    try {
        const response = await fetch(SENSOR_API_URL);
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        // Parse the JSON response body
        const data = await response.json();

        updateDisplay(data);
        statusMessage.textContent = `Data refreshed successfully at ${new Date().toLocaleTimeString()}`;

    } catch (error) {
        console.error('Error fetching sensor data:', error);
        statusMessage.textContent = 'Error fetching data.';
        // If fetching fails, display placeholders
        updateDisplay({});
    }
}

// Handles the water pump pulse request
async function waterPulse(pumpId) {
    const buttonId = `pump${pumpId}-btn`;
    const button = document.getElementById(buttonId);
    const originalText = button.textContent;
    const statusMessage = document.getElementById('status-message');

    //button.disabled = true;
    //button.textContent = 'Pulsing...';
    statusMessage.textContent = `Triggering pump ${pumpId}...`;

    // 2. Perform the fetch request
    fetch(PUMP_API_URL_BASE, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({
                duration: 1000,
                pumpId: parseInt(pumpId)
            })
        })
        .then(response => {
            if (!response.ok) {
                throw new Error(`HTTP error! Status: ${response.status}`);
            }
            return response.json();
        })
        .then(data => {
            const message = data.message || `Pump ${pumpId} on.`;
            statusMessage.textContent = `Status: Success! ${message}`;
            console.log('Server Response:', data);
        })
        .catch(error => {
            // 4. Update status on error
            statusMessage.textContent = `ERROR activating Pump ${pumpId}. See console for details.`;
            console.error('Fetch Error:', error);
        });
}

// --- Initialization ---
document.addEventListener('DOMContentLoaded', () => {
    // Add click listeners to all pump buttons
    document.querySelectorAll('#control-buttons button').forEach(button => {
        button.addEventListener('click', () => {
            const pumpId = button.getAttribute('data-pump-id');
            if (pumpId) {
                waterPulse(pumpId);
            }
        });
    });

    // 1. Fetch data immediately on load
    fetchSensorData();

    // 2. Set up continuous polling every 10 seconds
    setInterval(fetchSensorData, REFRESH_INTERVAL_MS);
});
