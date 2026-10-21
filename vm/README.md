# Stage A VM setup (VMware Workstation, Windows 11)

## Isolation checklist

- [ ] Use a dedicated Windows 11 VM; install updates and the tools needed for the demo.
- [ ] Keep VMware shared folders, clipboard copy/paste, and drag-and-drop disabled.
- [ ] Keep the virtual network disabled by default. Use host-only networking only when a tested workflow requires it; never use bridged networking.
- [ ] Do not connect personal accounts, host credentials, or sensitive host data to the guest.
- [ ] Keep test data inside the guest. Do not enable shared folders or clipboard to export results; choose and verify a separate offline export path before running experiments.
- [ ] Power off the prepared VM and create the `AutoPilot-Golden` snapshot with `setup-vm.ps1`.
- [ ] Confirm reset restores the golden state before running tests; retain benchmark, journal, and log exports outside the VM only through the approved offline export path.

## Prepare and reset

Run in PowerShell on the host. Replace the example paths with the VMX and VMware Workstation `vmrun.exe` paths:

```powershell
.\vm\setup-vm.ps1 -VmxPath 'D:\VMs\AutoPilot\Windows 11.vmx' -VmrunPath 'C:\Program Files (x86)\VMware\VMware Workstation\vmrun.exe'
```

The setup command disables the virtual network by default, disables shared folders and host/guest clipboard and drag-and-drop, and writes a one-time `.autopilot.bak` copy before changing the VMX. To use host-only networking:

```powershell
.\vm\setup-vm.ps1 -VmxPath 'D:\VMs\AutoPilot\Windows 11.vmx' -Network HostOnly
```

After completing guest setup and powering off the VM, create the golden snapshot:

```powershell
.\vm\setup-vm.ps1 -VmxPath 'D:\VMs\AutoPilot\Windows 11.vmx' -CreateGoldenSnapshot
```

Reset and start the guest from that snapshot with:

```powershell
.\vm\reset-vm.ps1 -VmxPath 'D:\VMs\AutoPilot\Windows 11.vmx'
```

Add `-Headless` to start without a VMware GUI. Reset restores the snapshot and discards later guest changes; the seeded-data generator and reseeding workflow are not part of Phase 0.

## Check

Run the configuration helper's dependency-free test:

```powershell
.\vm\setup-vm.ps1 -SelfTest
```

Before the Stage A gate, verify the guest has no host-shared paths, clipboard integration is unavailable, networking is disabled or host-only, and reset returns the VM to the golden snapshot.
