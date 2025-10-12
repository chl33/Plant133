<script>
  import { writable } from 'svelte/store';
  import { onMount } from 'svelte';
  import Navbar from './components/Navbar.svelte';
  import Sidebar from './components/Sidebar.svelte';
  import HomePage from './pages/HomePage.svelte';
  import PlantConfigPage from './pages/PlantConfigPage.svelte';
  import WiFiConfigPage from './pages/WiFiConfigPage.svelte';
  import MQTTConfigPage from './pages/MQTTConfigPage.svelte';

  // API base URL - configure this to match your device's address
  const API_BASE = '/api'; // Change to http://your-device-ip/api if needed

  let currentPage = 'home';
  let loading = true;
  let error = null;

  export let plants = writable([]);

  export let wifi = writable({
    essid: '',
    password: '',
    board: 'plant133'
  });

  export let mqtt = writable({
    broker: '',
    port: 1883,
    username: '',
    password: '',
  });

  // Load plant configurations from server
  async function loadPlantConfigs() {
    try {
      const response = await fetch(`${API_BASE}/plants`);
      if (!response.ok) throw new Error('Failed to load plant configurations');
      const data = await response.json();
      plants.set(data);
    } catch (err) {
      console.error('Error loading plant configs:', err);
      error = err.message;
      // Use default data if server unavailable
      plants.set([
        { id: 1, name: 'Plant 1', minMoisture: 30, maxMoisture: 70, adc0: 0, adc100: 1023, enabled: true, currentMoisture: 0 },
        { id: 2, name: 'Plant 2', minMoisture: 30, maxMoisture: 70, adc0: 0, adc100: 1023, enabled: true, currentMoisture: 0 },
        { id: 3, name: 'Plant 3', minMoisture: 30, maxMoisture: 70, adc0: 0, adc100: 1023, enabled: false, currentMoisture: 0 },
        { id: 4, name: 'Plant 4', minMoisture: 30, maxMoisture: 70, adc0: 0, adc100: 1023, enabled: true, currentMoisture: 0 }
      ]);
    }
  }

  // Load moisture levels from server
  async function loadMoistureLevels() {
    try {
      const response = await fetch(`${API_BASE}/moisture`);
      if (!response.ok) throw new Error('Failed to load moisture levels');
      const data = await response.json();

      plants.update(p => {
        return p.map(plant => {
          const moistureData = data.find(m => m.id === plant.id);
          if (moistureData) {
            return { ...plant, currentMoisture: moistureData.moisture };
          }
          return plant;
        });
      });
    } catch (err) {
      console.error('Error loading moisture levels:', err);
    }
  }

  // Load WiFi config from server
  async function loadWiFiConfig() {
    try {
      const response = await fetch(`${API_BASE}/wifi`);
      if (!response.ok) throw new Error('Failed to load WiFi config');
      const data = await response.json();
      wifi.set(data);
    } catch (err) {
      console.error('Error loading WiFi config:', err);
    }
  }

  // Load MQTT config from server
  async function loadMQTTConfig() {
    try {
      const response = await fetch(`${API_BASE}/mqtt`);
      if (!response.ok) throw new Error('Failed to load MQTT config');
      const data = await response.json();
      mqtt.set(data);
    } catch (err) {
      console.error('Error loading MQTT config:', err);
    }
  }

  // Initialize data on mount
  onMount(async () => {
    loading = true;
    await Promise.all([
      loadPlantConfigs(),
      loadMoistureLevels(),
      loadWiFiConfig(),
      loadMQTTConfig()
    ]);
    loading = false;

    // Poll moisture levels every 5 seconds
    const moistureInterval = setInterval(loadMoistureLevels, 5000);

    // Cleanup interval on component destroy
    return () => clearInterval(moistureInterval);
  });

  function changePage(page) {
    currentPage = page;
  }
</script>

<div class="app-container">
  <Navbar />
  {#if loading}
    <div class="loading-container">
      <div class="spinner"></div>
      <p>Loading configuration...</p>
    </div>
  {:else if error}
    <div class="error-container">
      <p class="error-message">⚠️ {error}</p>
      <p class="error-hint">Using default configuration. Check device connection.</p>
    </div>
  {/if}
  <div class="main-layout" class:hidden={loading}>
    <Sidebar {currentPage} {plants} on:changePage={(e) => changePage(e.detail)} />
    <main class="content">
      <div class="content-inner">
        {#if currentPage === 'home'}
          <HomePage {plants} {wifi} {mqtt} on:changePage={(e) => changePage(e.detail)} />
        {:else if currentPage === 'wifi'}
          <WiFiConfigPage {wifi} />
        {:else if currentPage === 'mqtt'}
          <MQTTConfigPage {mqtt} />
        {:else if currentPage.startsWith('plant')}
          <PlantConfigPage plantId={parseInt(currentPage.replace('plant', ''))} {plants} />
        {/if}
      </div>
    </main>
  </div>
</div>

<style>
  :global(*) {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
  }

  :global(body) {
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
    background: #f3f4f6;
  }

  .app-container {
    min-height: 100vh;
    display: flex;
    flex-direction: column;
  }

  .main-layout {
    display: flex;
    flex: 1;
  }

  .content {
    flex: 1;
    padding: 2rem;
  }

  .content-inner {
    max-width: 1000px;
  }

  .loading-container {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    min-height: 400px;
    gap: 1rem;
  }

  .spinner {
    width: 48px;
    height: 48px;
    border: 4px solid #e5e7eb;
    border-top-color: #059669;
    border-radius: 50%;
    animation: spin 1s linear infinite;
  }

  @keyframes spin {
    to { transform: rotate(360deg); }
  }

  .loading-container p {
    color: #6b7280;
    font-size: 1rem;
  }

  .error-container {
    background: #fef2f2;
    border: 1px solid #fecaca;
    border-radius: 0.5rem;
    padding: 1.5rem;
    margin: 2rem;
    text-align: center;
  }

  .error-message {
    color: #991b1b;
    font-weight: 600;
    margin-bottom: 0.5rem;
  }

  .error-hint {
    color: #6b7280;
    font-size: 0.875rem;
  }

  .hidden {
    display: none;
  }
</style>
