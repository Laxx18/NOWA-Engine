for %%f in (*.mesh) do (
    .\OgreMeshTool.exe -U "%%f" "%%~nf_unoptimized.mesh"
)