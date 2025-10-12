<script>
  import { Droplet } from 'lucide-svelte';

  export let wifi;
  $: wifiConfig = $wifi;
</script>

<nav class="navbar">
  <div class="navbar-content">
    <Droplet size={32} />
    <h1>Plant133 {wifiConfig ? wifiConfig.board : ''}</h1>
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
    display: flex;
    align-items: center;
    gap: 0.75rem;
  }

  h1 {
    font-size: 1.5rem;
    font-weight: 700;
  }
</style>

