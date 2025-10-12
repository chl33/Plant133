<script>
  export let value = 0;
  export let min = 30;
  export let max = 70;
  export let size = 'normal'; // 'normal' or 'large'

  $: gaugeSize = size === 'large' ? 180 : 120;
  $: strokeWidth = size === 'large' ? 12 : 8;
  $: radius = (gaugeSize - strokeWidth) / 2;
  $: circumference = 2 * Math.PI * radius;
  $: angle = 270; // 270 degrees for 3/4 circle
  $: startAngle = 135; // Start at bottom left
  
  // Calculate the progress based on value
  $: progress = Math.min(Math.max(value, 0), 100);
  $: offset = circumference - (progress / 100) * (angle / 360) * circumference;
  
  // Determine color based on value relative to min/max
  $: color = getColor(value, min, max);

  $: roundedValue = value.toFixed(1);
  
  function getColor(val, minimum, maximum) {
    if (val < minimum) return '#ef4444'; // red - too dry
    if (val > maximum) return '#3b82f6'; // blue - too wet
    return '#10b981'; // green - good
  }

  function polarToCartesian(centerX, centerY, radius, angleInDegrees) {
    const angleInRadians = (angleInDegrees - 90) * Math.PI / 180.0;
    return {
      x: centerX + (radius * Math.cos(angleInRadians)),
      y: centerY + (radius * Math.sin(angleInRadians))
    };
  }

  function describeArc(x, y, radius, startAngle, endAngle) {
    const start = polarToCartesian(x, y, radius, endAngle);
    const end = polarToCartesian(x, y, radius, startAngle);
    const largeArcFlag = endAngle - startAngle <= 180 ? "0" : "1";
    return [
      "M", start.x, start.y,
      "A", radius, radius, 0, largeArcFlag, 0, end.x, end.y
    ].join(" ");
  }

  $: center = gaugeSize / 2;
  $: backgroundPath = describeArc(center, center, radius, startAngle, startAngle + angle);
  $: progressPath = describeArc(center, center, radius, startAngle, startAngle + (angle * progress / 100));
</script>

<div class="gauge-container" class:large={size === 'large'}>
  <svg width={gaugeSize} height={gaugeSize * 0.75} viewBox="0 0 {gaugeSize} {gaugeSize * 0.75}">
    <!-- Background arc -->
    <path
      d={backgroundPath}
      fill="none"
      stroke="#e5e7eb"
      stroke-width={strokeWidth}
      stroke-linecap="round"
    />
    
    <!-- Progress arc -->
    <path
      d={progressPath}
      fill="none"
      stroke={color}
      stroke-width={strokeWidth}
      stroke-linecap="round"
    />
    
    <!-- Center text -->
    <text
      x={center}
      y={center + 5}
      text-anchor="middle"
      class="gauge-text"
      class:large-text={size === 'large'}
    >
      {roundedValue}%
    </text>
  </svg>
  
  <div class="gauge-labels">
    <span class="label-left">0%</span>
    <span class="label-right">100%</span>
  </div>
  
  <div class="range-indicator">
    <span class="range-label">Target: {min}%-{max}%</span>
  </div>
</div>

<style>
  .gauge-container {
    display: inline-block;
    position: relative;
  }

  .gauge-text {
    font-size: 20px;
    font-weight: 700;
    fill: #1f2937;
  }

  .large-text {
    font-size: 32px;
  }

  .gauge-labels {
    display: flex;
    justify-content: space-between;
    margin-top: -10px;
    padding: 0 10px;
    font-size: 11px;
    color: #6b7280;
  }

  .large .gauge-labels {
    font-size: 13px;
    padding: 0 15px;
  }

  .range-indicator {
    text-align: center;
    margin-top: 4px;
  }

  .range-label {
    font-size: 11px;
    color: #059669;
    font-weight: 500;
  }

  .large .range-label {
    font-size: 13px;
  }
</style>
