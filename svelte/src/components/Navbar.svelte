<script>
  import { Droplet, Activity, ZapOff } from 'lucide-svelte';

  export let wifi;
  export let isOnline;
  $: wifiConfig = $wifi;
  $: online = $isOnline;
</script>

<nav class="navbar">
  <div class="navbar-content">
    <div class="brand">
      <Droplet size={32} />
      <h1>Plant133</h1>
    </div>

    <div class="board-name">
      {wifiConfig ? wifiConfig.board : ''}
    </div>

    <div class="status-container">
      <div class="status-indicator" class:online={online} title={online ? 'Device is online' : 'Device is unreachable'}>
        {#if online}
          <Activity size={20} />
          <span>Online</span>
        {:else}
          <ZapOff size={20} />
          <span>Offline</span>
        {/if}
      </div>
    </div>
  </div>
</nav>

<style>
  .navbar {
    background: #047857;
    color: white;
    padding: 1rem 2rem;
    box-shadow: 0 2px 4px rgba(0,0,0,0.1);
  }

  .navbar-content {
    max-width: 1200px;
    margin: 0 auto;
    display: grid;
    grid-template-columns: 1fr auto 1fr;
    align-items: center;
    gap: 0.75rem;
  }

  .brand {
    display: flex;
    align-items: center;
    gap: 0.75rem;
  }

  .board-name {
    font-size: 1.25rem;
    font-weight: 600;
    background: rgba(255, 255, 255, 0.1);
    padding: 0.25rem 1rem;
    border-radius: 0.5rem;
    text-align: center;
  }

  .status-container {
    display: flex;
    justify-content: flex-end;
  }

  .status-indicator {
    display: flex;
    align-items: center;
    gap: 0.5rem;
    padding: 0.4rem 0.8rem;
    border-radius: 2rem;
    background: #991b1b;
    font-size: 0.875rem;
    font-weight: 600;
    transition: background 0.3s;
  }

  .status-indicator.online {
    background: #065f46;
  }

  h1 {
    font-size: 1.5rem;
    font-weight: 700;
  }

  @media (max-width: 768px) {
    h1 {
      font-size: 1.125rem;
    }
    .navbar {
      padding: 1rem;
    }
  }
</style>
