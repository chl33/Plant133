<script>
  import { createEventDispatcher } from 'svelte';
  import { Droplet, Wifi, Radio, Home, RefreshCw } from 'lucide-svelte';

  export let currentPage;
  export let plants;
  export let systemStatus;

  const dispatch = createEventDispatcher();

  function navigate(page) {
    dispatch('changePage', page);
  }

  $: plantsList = $plants;
  $: status = $systemStatus;

  async function restartDevice() {
    if (!confirm('Are you sure you want to restart the device?')) return;
    try {
      await fetch('/api/restart', { method: 'POST' });
      alert('Device is restarting...');
    } catch (err) {
      console.error('Error restarting:', err);
      alert('Failed to restart device');
    }
  }
</script>

<aside class="sidebar">
  <button
    class="nav-button"
    class:active={currentPage === 'home'}
    on:click={() => navigate('home')}>
    <Home size={20} />
    <span>Overview</span>
  </button>

  <div class="nav-section">Plant Configuration</div>

  {#each plantsList as plant}
    <button
      class="nav-button"
      class:active={currentPage === `plant${plant.id}`}
      on:click={() => navigate(`plant${plant.id}`)}>
      <Droplet size={20} />
      <span>{plant.name}</span>
    </button>
  {/each}

  <div class="nav-section">System Settings</div>

  <button
    class="nav-button"
    class:active={currentPage === 'wifi'}
    on:click={() => navigate('wifi')}>
    <Wifi size={20} />
    <span>WiFi Setup</span>
  </button>

  <button
    class="nav-button"
    class:active={currentPage === 'mqtt'}
    on:click={() => navigate('mqtt')}>
    <Radio size={20} />
    <span>MQTT Setup</span>
  </button>

  <div class="nav-section">Device</div>

  <button class="nav-button" on:click={restartDevice}>
    <RefreshCw size={20} />
    <span>Restart Device</span>
  </button>

  <div>
  <p>Software: v{status.software}</p>
  <p>Hardware: board v{status.hardware}</p>
  </div>
</aside>

<style>
  .sidebar {
    width: 16rem;
    background: #1f2937;
    color: white;
    padding: 1rem;
    min-height: calc(100vh - 64px);
  }

  @media (max-width: 768px) {
    .sidebar {
      width: 100%;
      min-height: auto;
      padding: 0.5rem;
    }
  }

  .nav-button {
    width: 100%;
    display: flex;
    align-items: center;
    gap: 0.75rem;
    padding: 0.75rem;
    margin-bottom: 0.5rem;
    border: none;
    background: transparent;
    color: white;
    border-radius: 0.5rem;
    cursor: pointer;
    transition: background 0.2s;
    font-size: 1rem;
  }

  @media (max-width: 768px) {
    .nav-button {
      padding: 0.5rem;
      font-size: 0.875rem;
      margin-bottom: 0.25rem;
    }
  }

  .nav-button:hover {
    background: #374151;
  }

  .nav-button.active {
    background: #059669;
  }

  .nav-section {
    margin-top: 1.5rem;
    margin-bottom: 0.5rem;
    padding: 0.5rem 0.75rem;
    text-transform: uppercase;
    font-size: 0.75rem;
    color: #9ca3af;
    font-weight: 600;
  }

  @media (max-width: 768px) {
    .nav-section {
      margin-top: 0.75rem;
      margin-bottom: 0.25rem;
      padding: 0.25rem 0.5rem;
      font-size: 0.625rem;
    }
  }
</style>
