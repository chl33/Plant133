<script>
  import { Save } from 'lucide-svelte';

  // API base URL
  const API_BASE = '/api';

  export let mqtt;
  export let systemStatus;

  let mqttConfig;
  let sysStat;
  let saving = false;
  let saveMessage = '';

  $: mqttConfig = $mqtt;
  $: status = $systemStatus;

  function updateMqtt(field, value) {
    mqtt.update(m => {
      m[field] = value;
      return m;
    });
  }

  async function saveMqttConfig() {
    saving = true;
    saveMessage = '';

    try {
      const response = await fetch(`${API_BASE}/mqtt`, {
        method: 'PUT',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify(mqttConfig)
      });

      if (!response.ok) {
        throw new Error('Failed to save MQTT configuration');
      }

      saveMessage = 'MQTT configuration saved successfully!';
      setTimeout(() => saveMessage = '', 3000);
    } catch (err) {
      console.error('Error saving MQTT config:', err);
      saveMessage = `Error: ${err.message}`;
    } finally {
      saving = false;
    }
  }
</script>

<div class="page">
  <h2 class="page-title">MQTT Configuration</h2>

  <div class="card">
    <div class="status-badge" class:disabled={status && !status.mqttConnected}>
      {status && status.mqttConnected ? 'Connected' : 'Not connected'}
    </div>
    <div class="form-group">
      <label class="form-label">MQTT Broker Address</label>
      <input
        type="text"
        class="form-input"
        placeholder="mqtt.example.com or 192.168.1.100"
        bind:value={mqttConfig.host}
        on:change={() => updateMqtt('host', mqttConfig.host)}
      />
    </div>

    <div class="form-group">
      <label class="form-label">Port</label>
      <input
        type="number"
        class="form-input"
        bind:value={mqttConfig.port}
        on:change={() => updateMqtt('port', mqttConfig.port)}
      />
    </div>

    <div class="form-group">
      <label class="form-label">Username (optional)</label>
      <input
        type="text"
        class="form-input"
        placeholder="Leave blank if not required"
        bind:value={mqttConfig.user}
        on:change={() => updateMqtt('user', mqttConfig.user)}
      />
    </div>

    <div class="form-group">
      <label class="form-label">Password (optional)</label>
      <input
        type="password"
        class="form-input"
        placeholder="Leave blank if not required"
        bind:value={mqttConfig.password}
        on:change={() => updateMqtt('password', mqttConfig.password)}
      />
    </div>

    <button class="btn btn-purple" on:click={saveMqttConfig} disabled={saving}>
      <Save size={20} />
      {saving ? 'Saving...' : 'Save MQTT Configuration'}
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
    border-color: #7c3aed;
    box-shadow: 0 0 0 3px rgba(124, 58, 237, 0.1);
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

  .btn-purple {
    background: #7c3aed;
    color: white;
  }

  .btn-purple:hover {
    background: #6d28d9;
  }

  .btn-purple:disabled {
    background: #9ca3af;
    cursor: not-allowed;
  }

  .save-message {
    margin-top: 1rem;
    padding: 0.75rem 1rem;
    border-radius: 0.5rem;
    background: #ede9fe;
    color: #5b21b6;
    font-size: 0.875rem;
    font-weight: 500;
  }

  .save-message.error {
    background: #fee2e2;
    color: #991b1b;
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
</style>
