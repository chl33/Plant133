<script>
  import { Save } from 'lucide-svelte';
  import MoistureGauge from '../components/MoistureGauge.svelte';

  // API base URL
  const API_BASE = '/api';

  export let plantId;
  export let plants;

  let plant;
  let saving = false;
  let saveMessage = '';

  $: plant = $plants.find(p => p.id === plantId);

  function updatePlant(field, value) {
    plants.update(p => {
      const index = p.findIndex(pl => pl.id === plantId);
      if (index !== -1) {
        p[index][field] = value;
      }
      return p;
    });
  }

  async function savePlantConfig() {
    saving = true;
    saveMessage = '';

    try {
      const response = await fetch(`${API_BASE}/plants/${plantId}`, {
        method: 'PUT',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          name: plant.name,
          minMoisture: plant.minMoisture,
          maxMoisture: plant.maxMoisture,
          adc0: plant.adc0,
          adc100: plant.adc100,
          enabled: plant.enabled,
	  pumpOnTime: plant.pumpOnTime,
	  secsBetweenDoses: plant.secsBetweenDoses,
	  maxDosesPerCycle: plant.maxDosesPerCycle
        })
      });

      if (!response.ok) {
        throw new Error('Failed to save configuration');
      }

      saveMessage = `${plant.name} configuration saved successfully!`;
      setTimeout(() => saveMessage = '', 3000);
    } catch (err) {
      console.error('Error saving plant config:', err);
      saveMessage = `Error: ${err.message}`;
    } finally {
      saving = false;
    }
  }
</script>

