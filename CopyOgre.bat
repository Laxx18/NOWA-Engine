@echo off

set SRC_LIB_DEBUG=E:\Ogre2.2SDK\VCBuild\lib\debug
set SRC_BIN_DEBUG=E:\Ogre2.2SDK\VCBuild\bin\debug
set SRC_LIB_RELEASE=E:\Ogre2.2SDK\VCBuild\lib\release
set SRC_BIN_RELEASE=E:\Ogre2.2SDK\VCBuild\bin\release

set DST_LIB_DEBUG=.\bin\Debug\libs
set DST_BIN_DEBUG=.\bin\Debug
set DST_LIB_RELEASE=.\bin\Release\libs
set DST_BIN_RELEASE=.\bin\Release

REM =========================
REM DEBUG LIBS
REM =========================

copy /Y "%SRC_LIB_DEBUG%\OgreHlmsPbs_d.exp" "%DST_LIB_DEBUG%\"
copy /Y "%SRC_LIB_DEBUG%\OgreHlmsPbs_d.lib" "%DST_LIB_DEBUG%\"
copy /Y "%SRC_LIB_DEBUG%\OgreHlmsUnlit_d.exp" "%DST_LIB_DEBUG%\"
copy /Y "%SRC_LIB_DEBUG%\OgreHlmsUnlit_d.lib" "%DST_LIB_DEBUG%\"
copy /Y "%SRC_LIB_DEBUG%\OgreMain_d.exp" "%DST_LIB_DEBUG%\"
copy /Y "%SRC_LIB_DEBUG%\OgreMain_d.lib" "%DST_LIB_DEBUG%\"
copy /Y "%SRC_LIB_DEBUG%\OgreMeshLodGenerator_d.exp" "%DST_LIB_DEBUG%\"
copy /Y "%SRC_LIB_DEBUG%\OgreMeshLodGenerator_d.lib" "%DST_LIB_DEBUG%\"
copy /Y "%SRC_LIB_DEBUG%\OgreOverlay_d.exp" "%DST_LIB_DEBUG%\"
copy /Y "%SRC_LIB_DEBUG%\OgreOverlay_d.lib" "%DST_LIB_DEBUG%\"
copy /Y "%SRC_LIB_DEBUG%\OgrePlanarReflections_d.exp" "%DST_LIB_DEBUG%\"
copy /Y "%SRC_LIB_DEBUG%\OgrePlanarReflections_d.lib" "%DST_LIB_DEBUG%\"
copy /Y "%SRC_LIB_DEBUG%\Plugin_ParticleFX2_d.exp" "%DST_LIB_DEBUG%\"
copy /Y "%SRC_LIB_DEBUG%\Plugin_ParticleFX2_d.lib" "%DST_LIB_DEBUG%\"
copy /Y "%SRC_LIB_DEBUG%\RenderSystem_Direct3D11_d.exp" "%DST_LIB_DEBUG%\"
copy /Y "%SRC_LIB_DEBUG%\RenderSystem_Direct3D11_d.lib" "%DST_LIB_DEBUG%\"
copy /Y "%SRC_LIB_DEBUG%\RenderSystem_GL3Plus_d.exp" "%DST_LIB_DEBUG%\"
copy /Y "%SRC_LIB_DEBUG%\RenderSystem_GL3Plus_d.lib" "%DST_LIB_DEBUG%\"
copy /Y "%SRC_LIB_DEBUG%\RenderSystem_NULL_d.exp" "%DST_LIB_DEBUG%\"
copy /Y "%SRC_LIB_DEBUG%\RenderSystem_NULL_d.lib" "%DST_LIB_DEBUG%\"
copy /Y "%SRC_LIB_DEBUG%\OgreAtmosphere_d.lib" "%DST_LIB_DEBUG%\"
copy /Y "%SRC_LIB_DEBUG%\OgreSceneFormat_d.lib" "%DST_LIB_DEBUG%\"

REM =========================
REM DEBUG BIN
REM =========================

copy /Y "%SRC_BIN_DEBUG%\*.dll" "%DST_BIN_DEBUG%\"
copy /Y "%SRC_BIN_DEBUG%\*.pdb" "%DST_BIN_DEBUG%\"
copy /Y "%SRC_BIN_DEBUG%\*.exe" "%DST_BIN_DEBUG%\"

