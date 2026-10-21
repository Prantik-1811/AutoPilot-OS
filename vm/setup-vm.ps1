param(
    [string]$VmxPath,
    [string]$VmrunPath,
    [ValidateSet("Disabled", "HostOnly")]
    [string]$Network = "Disabled",
    [switch]$CreateGoldenSnapshot,
    [switch]$SelfTest
)

$ErrorActionPreference = "Stop"

function Set-VmxSetting {
    param(
        [string[]]$Lines,
        [string]$Key,
        [string]$Value
    )

    $pattern = '^\s*' + [regex]::Escape($Key) + '\s*=.*$'
    $remaining = @($Lines | Where-Object { $_ -notmatch $pattern })
    return @($remaining + "$Key = `"$Value`"")
}

function Invoke-Vmrun {
    param([string[]]$Arguments)

    $output = @(& $VmrunPath -T ws @Arguments 2>&1)
    if ($LASTEXITCODE -ne 0) {
        throw "vmrun failed ($LASTEXITCODE): $($output -join [Environment]::NewLine)"
    }
    return $output
}

if ($SelfTest) {
    $lines = @(
        'displayName = "Test VM"',
        'isolation.tools.copy.disable = "FALSE"',
        'isolation.tools.copy.disable = "FALSE"'
    )
    $updated = @(Set-VmxSetting -Lines $lines -Key "isolation.tools.copy.disable" -Value "TRUE")
    $matches = @($updated | Where-Object { $_ -match '^\s*isolation\.tools\.copy\.disable\s*=' })
    if ($matches.Count -ne 1 -or $matches[0] -ne 'isolation.tools.copy.disable = "TRUE"') {
        throw "VMX settings self-test failed."
    }
    Write-Output "VMX settings self-test passed."
    exit 0
}

if (-not $VmxPath) {
    throw "Specify -VmxPath, or use -SelfTest."
}
if (-not (Test-Path -LiteralPath $VmxPath -PathType Leaf)) {
    throw "VMX file not found: $VmxPath"
}
$VmxPath = [System.IO.Path]::GetFullPath($VmxPath)

if (-not $VmrunPath) {
    $vmrunCommand = Get-Command "vmrun.exe" -ErrorAction SilentlyContinue
    if ($vmrunCommand) {
        $VmrunPath = $vmrunCommand.Source
    }
}
if (-not $VmrunPath -or -not (Test-Path -LiteralPath $VmrunPath -PathType Leaf)) {
    throw "Specify -VmrunPath or add VMware Workstation's vmrun.exe to PATH."
}

$running = @(Invoke-Vmrun -Arguments @("list") | Select-Object -Skip 1 | ForEach-Object { $_.Trim() } | Where-Object { $_ })
if ($running | Where-Object { $_ -ieq $VmxPath }) {
    throw "Power off this VM before changing its isolation settings."
}

$backupPath = "$VmxPath.autopilot.bak"
if (-not (Test-Path -LiteralPath $backupPath)) {
    Copy-Item -LiteralPath $VmxPath -Destination $backupPath
}

$lines = [System.Collections.Generic.List[string]]::new([string[]][System.IO.File]::ReadAllLines($VmxPath))
$settings = [ordered]@{
    "isolation.tools.copy.disable" = "TRUE"
    "isolation.tools.paste.disable" = "TRUE"
    "isolation.tools.dnd.disable" = "TRUE"
    "isolation.tools.hgfs.disable" = "TRUE"
    "sharedFolder.maxNum" = "0"
}
foreach ($key in $settings.Keys) {
    $lines = [System.Collections.Generic.List[string]]::new(
        [string[]](Set-VmxSetting -Lines $lines.ToArray() -Key $key -Value $settings[$key])
    )
}

$networkAdapters = @(
    $lines | ForEach-Object {
        if ($_ -match '^\s*ethernet(\d+)\.(?:present|connectionType)\s*=') {
            [int]$Matches[1]
        }
    } | Sort-Object -Unique
)
if ($networkAdapters.Count -eq 0) {
    $networkAdapters = @(0)
}
foreach ($adapter in $networkAdapters) {
    $present = if ($Network -eq "Disabled") { "FALSE" } else { "TRUE" }
    $lines = [System.Collections.Generic.List[string]]::new(
        [string[]](Set-VmxSetting -Lines $lines.ToArray() -Key "ethernet$adapter.present" -Value $present)
    )
    if ($Network -eq "HostOnly") {
        $lines = [System.Collections.Generic.List[string]]::new(
            [string[]](Set-VmxSetting -Lines $lines.ToArray() -Key "ethernet$adapter.connectionType" -Value "hostonly")
        )
    }
}

[System.IO.File]::WriteAllLines(
    $VmxPath,
    $lines.ToArray(),
    [System.Text.UTF8Encoding]::new($false)
)
Write-Output "Applied isolation settings to $VmxPath ($Network network)."
Write-Output "Original VM configuration backup: $backupPath"

if ($CreateGoldenSnapshot) {
    $snapshotName = "AutoPilot-Golden"
    $snapshots = Invoke-Vmrun -Arguments @("listSnapshots", $VmxPath)
    if (($snapshots -join "`n") -match "(?m)^\s*(?:Snapshot\s+\d+:\s*)?$([regex]::Escape($snapshotName))\s*$") {
        throw "Snapshot '$snapshotName' already exists; refusing to replace it."
    }
    [void](Invoke-Vmrun -Arguments @("snapshot", $VmxPath, $snapshotName))
    Write-Output "Created snapshot '$snapshotName'."
}
