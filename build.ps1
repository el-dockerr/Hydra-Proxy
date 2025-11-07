# Hydra Build Script for Windows
# This script automates the build process

param(
    [string]$BuildType = "Release"
)

Write-Host "=================================="
Write-Host "   Hydra Build Script"
Write-Host "=================================="
Write-Host ""

Write-Host "Build type: $BuildType" -ForegroundColor Green
Write-Host ""

# Create build directory
if (Test-Path "build") {
    Write-Host "Cleaning existing build directory..."
    Remove-Item -Recurse -Force "build"
}

Write-Host "Creating build directory..."
New-Item -ItemType Directory -Path "build" | Out-Null
Set-Location "build"

# Configure with CMake
Write-Host ""
Write-Host "Configuring with CMake..."
cmake ..

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: CMake configuration failed!" -ForegroundColor Red
    Set-Location ..
    exit 1
}

# Build
Write-Host ""
Write-Host "Building..."
cmake --build . --config $BuildType

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Build failed!" -ForegroundColor Red
    Set-Location ..
    exit 1
}

# Copy config file to build directory
Write-Host ""
Write-Host "Copying configuration file..."
Copy-Item "..\config.json" ".\$BuildType\config.json" -Force

Set-Location ..

Write-Host ""
Write-Host "=================================="
Write-Host "   Build Successful!" -ForegroundColor Green
Write-Host "=================================="
Write-Host ""
Write-Host "Executable location: build\$BuildType\hydra.exe" -ForegroundColor Green
Write-Host ""
Write-Host "To run Hydra:" -ForegroundColor Yellow
Write-Host "  cd build\$BuildType" -ForegroundColor Yellow
Write-Host "  .\hydra.exe" -ForegroundColor Yellow
Write-Host ""

