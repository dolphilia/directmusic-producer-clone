# Source Producer deployment

Build with scripts/Build-ProductSnapshot.ps1 (Win32, MSVC 2022, Windows SDK 10.0.26100.0, CMake; reference tools OFF). The fixed build-summary records sources, commands, logs and hashes. CMake install contains bin/Producer.exe, tools deployment scripts and this document.

Deployment scripts require PowerShell 7.2 or newer (.NET), including its standard Add-Type compiler. Cloud Files reparse metadata (CLOUD through CLOUD_F) is accepted; name-surrogate links and every unknown reparse tag are refused.

Run the packaged tools/Install-SourceProduct.ps1 -BuildSummaryPath <fixed-summary> -Destination <new-directory>. Parent must exist; destination must not exist. No Producer COM registration, file associations or machine settings are changed. Launch bin/Producer.exe. Windows DirectMusic/DirectSound and declared sound resources remain OS dependencies; this package does not redistribute or unregister them.

Run packaged tools/Uninstall-SourceProduct.ps1 -InstallRoot <directory>. It verifies the exact manifest/root and every owned file, refuses modified/missing assets, unknown files/directories and reparse paths before removal, and removes only that verified installation. Put authored documents outside the install directory; a document inside causes refusal and is retained. Existing original Producer installations are separate and untouched. No recursive deletion is used. I/O failures can leave partial owned-file removal and are reported as failures; concurrent mutation and transactional upgrades remain unsupported.

Portable deployment is a replacement for own-state installation/cleanup, not the legacy installer DLL ABI. Successful packaging alone does not establish all40 functions, original interoperability, clean independent environment or the full eight acceptance criteria.

Installation is published by renaming a verified staging directory within the destination parent. Copy or manifest-write failure leaves the requested destination absent and retains the staging directory named in the error for inspection. An existing or concurrently created destination is refused. This is fresh-install publication; upgrade rollback and uninstall I/O rollback remain unimplemented.