REM =========================
REM RELEASE LIBS
REM =========================

copy /Y "%SRC_LIB_RELEASE%\OgreHlmsPbs.exp" "%DST_LIB_RELEASE%\"
copy /Y "%SRC_LIB_RELEASE%\OgreHlmsPbs.lib" "%DST_LIB_RELEASE%\"
copy /Y "%SRC_LIB_RELEASE%\OgreHlmsUnlit.exp" "%DST_LIB_RELEASE%\"
copy /Y "%SRC_LIB_RELEASE%\OgreHlmsUnlit.lib" "%DST_LIB_RELEASE%\"
copy /Y "%SRC_LIB_RELEASE%\OgreMain.exp" "%DST_LIB_RELEASE%\"
copy /Y "%SRC_LIB_RELEASE%\OgreMain.lib" "%DST_LIB_RELEASE%\"
copy /Y "%SRC_LIB_RELEASE%\OgreMeshLodGenerator.exp" "%DST_LIB_RELEASE%\"
copy /Y "%SRC_LIB_RELEASE%\OgreMeshLodGenerator.lib" "%DST_LIB_RELEASE%\"
copy /Y "%SRC_LIB_RELEASE%\OgreOverlay.exp" "%DST_LIB_RELEASE%\"
copy /Y "%SRC_LIB_RELEASE%\OgreOverlay.lib" "%DST_LIB_RELEASE%\"
copy /Y "%SRC_LIB_RELEASE%\OgrePlanarReflections.exp" "%DST_LIB_RELEASE%\"
copy /Y "%SRC_LIB_RELEASE%\OgrePlanarReflections.lib" "%DST_LIB_RELEASE%\"
copy /Y "%SRC_LIB_RELEASE%\Plugin_ParticleFX2.exp" "%DST_LIB_RELEASE%\"
copy /Y "%SRC_LIB_RELEASE%\Plugin_ParticleFX2.lib" "%DST_LIB_RELEASE%\"
copy /Y "%SRC_LIB_RELEASE%\RenderSystem_Direct3D11.exp" "%DST_LIB_RELEASE%\"
copy /Y "%SRC_LIB_RELEASE%\RenderSystem_Direct3D11.lib" "%DST_LIB_RELEASE%\"
copy /Y "%SRC_LIB_RELEASE%\RenderSystem_GL3Plus.exp" "%DST_LIB_RELEASE%\"
copy /Y "%SRC_LIB_RELEASE%\RenderSystem_GL3Plus.lib" "%DST_LIB_RELEASE%\"
copy /Y "%SRC_LIB_RELEASE%\RenderSystem_NULL.exp" "%DST_LIB_RELEASE%\"
copy /Y "%SRC_LIB_RELEASE%\RenderSystem_NULL.lib" "%DST_LIB_RELEASE%\"
copy /Y "%SRC_LIB_RELEASE%\OgreAtmosphere.lib" "%DST_LIB_RELEASE%\"
copy /Y "%SRC_LIB_RELEASE%\OgreSceneFormat.lib" "%DST_LIB_RELEASE%\"

REM =========================
REM RELEASE BIN
REM =========================

copy /Y "%SRC_BIN_RELEASE%\*.dll" "%DST_BIN_RELEASE%\"
copy /Y "%SRC_BIN_RELEASE%\*.exe" "%DST_BIN_RELEASE%\"

REM =========================
REM SDK MIRROR (directory copy still needs xcopy)
REM =========================

xcopy "E:\Ogre2.2SDK\OgreMain" ".\external\Ogre2.2SDK\OgreMain" /E /Y /I /Q
xcopy "E:\Ogre2.2SDK\Components" ".\external\Ogre2.2SDK\Components" /E /Y /I /Q
xcopy "E:\Ogre2.2SDK\PlugIns" ".\external\Ogre2.2SDK\PlugIns" /E /Y /I /Q
xcopy "E:\Ogre2.2SDK\RenderSystems" ".\external\Ogre2.2SDK\RenderSystems" /E /Y /I /Q
xcopy "E:\Ogre2.2SDK\Samples" ".\external\Ogre2.2SDK\Samples" /E /Y /I /Q
xcopy "E:\Ogre2.2SDK\VCBuild\include" ".\external\Ogre2.2SDK\VCBuild\include" /E /Y /I /Q

exit /b 0