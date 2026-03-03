@echo off

REM ================================
REM Debug
REM ================================

if not exist ".\bin\Debug\libs" mkdir ".\bin\Debug\libs"
if not exist ".\bin\Debug" mkdir ".\bin\Debug"

copy /Y ".\external\newton-4.00\sdk\Debug\ndNewton_d.exp" ".\bin\Debug\libs\"
copy /Y ".\external\newton-4.00\sdk\Debug\ndNewton_d.lib" ".\bin\Debug\libs\"
copy /Y ".\external\newton-4.00\sdk\dModel\Debug\ndModel_d.lib" ".\bin\Debug\libs\"

copy /Y ".\external\newton-4.00\sdk\Debug\ndNewton_d.pdb" ".\bin\Debug\"
copy /Y ".\external\newton-4.00\sdk\Debug\ndNewton_d.dll" ".\bin\Debug\"
copy /Y ".\external\newton-4.00\sdk\dModel\Debug\ndModel_d.pdb" ".\bin\Debug\"


REM ================================
REM Release
REM ================================

if not exist ".\bin\Release\libs" mkdir ".\bin\Release\libs"
if not exist ".\bin\Release" mkdir ".\bin\Release"

copy /Y ".\external\newton-4.00\sdk\Release\ndNewton.exp" ".\bin\Release\libs\"
copy /Y ".\external\newton-4.00\sdk\Release\ndNewton.lib" ".\bin\Release\libs\"
copy /Y ".\external\newton-4.00\sdk\dModel\Release\ndModel.lib" ".\bin\Release\libs\"

copy /Y ".\external\newton-4.00\sdk\Release\ndNewton.pdb" ".\bin\Release\"
copy /Y ".\external\newton-4.00\sdk\Release\ndNewton.dll" ".\bin\Release\"
copy /Y ".\external\newton-4.00\sdk\dModel\Release\ndModel.pdb" ".\bin\Release\"

exit /b 0