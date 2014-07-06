msbuild pom.targets /t:Construct /p:PackagePlatform=Win32 /p:PackageIDE=VS2012 /p:PackageToolSet=v110
msbuild pom.targets /t:Install /p:PackagePlatform=Win32 /p:PackageIDE=VS2012 /p:PackageToolSet=v110
