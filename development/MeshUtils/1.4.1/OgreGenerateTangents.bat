for %%f in (*.mesh) do .\OgreXMLConverter.exe -t %%f 
for %%f in (*.mesh.xml) do .\OgreXMLConverter.exe -t -e %%f 
for %%f in (*.skeleton) do .\OgreXMLConverter.exe %%f 
for %%f in (*.skeleton.xml) do .\OgreXMLConverter.exe %%f 