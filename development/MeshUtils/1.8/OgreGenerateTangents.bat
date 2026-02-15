for %%f in (*.mesh) do .\OgreXMLConverter.exe -t %%f 
for %%f in (*.mesh.xml) do .\OgreXMLConverter.exe -t -e %%f 