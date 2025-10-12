<script>
  import { Save } from 'lucide-svelte';

  // API base URL
  const API_BASE = '/api';

  export let wifi;

  let wifiConfig;
  let saving = false;
  let saveMessage = '';

  $: wifiConfig = $wifi;

  function updateWifi(field, value) {
    wifi.update(w => {
      w[field] = value;
      return w;
    });
  }

  async function saveWifiConfig() {
    saving = true;
    saveMessage = '';

    try {
      const response = await fetch(`${API_BASE}/wifi`, {
        method: 'PUT',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify(wifiConfig)
      });

      if (!response.ok) {
        throw new Error('Failed to save WiFi configuration');
      }

      saveMessage = 'WiFi configuration saved successfully!';
      setTimeout(() => saveMessage = '', 3000);
    } catch (err) {
      console.error('Error saving WiFi config:', err);
      saveMessage = `Error: ${err.message}`;
    } finally {
      saving = false;
    }
  }
</script>

<div class="page">
  <h2 class="page-title">WiFi Configuration</h2>

  <div class="card">
    <div class="form-group">
      <label class="form-label">Network SSID</label>
      <input
        type="text"
        class="form-input"
        placeholder="Enter WiFi network name"
        bind:value={wifiConfig.essid}
        on:change={() => updateWifi('essid', wifiConfig.essid)}
      />
    </div>

    <div class="form-group">
      <label class="form-label">Password</label>
      <input
        type="password"
        class="form-input"
        placeholder="Enter WiFi password"
        bind:value={wifiConfig.password}
        on:change={() => updateWifi('password', wifiConfig.password)}
      />
    </div>

    <div class="form-group">
      <label class="form-label">Device Hostname</label>
      <input
        type="text"
        class="form-input"
        placeholder="plant-waterer"
        bind:value={wifiConfig.board}
        on:change={() => updateWifi('board', wifiConfig.board)}
      />
      <div class="form-hint">Network name for this device</div>
    </div>

    <button class="btn btn-blue" on:click={saveWifiConfig} disabled={saving}>
      <Save size={20} />
      {saving ? 'Saving...' : 'Save WiFi Configuration'}
    </button>

    {#if saveMessage}
      <div class="save-message" class:error={saveMessage.startsWith('Error')}>
        {saveMessage}
      </div>
    {/if}
  </div>
</div>

<style>
  .page-title {
    font-size: 2rem;
    font-weight: 700;
    color: #1f2937;
    margin-bottom: 1.5rem;
  }

  .card {
    background: white;
    padding: 1.5rem;
    border-radius: 0.5rem;
    box-shadow: 0 1px 3px rgba(0,0,0,0.1);
  }

  .form-group {
    margin-bottom: 1.5rem;
  }

  .form-label {
    display: block;
    font-size: 0.875rem;
    font-weight: 500;
    color: #374151;
    margin-bottom: 0.5rem;
  }

  .form-input {
    width: 100%;
    padding: 0.5rem 1rem;
    border: 1px solid #d1d5db;
    border-radius: 0.5rem;
    font-size: 1rem;
  }

  .form-input:focus {
    outline: none;
    border-color: #2563eb;
    box-shadow: 0 0 0 3px rgba(37, 99, 235, 0.1);
  }

  .form-hint {
    font-size: 0.75rem;
    color: #6b7280;
    margin-top: 0.25rem;
  }

  .btn {
    display: inline-flex;
    align-items: center;
    gap: 0.5rem;
    padding: 0.75rem 1.5rem;
    border: none;
    border-radius: 0.5rem;
    font-size: 1rem;
    font-weight: 500;
    cursor: pointer;
    transition: background 0.2s;
  }

  .btn-blue {
    background: #2563eb;
    color: white;
  }

  .btn-blue:hover {
    background: #1d4ed8;
  }

  .btn-blue:disabled {
    background: #9ca3af;
    cursor: not-allowed;
  }

  .save-message {
    margin-top: 1rem;
    padding: 0.75rem 1rem;
    border-radius: 0.5rem;
    background: #dbeafe;
    color: #1e40af;
    font-size: 0.875rem;
    font-weight: 500;
  }

  .save-message.error {
    background: #fee2e2;
    color: #991b1b;
  }
</style>
