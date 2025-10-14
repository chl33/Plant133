<script>
  export let value = 0;
  export let min = 30;
  export let max = 70;
  export let size = 'normal'; // 'normal' or 'large'

  $: gaugeSize = size === 'large' ? 180 : 120;
  $: strokeWidth = size === 'large' ? 12 : 8;
  $: radius = (gaugeSize - strokeWidth) / 2;
  $: circumference = 2 * Math.PI * radius;
  $: angle = 180; // 180 degrees for semicircle
  $: startAngle = -90; // Start at left horizontal

  // Calculate the progress based on value
  $: progress = Math.min(Math.max(value, 0), 100);

  // Calculate segment endpoints as percentages of the full arc
  $: minPercent = Math.min(Math.max(min, 0), 100);
  $: maxPercent = Math.min(Math.max(max, 0), 100);

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
  $: centerY = gaugeSize / 2 + strokeWidth;

  // Background arc (full range)
  $: backgroundPath = describeArc(center, centerY, radius, startAngle, startAngle + angle);

  // Yellow segment (0 to min)
  $: yellowEndAngle = startAngle + (angle * minPercent / 100);
  $: yellowPath = describeArc(center, centerY, radius, startAngle, yellowEndAngle);

  // Green segment (min to max)
  $: greenStartAngle = startAngle + (angle * minPercent / 100);
  $: greenEndAngle = startAngle + (angle * maxPercent / 100);
  $: greenPath = describeArc(center, centerY, radius, greenStartAngle, greenEndAngle);

  // Blue segment (max to 100)
  $: blueStartAngle = startAngle + (angle * maxPercent / 100);
  $: blueEndAngle = startAngle + angle;
  $: bluePath = describeArc(center, centerY, radius, blueStartAngle, blueEndAngle);

  // Progress indicator (current value)
  $: progressAngle = startAngle + (angle * progress / 100);
  $: progressDot = polarToCartesian(center, centerY, radius, progressAngle);

  // Determine color for center text based on value
  $: textColor = progress < minPercent ? '#f59e0b' : progress > maxPercent ? '#3b82f6' : '#10b981';
</script>

<div class="gauge-container" class:large={size === 'large'}>
  <svg width={gaugeSize} height={gaugeSize * 0.65} viewBox="0 0 {gaugeSize} {gaugeSize * 0.65}">
    <!-- Background arc (gray) -->
    <path
      d={backgroundPath}
      fill="none"
      stroke="#e5e7eb"
      stroke-width={strokeWidth}
      stroke-linecap="round"
    />

    <!-- Yellow segment (below minimum) -->
    <path
      d={yellowPath}
      fill="none"
      stroke="#fbbf24"
      stroke-width={strokeWidth}
      stroke-linecap="round"
    />

    <!-- Green segment (optimal range) -->
    <path
      d={greenPath}
      fill="none"
      stroke="#10b981"
      stroke-width={strokeWidth}
      stroke-linecap="round"
    />

    <!-- Blue segment (above maximum) -->
    <path
      d={bluePath}
      fill="none"
      stroke="#60a5fa"
      stroke-width={strokeWidth}
      stroke-linecap="round"
    />

    <!-- Current value indicator dot -->
    <circle
      cx={progressDot.x}
      cy={progressDot.y}
      r={strokeWidth * 0.8}
      fill={textColor}
      stroke="white"
      stroke-width="2"
    />

    <!-- Center text -->
    <text
      x={center}
      y={centerY + 10}
      text-anchor="middle"
      class="gauge-text"
      class:large-text={size === 'large'}
      fill={textColor}
    >
      {value.toFixed(1)}%
    </text>
  </svg>

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
  }

  .large-text {
    font-size: 32px;
  }

  .gauge-labels {
    display: flex;
    justify-content: space-between;
    margin-top: 0.25rem;
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
    margin-top: 0.5rem;
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