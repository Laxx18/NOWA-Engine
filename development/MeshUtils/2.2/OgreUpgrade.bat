@echo off
title Ogre Mesh Safe Converter

echo.
echo ==========================================
echo   OGRE SAFE MESH CONVERSION (LEGACY SAFE)
echo ==========================================
echo.

REM Ensure tool exists
if not exist OgreMeshTool.exe (
    echo ERROR: OgreMeshTool.exe not found!
    pause
    exit /b
)

REM --------------------------------------------------
REM STEP 1 — Normalize & repair legacy meshes
REM --------------------------------------------------
echo STEP 1: Normalizing meshes...
echo.

for %%f in (*.mesh) do (
    echo [Normalize] %%f
    OgreMeshTool.exe -U "%%f"
)

echo.

REM --------------------------------------------------
REM STEP 2 — Generate tangents (required for PBS)
REM --------------------------------------------------
echo STEP 2: Generating tangents...
echo.

for %%f in (*.mesh) do (
    echo [Tangents] %%f
    OgreMeshTool.exe -t -ts 4 "%%f"
)

echo.

REM --------------------------------------------------
REM STEP 3 — Convert to Ogre v2 (SAFE MODE)
REM --------------------------------------------------
echo STEP 3: Converting to v2 format...
echo.

for %%f in (*.mesh) do (
    echo [Convert v2] %%f
    OgreMeshTool.exe -v2 -e "%%f"
)

echo.

REM --------------------------------------------------
REM STEP 4 — Convert skeletons
REM --------------------------------------------------
echo STEP 4: Conerting skeletons...
echo.

for %%f in (*.skeleton) do (
    echo [Skeleton] %%f
    OgreMeshTool.exe "%%f"
)

echo.
echo ==========================================
echo SAFE conversion complete.
echo ==========================================
echo.
echo If meshes are visible now, you can run
echo the optimization script next.
echo.

pause
