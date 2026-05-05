$ErrorActionPreference = 'Stop'

$root = Resolve-Path (Join-Path $PSScriptRoot '..')
$uvproj = Join-Path $root 'MDK-ARM\STM32H750XBH6.uvprojx'
$mapPath = Join-Path $root 'MDK-ARM\STM32H750XBH6\STM32H750XBH6.map'
$failures = New-Object System.Collections.Generic.List[string]

function Add-Failure([string]$message) {
  $script:failures.Add($message) | Out-Null
}

if (-not (Test-Path -LiteralPath $uvproj)) {
  Add-Failure "Missing Keil project: $uvproj"
} else {
  $projectText = Get-Content -LiteralPath $uvproj -Raw

  if ($projectText -match 'ARM_CM4F') {
    Add-Failure 'Keil project still references FreeRTOS ARM_CM4F port.'
  }
  if ($projectText -notmatch 'FreeRTOS/Source/portable/RVDS/ARM_CM7/r0p1') {
    Add-Failure 'Keil include path does not reference FreeRTOS ARM_CM7/r0p1.'
  }
  if ($projectText -notmatch 'FreeRTOS[/\\]Source[/\\]portable[/\\]RVDS[/\\]ARM_CM7[/\\]r0p1[/\\]port\.c') {
    Add-Failure 'Keil source list does not reference ARM_CM7/r0p1/port.c.'
  }

  foreach ($token in @('GUI-Guider_Runtime', 'GUI-Guider_Source', 'dac_wave_sync.c', 'dac_wave_sync.h')) {
    if ($projectText -match [regex]::Escape($token)) {
      Add-Failure "Legacy source is still referenced by Keil project: $token"
    }
  }

  if ($projectText -notmatch '<Device>STM32H750XBHx</Device>') {
    Add-Failure 'Keil target device is not fixed to STM32H750XBHx.'
  }
  if ($projectText -notmatch '<ScatterFile>STM32H750XBH6_d2\.sct</ScatterFile>') {
    Add-Failure 'Active scatter file is not STM32H750XBH6_d2.sct.'
  }
  if ($projectText -notmatch 'STM32H7x_2048\.FLM') {
    Add-Failure 'Custom STM32H7x_2048.FLM flash algorithm is missing.'
  }
}

if (Test-Path -LiteralPath $mapPath) {
  $mapText = Get-Content -LiteralPath $mapPath -Raw

  function Get-MapRegion([string]$kind, [string]$name) {
    $pattern = "$kind $name .*?Size:\s+0x([0-9A-Fa-f]+), Max:\s+0x([0-9A-Fa-f]+)"
    $match = [regex]::Match($script:mapText, $pattern)
    if (-not $match.Success) {
      return $null
    }

    [pscustomobject]@{
      Name = $name
      Size = [Convert]::ToInt32($match.Groups[1].Value, 16)
      Max  = [Convert]::ToInt32($match.Groups[2].Value, 16)
    }
  }

  foreach ($region in @(
      (Get-MapRegion 'Load Region' 'LR_IROM1'),
      (Get-MapRegion 'Load Region' 'LR_IROM2'),
      (Get-MapRegion 'Execution Region' 'RW_D2SRAM')
    )) {
    if ($null -ne $region) {
      $free = $region.Max - $region.Size
      Write-Host ("{0}: used=0x{1:X} max=0x{2:X} free=0x{3:X}" -f $region.Name, $region.Size, $region.Max, $free)
    }
  }

  $d2 = Get-MapRegion 'Execution Region' 'RW_D2SRAM'
  if ($null -eq $d2) {
    Add-Failure 'Map file does not contain RW_D2SRAM.'
  } else {
    $d2Free = $d2.Max - $d2.Size
    if ($d2Free -lt 32) {
      Add-Failure ("RW_D2SRAM free space is below 32 bytes: {0}" -f $d2Free)
    }
  }
} else {
  Write-Warning "Map file not found; skipping memory budget check: $mapPath"
}

$latestBuildLog = Get-ChildItem -LiteralPath (Join-Path $root 'MDK-ARM') -Filter 'rebuild*.log' -File |
  Sort-Object LastWriteTime -Descending |
  Select-Object -First 1

if ($null -ne $latestBuildLog) {
  $logText = Get-Content -LiteralPath $latestBuildLog.FullName -Raw
  if ($logText -match '0 Error\(s\), 0 Warning\(s\)') {
    Write-Host "Latest rebuild log is clean: $($latestBuildLog.Name)"
  } else {
    Write-Warning "Latest rebuild log is not clean or uses an unexpected summary: $($latestBuildLog.Name)"
  }
} else {
  Write-Warning 'No rebuild*.log found; skipping build-log summary check.'
}

if ($failures.Count -gt 0) {
  foreach ($failure in $failures) {
    Write-Error $failure -ErrorAction Continue
  }
  throw "Keil audit failed with $($failures.Count) issue(s)."
}

Write-Host 'Keil audit checks passed.'