<div class="page">
  <h2 class="page-title">{plant.name} Configuration</h2>

  <div class="card">
    <div class="form-group">
      <label class="form-label">Plant Name</label>
      <input
        type="text"
        class="form-input"
        bind:value={plant.name}
        on:change={() => updatePlant('name', plant.name)}
      />
    </div>

    <div class="form-group">
      <label class="toggle-container">
        <input
          type="checkbox"
          class="toggle-input"
          bind:checked={plant.enabled}
          on:change={() => updatePlant('enabled', plant.enabled)}
        />
        <span class="toggle-slider"></span>
        <span class="toggle-label">Enable Automatic Watering</span>
      </label>
      <div class="form-hint">When disabled, this plant will not be watered automatically</div>
    </div>

    <div class="current-status">
      <h3 class="section-title">Current Status</h3>
      <div class="gauge-large">
        <MoistureGauge
          value={plant.currentMoisture}
          min={plant.minMoisture}
          max={plant.maxMoisture}
          size="large"
        />
      </div>
      <div class="status-text">
        Current Moisture Level: <strong>{plant.currentMoisture}%</strong>
      </div>
    </div>

    <div class="form-row">
      <div class="form-group">
        <label class="form-label">Minimum Moisture Level (%)</label>
        <input
          type="number"
          class="form-input"
          min="0"
          max="100"
          bind:value={plant.minMoisture}
          on:change={() => updatePlant('minMoisture', plant.minMoisture)}
        />
        <div class="form-hint">Water will be activated below this level</div>
      </div>

      <div class="form-group">
        <label class="form-label">Maximum Moisture Level (%)</label>
        <input
          type="number"
          class="form-input"
          min="0"
          max="100"
          bind:value={plant.maxMoisture}
          on:change={() => updatePlant('maxMoisture', plant.maxMoisture)}
        />
        <div class="form-hint">Water will stop above this level</div>
      </div>
    </div>

    <div class="section-divider">
      <h3 class="section-title">ADC Calibration</h3>

      <div class="form-row">
        <div class="form-group">
          <label class="form-label">ADC Counts at 0% Moisture (Dry)</label>
          <input
            type="number"
            class="form-input"
            min="0"
            max="4095"
            bind:value={plant.adc0}
            on:change={() => updatePlant('adc0', plant.adc0)}
          />
          <div class="form-hint">ADC reading when sensor is completely dry</div>
        </div>

        <div class="form-group">
          <label class="form-label">ADC Counts at 100% Moisture (Wet)</label>
          <input
            type="number"
            class="form-input"
            min="0"
            max="4095"
            bind:value={plant.adc100}
            on:change={() => updatePlant('adc100', plant.adc100)}
          />
          <div class="form-hint">ADC reading when sensor is in water</div>
        </div>
      </div>
    </div>

    <div class="section-divider">
      <h3 class="section-title">Pump configuration</h3>

      <div class="form-row">
        <div class="form-group">
          <label class="form-label">Pump dose length (msec)</label>
          <input
            type="number"
            class="form-input"
            min="100"
            max="5000"
            bind:value={plant.pumpOnTime}
            on:change={() => updatePlant('pumpOnTime', plant.pumpOnTime)}
          />
          <div class="form-hint">How the pump should run per dose.</div>
        </div>

        <div class="form-group">
          <label class="form-label">Seconds between doses</label>
          <input
            type="number"
            class="form-input"
            min="10"
            max="7200"
            bind:value={plant.adc100}
            on:change={() => updatePlant('secsBetweenDoses', plant.secsBetweenDoses)}
          />
          <div class="form-hint">Time to wait between pump doses</div>
        </div>

        <div class="form-group">
          <label class="form-label">Maximum doses per cycle</label>
          <input
            type="number"
            class="form-input"
            min="1"
            max="10"
            bind:value={plant.maxDosesPerCycle}
            on:change={() => updatePlant('maxDosesPerCycle', plant.maxDosesPerCycle)}
          />
          <div class="form-hint">Maximum doses while watering and per day</div>
        </div>
      </div>
    </div>

    <button class="btn btn-primary" on:click={savePlantConfig} disabled={saving}>
      <Save size={20} />
      {saving ? 'Saving...' : 'Save Configuration'}
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
    border-color: #059669;
    box-shadow: 0 0 0 3px rgba(5, 150, 105, 0.1);
  }

  .form-hint {
    font-size: 0.75rem;
    color: #6b7280;
    margin-top: 0.25rem;
  }

  .form-row {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
    gap: 1.5rem;
  }

  .toggle-container {
    display: flex;
    align-items: center;
    gap: 0.75rem;
    cursor: pointer;
    user-select: none;
  }

  .toggle-input {
    position: absolute;
    opacity: 0;
    width: 0;
    height: 0;
  }

  .toggle-slider {
    position: relative;
    display: inline-block;
    width: 48px;
    height: 24px;
    background: #cbd5e1;
    border-radius: 24px;
    transition: background 0.3s;
  }

  .toggle-slider::after {
    content: '';
    position: absolute;
    top: 2px;
    left: 2px;
    width: 20px;
    height: 20px;
    background: white;
    border-radius: 50%;
    transition: transform 0.3s;
  }

  .toggle-input:checked + .toggle-slider {
    background: #059669;
  }

  .toggle-input:checked + .toggle-slider::after {
    transform: translateX(24px);
  }

  .toggle-label {
    font-size: 0.875rem;
    font-weight: 500;
    color: #374151;
  }

  .current-status {
    background: #f9fafb;
    padding: 1.5rem;
    border-radius: 0.5rem;
    margin-bottom: 1.5rem;
  }

  .gauge-large {
    display: flex;
    justify-content: center;
    margin-bottom: 1rem;
  }

  .status-text {
    text-align: center;
    font-size: 1rem;
    color: #6b7280;
  }

  .status-text strong {
    color: #1f2937;
    font-size: 1.25rem;
  }

  .section-divider {
    border-top: 1px solid #e5e7eb;
    padding-top: 1.5rem;
    margin-top: 1.5rem;
  }

  .section-title {
    font-size: 1.125rem;
    font-weight: 600;
    margin-bottom: 1rem;
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

  .btn-primary {
    background: #059669;
    color: white;
  }

  .btn-primary:hover {
    background: #047857;
  }

  .btn-primary:disabled {
    background: #9ca3af;
    cursor: not-allowed;
  }

  .save-message {
    margin-top: 1rem;
    padding: 0.75rem 1rem;
    border-radius: 0.5rem;
    background: #d1fae5;
    color: #065f46;
    font-size: 0.875rem;
    font-weight: 500;
  }

  .save-message.error {
    background: #fee2e2;
    color: #991b1b;
  }
</style>
