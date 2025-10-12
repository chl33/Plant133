<script>
  import { createEventDispatcher } from 'svelte';
  import { Droplet, Wifi, Radio, Home } from 'lucide-svelte';

  export let currentPage;
  export let plants;

  const dispatch = createEventDispatcher();

  function navigate(page) {
    dispatch('changePage', page);
  }

  $: plantsList = $plants;
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
</aside>

<style>
  .sidebar {
    width: 16rem;
    background: #1f2937;
    color: white;
    padding: 1rem;
    min-height: calc(100vh - 64px);
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
</style>
