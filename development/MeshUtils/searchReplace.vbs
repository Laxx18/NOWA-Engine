
Set objFso = CreateObject("Scripting.FileSystemObject")
Set Folder = objFSO.GetFolder("C:\Users\lukas\Desktop\t2\")

For Each File In Folder.Files
    sNewFile = File.Name
    sNewFile = Replace(sNewFile,"_d_SPEC","_s")
    if (sNewFile<>File.Name) then 
        File.Move(File.ParentFolder+"\"+sNewFile)
    end if

Next