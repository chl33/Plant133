<script>
  import { createEventDispatcher } from 'svelte';
  import { Droplet, Wifi, Radio } from 'lucide-svelte';
  import MoistureGauge from '../components/MoistureGauge.svelte';

  export let plants;
  export let wifi;
  export let mqtt;

  const dispatch = createEventDispatcher();

  function navigate(page) {
    dispatch('changePage', page);
  }

  function getMoistureStatus(moisture, min, max) {
    if (moisture < min) return 'low';
    if (moisture > max) return 'high';
    return 'good';
  }

  $: plantsList = $plants;
  $: wifiConfig = $wifi;
  $: mqttConfig = $mqtt;
</script>

<div class="page">
  <h2 class="page-title">System Overview</h2>

  <div class="card-grid">
    {#each plantsList as plant}
      <div class="overview-card">
        <div class="card-header">
          <Droplet size={24} class="text-green-600" />
          <h3>{plant.name}</h3>
          <div class="status-badge" class:disabled={!plant.enabled}>
            {plant.enabled ? 'Enabled' : 'Disabled'}
          </div>
        </div>
        <div class="gauge-container">
          <MoistureGauge
            value={plant.currentMoisture}
            min={plant.minMoisture}
            max={plant.maxMoisture}
          />
        </div>
        <button class="link-btn" on:click={() => navigate(`plant${plant.id}`)}>
          Configure →
        </button>
      </div>
    {/each}
  </div>

  <div class="card-grid">
    <div class="status-card blue">
      <div class="card-header">
        <Wifi size={24} />
        <h3>WiFi Status</h3>
      </div>
      <p>{wifiConfig.essid || 'Not configured'}</p>
      <button class="link-btn" on:click={() => navigate('wifi')}>
        Configure WiFi →
      </button>
    </div>

    <div class="status-card purple">
      <div class="card-header">
        <Radio size={24} />
        <h3>MQTT Status</h3>
      </div>
      <p>{mqttConfig.broker || 'Not configured'}</p>
      <button class="link-btn" on:click={() => navigate('mqtt')}>
        Configure MQTT →
      </button>
    </div>
  </div>
</div>

<style>
  .page-title {
    font-size: 2rem;
    font-weight: 700;
    color: #1f2937;
    margin-bottom: 1.5rem;
  }

  .card-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
    gap: 1rem;
    margin-bottom: 1rem;
  }

  .overview-card {
    background: white;
    padding: 1.5rem;
    border-radius: 0.5rem;
    border: 1px solid #e5e7eb;
  }

  .card-header {
    display: flex;
    align-items: center;
    gap: 0.75rem;
    margin-bottom: 1rem;
  }

  .card-header h3 {
    font-size: 1.25rem;
    font-weight: 600;
    flex: 1;
  }

  .status-badge {
    padding: 0.25rem 0.75rem;
    border-radius: 9999px;
    font-size: 0.75rem;
    font-weight: 600;
    background: #d1fae5;
    color: #065f46;
  }

  .status-badge.disabled {
    background: #fee2e2;
    color: #991b1b;
  }

  .gauge-container {
    margin-bottom: 1rem;
  }

  .card-content p {
    font-size: 0.875rem;
    color: #6b7280;
    margin-bottom: 0.5rem;
  }

  .card-content strong {
    color: #1f2937;
  }

  .status-card {
    padding: 1.5rem;
    border-radius: 0.5rem;
    border: 1px solid;
  }

  .status-card.blue {
    background: #eff6ff;
    border-color: #bfdbfe;
  }

  .status-card.purple {
    background: #f5f3ff;
    border-color: #ddd6fe;
  }

  .status-card p {
    color: #6b7280;
    margin-bottom: 0.5rem;
  }

  .link-btn {
    margin-top: 1rem;
    color: #059669;
    font-weight: 500;
    background: none;
    border: none;
    cursor: pointer;
    font-size: 0.875rem;
  }

  .link-btn:hover {
    color: #047857;
  }

  .status-card.blue .link-btn {
    color: #2563eb;
  }

  .status-card.blue .link-btn:hover {
    color: #1d4ed8;
  }

  .status-card.purple .link-btn {
    color: #7c3aed;
  }

  .status-card.purple .link-btn:hover {
    color: #6d28d9;
  }
</style>
