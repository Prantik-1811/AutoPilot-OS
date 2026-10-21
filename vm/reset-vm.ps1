[CmdletBinding(SupportsShouldProcess = $true, ConfirmImpact = "High")]
param(
    [Parameter(Mandatory = $true)]
    [string]$VmxPath,
    [string]$VmrunPath,
    [switch]$Headless
)

$ErrorActionPreference = "Stop"

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

$mode = if ($Headless) { "nogui" } else { "gui" }
Write-Warning "Reverting '$VmxPath' discards guest changes made since the AutoPilot-Golden snapshot."
if ($PSCmdlet.ShouldProcess($VmxPath, "Revert to AutoPilot-Golden and start the VM")) {
    & $VmrunPath -T ws revertToSnapshot $VmxPath "AutoPilot-Golden"
    if ($LASTEXITCODE -ne 0) {
        throw "vmrun failed to revert the VM ($LASTEXITCODE)."
    }
    & $VmrunPath -T ws start $VmxPath $mode
    if ($LASTEXITCODE -ne 0) {
        throw "The VM was reverted but could not be started ($LASTEXITCODE)."
    }
    Write-Output "Reverted to AutoPilot-Golden and started the VM."
}
