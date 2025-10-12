<script>
  import { createEventDispatcher } from 'svelte';
  import { Droplet, Wifi, Radio, Thermometer, Wind, Droplets } from 'lucide-svelte';
  import MoistureGauge from '../components/MoistureGauge.svelte';

  export let plants;
  export let wifi;
  export let mqtt;
  export let systemStatus;

  const dispatch = createEventDispatcher();

  function navigate(page) {
    dispatch('changePage', page);
  }

  function getMoistureStatus(moisture, min, max) {
    if (moisture < min) return 'low';
    if (moisture > max) return 'high';
    return 'good';
  }

  function formatTime(seconds) {
    if (seconds < 60) return `${seconds}s`;
    const minutes = Math.floor(seconds / 60);
    const secs = seconds % 60;
    return `${minutes}m ${secs}s`;
  }

  $: plantsList = $plants;
  $: wifiConfig = $wifi;
  $: mqttConfig = $mqtt;
  $: status = $systemStatus;
</script>

<div class="page">
  <h2 class="page-title">System Overview</h2>

  <!-- System Status Cards -->
  <div class="system-status-bar">
    <div class="stat-compact">
      <span class="stat-icon-inline temp-color">
        <Thermometer size={18} />
      </span>
      <span class="stat-text">{status.temperature.toFixed(1)}°C</span>
    </div>

    <div class="stat-compact">
      <span class="stat-icon-inline humidity-color">
        <Wind size={18} />
      </span>
      <span class="stat-text">{status.humidity.toFixed(1)}%</span>
    </div>

    <div class="stat-compact">
      <span class="stat-icon-inline" class:water-ok-color={status.waterLevel} class:water-low-color={!status.waterLevel}>
        <Droplets size={18} />
      </span>
      <span class="stat-text" class:status-ok={status.waterLevel} class:status-warning={!status.waterLevel}>
        {status.waterLevel ? 'Reservoir OK' : 'Reservoir Low'}
      </span>
    </div>

    <div class="stat-compact">
      <span class="stat-icon-inline pump-color">
        <Droplet size={18} />
      </span>
      <span class="stat-text">Pump: {formatTime(status.pumpTimeRemaining)}</span>
    </div>
  </div>

  <!-- Plant Status Cards -->

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
        <div class="card-content">
          <p>State: {plant.state}</p>
          <p>Target Range: {plant.minMoisture}% - {plant.maxMoisture}%</p>
          <p>Doses: {plant.doseCount} / {plant.maxDosesPerCycle}</p>
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
      <p>{mqttConfig.host || 'Not configured'}</p>
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

  @media (max-width: 768px) {
    .page-title {
      font-size: 1.5rem;
      margin-bottom: 1rem;
    }
  }

  .card-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
    gap: 1rem;
    margin-bottom: 1rem;
  }

  @media (max-width: 768px) {
    .card-grid {
      grid-template-columns: 1fr;
      gap: 0.75rem;
    }
  }

  .overview-card {
    background: white;
    padding: 1.5rem;
    border-radius: 0.5rem;
    border: 1px solid #e5e7eb;
  }

  @media (max-width: 768px) {
    .overview-card {
      padding: 1rem;
    }
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

  @media (max-width: 768px) {
    .card-header h3 {
      font-size: 1.125rem;
    }
  }

  .status-badge {
    padding: 0.25rem 0.75rem;
    border-radius: 9999px;
    font-size: 0.75rem;
    font-weight: 600;
    background: #d1fae5;
    color: #065f46;
  }

  @media (max-width: 768px) {
    .status-badge {
      padding: 0.2rem 0.5rem;
      font-size: 0.625rem;
    }
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

  .system-status-bar {
    display: flex;
    flex-wrap: wrap;
    gap: 1.5rem;
    padding: 1rem;
    background: white;
    border-radius: 0.5rem;
    border: 1px solid #e5e7eb;
    margin-bottom: 1.5rem;
  }

  @media (max-width: 768px) {
    .system-status-bar {
      gap: 0.75rem;
      padding: 0.75rem;
      margin-bottom: 1rem;
    }
  }

  .stat-compact {
    display: flex;
    align-items: center;
    gap: 0.5rem;
  }

  @media (max-width: 768px) {
    .stat-compact {
      gap: 0.375rem;
      flex: 1 1 calc(50% - 0.375rem);
      min-width: 0;
    }
  }

  .stat-icon-inline {
    flex-shrink: 0;
  }

  .stat-icon-inline.temp-color {
    color: #dc2626;
  }

  .stat-icon-inline.humidity-color {
    color: #2563eb;
  }

  .stat-icon-inline.water-ok-color {
    color: #059669;
  }

  .stat-icon-inline.water-low-color {
    color: #f59e0b;
  }

  .stat-icon-inline.pump-color {
    color: #6366f1;
  }

  .stat-text {
    font-size: 0.875rem;
    font-weight: 600;
    color: #1f2937;
  }

  @media (max-width: 768px) {
    .stat-text {
      font-size: 0.75rem;
      white-space: nowrap;
      overflow: hidden;
      text-overflow: ellipsis;
    }
  }

  .stat-text.status-ok {
    color: #059669;
  }

  .stat-text.status-warning {
    color: #f59e0b;
  }
</style>
